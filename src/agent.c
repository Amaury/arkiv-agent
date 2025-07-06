/**
 * Arkiv agent.
 * Command line:
 * ./agent
 * debug=true ./agent
 * logfile=/var/log/arkiv/arkiv.log ./agent
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2024, Amaury Bouchard
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "ymemory.h"
#include "ydefs.h"
#include "yansi.h"
#include "ystr.h"
#include "yfile.h"
#include "yvar.h"
#include "yresult.h"
#include "yjson.h"
#include "yexec.h"
#include "utils.h"
#include "agent.h"

/* Create a new agent structure. */
agent_t *agent_new(char *exe_path) {
	agent_t *agent;

	// memory allocation, and get agent path
	if (!(agent = malloc0(sizeof(agent_t))) ||
	    (!(agent->agent_path = realpath(exe_path, NULL)) &&
	     !(agent->agent_path = strdup(exe_path)))) {
		printf(YANSI_RED "Memory allocation error. Abort." YANSI_RESET);
		exit(1);
	}
	// set execution timestamp
	agent->exec_timestamp = time(NULL);
	// set default configuration file
	agent->conf_path = agent_getenv_static(A_ENV_CONF, A_PATH_AGENT_CONFIG);
	// set default log file
	agent->conf.logfile = agent_getenv_static(A_ENV_LOGFILE, A_PATH_LOGFILE);
	// set default local archives path
	agent->conf.archives_path = agent_getenv_static(A_ENV_ARCHIVES_PATH, A_PATH_ARCHIVES);
	// manage debug mode
	ystr_t ys = agent_getenv(A_ENV_DEBUG_MODE, NULL);
	if (STR_IS_TRUE(ys)) {
		agent->debug_mode = true;
		agent->conf.use_stdout = true;
	}
	ys_free(ys);
	// manage ANSI parameter
	ys = agent_getenv(A_ENV_ANSI, NULL);
	agent->conf.use_ansi = !STR_IS_FALSE(ys);
	ys_free(ys);
	return (agent);
}
/* Returns a copy of an environment variable, or a default value. */
ystr_t agent_getenv(char *envvar, ystr_t default_value) {
#if defined ZIG_PLATFORM_MACOS_X86_64 || defined ZIG_PLATFORM_MACOS_ARM_64
	char *value = getenv(envvar);
#else
	char *value = secure_getenv(envvar);
#endif
	ystr_t ys = NULL;
	if (value) {
		ys = ys_copy(value);
		if (!ys) {
			printf(YANSI_RED "Memory allocation error. Abort.\n" YANSI_RESET);
			exit(3);
		}
	} else {
		ys = default_value;
	}
	return (ys);
}
/* Returns a copy of an environment variable, or a copy of a default value. */
ystr_t agent_getenv_static(char *envvar, const char *default_value) {
#if ZIG_PLATFORM_MACOS_X86_64 || ZIG_PLATFORM_MACOS_ARM_64
	char *value = getenv(envvar);
#else
	char *value = secure_getenv(envvar);
#endif
	ystr_t ys = NULL;
	if (value) {
		ys = ys_copy(value);
	} else {
		if (!default_value)
			return (NULL);
		ys = ys_copy(default_value);
	}
	if (!ys) {
		printf(YANSI_RED "Memory allocation error. Abort.\n" YANSI_RESET);
		exit(3);
	}
	return (ys);
}
/* Reads the configuration file. */
void agent_load_configuration(agent_t *agent, bool permissive) {
	ystr_t ys = NULL;
	ytable_t *json = NULL;

	// init
	agent->exec_log.pre_scripts = ytable_new();
	agent->exec_log.backup_files = ytable_new();
	agent->exec_log.backup_databases = ytable_new();
	agent->exec_log.post_scripts = ytable_new();
	if (!agent->exec_log.pre_scripts || !agent->exec_log.backup_files ||
	    !agent->exec_log.backup_databases || !agent->exec_log.post_scripts) {
		printf(YANSI_RED "Memory allocation error\n" YANSI_RESET);
		exit(3);
	}
	agent->exec_log.status_pre_scripts = true;
	agent->exec_log.status_files = true;
	agent->exec_log.status_databases = true;
	agent->exec_log.status_post_scripts = true;
	// read the configuration file
	ys = yfile_get_string_contents(agent->conf_path);
	// parsing the configuration file
	yjson_parser_t *json_parser = yjson_new();
	yres_var_t res = yjson_parse(json_parser, ys);
	ys_free(ys);
	// check result
	if (YRES_STATUS(res) != YENOERR) {
		if (permissive)
			goto cleanup;
		printf(YANSI_RED "Unable to read configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	yvar_t json_var = YRES_VAL(res);
	if (!yvar_is_table(&json_var)) {
		if (permissive)
			goto cleanup;
		printf(YANSI_RED "Wrongly formatted configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	json = yvar_get_table(&json_var);
	// manage hostname
	ys = agent_getenv(A_ENV_HOSTNAME, NULL);
	if (!ys_empty(ys)) {
		// got hostname from environment
		agent->conf.hostname = ys;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		yvar_t *hostname = ytable_get_key_data(json, A_JSON_HOSTNAME);
		if (yvar_is_string(hostname) && (ys = yvar_get_string(hostname)) && !ys_empty(ys)) {
			// got hostname from configuration file
			agent->conf.hostname = ys_copy(ys);
		} else {
			// get hostname from the system
			ys = get_program_path("hostname");
			ybin_t data = {0};
			ystatus_t res = yexec((!ys_empty(ys) ? ys : "/usr/bin/hostname"), NULL, NULL, &data, NULL);
			ys_delete(&ys);
			ys = NULL;
			if (res == YENOERR) {
				ys = ys_copy(data.data);
				ys_trim(ys);
			}
			ybin_delete_data(&data);
			if (res != YENOERR || ys_empty(ys)) {
				ys_delete(&ys);
				if (permissive)
					goto cleanup;
				printf(YANSI_RED "Empty host name in configuration file '%s'." YANSI_RESET, agent->conf_path);
				exit(3);
			}
			agent->conf.hostname = ys;
		}
	}
	// mmanage standalone mode
	ys = agent_getenv(A_ENV_STANDALONE, NULL);
	if (!ys_empty(ys)) {
		// got value from environement
		agent->conf.standalone = STR_IS_TRUE(ys) ? true : false;
	} else {
		yvar_t *standalone = ytable_get_key_data(json, A_JSON_STANDALONE);
		if (yvar_is_bool(standalone)) {
			// got value from configuration file
			agent->conf.standalone = yvar_get_bool(standalone);
		}
	}
	ys_delete(&ys);
	// manage organization key
	ys = agent_getenv(A_ENV_ORG_KEY, NULL);
	if (!ys_empty(ys)) {
		// got value from environment
		agent->conf.org_key = ys;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		// search value in the configuration file
		yvar_t *org_key = ytable_get_key_data(json, A_JSON_ORG_KEY);
		if (!yvar_is_string(org_key) || !(ys = yvar_get_string(org_key)) || ys_empty(ys)) {
			// not found
			if (permissive)
				goto cleanup;
			printf(YANSI_RED "Empty organization key in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
			exit(3);
		}
		agent->conf.org_key = ys_copy(ys);
	}
	// manage scripts authorization
	ys = agent_getenv(A_ENV_SCRIPTS, NULL);
	if (!ys_empty(ys)) {
		// got value from environment
		agent->conf.scripts_allowed = STR_IS_TRUE(ys) ? true : false;
		ys_delete(&ys);
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		yvar_t *scripts = ytable_get_key_data(json, A_JSON_SCRIPTS);
		if (yvar_is_bool(scripts)) {
			// got value from configuration file
			agent->conf.scripts_allowed = yvar_get_bool(scripts);
		} else {
			// default value
			agent->conf.scripts_allowed = true;
		}
	}
	// manage archives path
	ys = agent_getenv(A_ENV_ARCHIVES_PATH, NULL);
	if (!ys_empty(ys)) {
		// get value from environment
		agent->conf.archives_path = ys;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		// search value in the configuration file
		yvar_t *archives_path = ytable_get_key_data(json, A_JSON_ARCHIVES_PATH);
		if (!yvar_is_string(archives_path) || !(ys = yvar_get_string(archives_path)) || ys_empty(ys)) {
			if (permissive)
				goto cleanup;
			printf(YANSI_RED "Empty archives path in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
			exit(3);
		}
		agent->conf.archives_path = ys_copy(ys);
	}
	// manage logfile and initialize log's file descriptor
	ys = agent_getenv(A_ENV_LOGFILE, NULL);
	if (!ys_empty(ys)) {
		// got value from environment
		agent->conf.logfile = ys;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		yvar_t *logfile = ytable_get_key_data(json, A_JSON_LOGFILE);
		if (yvar_is_string(logfile) && (ys = yvar_get_string(logfile)) && !ys_empty(ys)) {
			// got value from configuration file
			agent->conf.logfile = ys_copy(ys);
		} else {
			// default value
			agent->conf.logfile = ys_copy(A_PATH_LOGFILE);
		}
	}
	if (!agent->conf.logfile || !strcmp(agent->conf.logfile, "/dev/null") ||
	    !(agent->log_fd = fopen(agent->conf.logfile, "a"))) {
		ys_delete(&agent->conf.logfile);
		agent->log_fd = NULL;
		agent->conf.use_stdout = true;
	}
	// manage syslog and initialize syslog connection
	enum { A_SYSLOG_UNDEF, A_SYSLOG_FORCE, A_SYSLOG_AVOID } use_syslog = A_SYSLOG_UNDEF;
	ys = agent_getenv(A_ENV_SYSLOG, NULL);
	if (!ys_empty(ys)) {
		use_syslog = STR_IS_TRUE(ys) ? A_SYSLOG_FORCE : A_SYSLOG_AVOID;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		yvar_t *syslog = ytable_get_key_data(json, A_JSON_SYSLOG);
		if (yvar_is_bool(syslog))
			use_syslog = yvar_get_bool(syslog) ? A_SYSLOG_FORCE : A_SYSLOG_AVOID;
	}
	if (use_syslog == A_SYSLOG_FORCE) {
		agent->conf.use_syslog = true;
		openlog(A_SYSLOG_IDENT, LOG_CONS, LOG_USER);
	}
	// manage stdout
	ys = agent_getenv(A_ENV_STDOUT, NULL);
	if (!ys_empty(ys)) {
		// got value from environment
		agent->conf.use_stdout = STR_IS_TRUE(ys) ? true : false;
		ys_free(ys);
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		yvar_t *var = ytable_get_key_data(json, A_JSON_STDOUT);
		if (yvar_is_bool(var)) {
			// got value from configuration file
			agent->conf.use_stdout = yvar_get_bool(var);
		}
	}
	// manage ANSI only if it wasn't overridden by environment in agent_new()
	if (agent->conf.use_ansi) {
		yvar_t *var = ytable_get_key_data(json, A_JSON_ANSI);
		if (yvar_is_bool(var)) {
			// got value from configuration file
			agent->conf.use_ansi = yvar_get_bool(var);
		}
	}
	// manage encryption password
	ys = agent_getenv(A_ENV_CRYPT_PWD, NULL);
	if (!ys_empty(ys)) {
		// got value from environment
		agent->conf.crypt_pwd = ys;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		// search value in the configuration file
		yvar_t *var = ytable_get_key_data(json, A_JSON_CRYPT_PWD);
		if (!yvar_is_string(var) || !(ys = yvar_get_string(var)) || ys_empty(ys)) {
			if (permissive)
				goto cleanup;
			printf(YANSI_RED "Empty encryption password in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
			exit(3);
		}
		agent->conf.crypt_pwd = ys_copy(ys);
	}
	// manage API base URL
	ys = agent_getenv(A_ENV_API_URL, NULL);
	if (!ys_empty(ys)) {
		// got value from environment
		agent->conf.api_base_url = ys;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		yvar_t *var = ytable_get_key_data(json, A_JSON_API_URL);
		if (yvar_is_string(var) && (ys = yvar_get_string(var)) && !ys_empty(ys)) {
			// got value from the configuration file
			agent->conf.api_base_url = ys_copy(ys);
		} else {
			// use default value
			agent->conf.api_base_url = ys_copy(A_API_BASE_URL);
		}
	}
	// manage parameter file's URL
	ys = agent_getenv(A_ENV_PARAM_URL, NULL);
	if (!ys_empty(ys)) {
		// got value from environment
		agent->conf.param_url = ys;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		yvar_t *var = ytable_get_key_data(json, A_JSON_PARAM_URL);
		if (yvar_is_string(var) && (ys = yvar_get_string(var)) && !ys_empty(ys)) {
			// got value from the configuration file
			agent->conf.param_url = ys_copy(ys);
		} else {
			// use default value
			agent->conf.param_url = ys_copy(A_API_URL_SERVER_PARAM);
		}
	}
	if (!ys_empty(agent->conf.param_url) && strstr(agent->conf.param_url, "[ORG]") && !ys_empty(agent->conf.org_key)) {
		// replace "[ORG]" in the URL
		ys = ys_subs(agent->conf.param_url, "[ORG]", agent->conf.org_key);
		ys_free(agent->conf.param_url);
		agent->conf.param_url = ys;
	}
	if (!ys_empty(agent->conf.param_url) && strstr(agent->conf.param_url, "[HOST]") && !ys_empty(agent->conf.hostname)) {
		// replace "[HOST]" in the URL
		ys = ys_subs(agent->conf.param_url, "[HOST]", agent->conf.hostname);
		ys_free(agent->conf.param_url);
		agent->conf.param_url = ys;
	}
	// manage local parameter file's path
	ys = agent_getenv(A_ENV_PARAM_FILE, NULL);
	if (!ys_empty(ys)) {
		// got value from environment
		agent->conf.param_file = ys;
	} else {
		ys_delete(&ys); // in case of allocated but empty string
		yvar_t *var = ytable_get_key_data(json, A_JSON_PARAM_FILE);
		if (yvar_is_string(var) && (ys = yvar_get_string(var)) && !ys_empty(ys)) {
			// got value from the configuration file
			agent->conf.param_file = ys_copy(ys);
		} else {
			// use default value
			agent->conf.param_file = ys_copy(A_PATH_PARAM_FILE);
		}
	}
	// manage debug mode only if it wasn't overridden by environment in agent_new()
	if (!agent->debug_mode) {
		yvar_t *var = ytable_get_key_data(json, A_JSON_DEBUG_MODE);
		if (yvar_is_bool(var)) {
			// got value from configuration file
			agent->debug_mode = yvar_get_bool(var);
		}
	}
cleanup:
	ytable_free(json);
	yjson_free(json_parser);
}
/* Frees a previously created agent structure. */
void agent_free(agent_t *agent) {
	if (!agent)
		return;
	/*
	ys_del(&agent->backup_path);
	free0(agent->conf.hostname);
	free0(agent->conf.public_key);
	free0(agent->conf.private_key);
	free0(agent->bin_path.mysqldump);
	free0(agent->bin_path.xtrabackup);
	free0(agent->bin_path.awscli);
	free0(agent->mysqldump.user);
	free0(agent->mysqldump.pwd);
	free0(agent->mysqldump.host);
	yarray_del(&agent->mysqldump.db, callback_free_raw, NULL);
	free0(agent->aws.public_key);
	free0(agent->aws.private_key);
	free0(agent->aws.bucket);
	free0(agent->aws.region);
	yarray_del(&agent->log.backup_files, callback_free_log_item, NULL);
	yarray_del(&agent->log.backup_databases, callback_free_log_item, NULL);
	yarray_del(&agent->log.upload_s3, callback_free_log_item, NULL);
	*/
	free0(agent);
}

