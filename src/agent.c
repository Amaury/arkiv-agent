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
	agent->conf.use_ansi = !ys || !ys[0] || !STR_IS_FALSE(ys);
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
	agent->exec_log.backup_mysql = ytable_new();
	agent->exec_log.backup_pgsql = ytable_new();
	agent->exec_log.post_scripts = ytable_new();
	if (!agent->exec_log.pre_scripts || !agent->exec_log.backup_files ||
	    !agent->exec_log.backup_mysql || !agent->exec_log.backup_pgsql ||
	    !agent->exec_log.post_scripts) {
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
	// get JSON values
	yvar_t *hostname = ytable_get_key_data(json, A_JSON_HOSTNAME);
	yvar_t *org_key = ytable_get_key_data(json, A_JSON_ORG_KEY);
	yvar_t *archives_path = ytable_get_key_data(json, A_JSON_ARCHIVES_PATH);
	yvar_t *scripts = ytable_get_key_data(json, A_JSON_SCRIPTS);
	yvar_t *logfile = ytable_get_key_data(json, A_JSON_LOGFILE);
	yvar_t *syslog = ytable_get_key_data(json, A_JSON_SYSLOG);
	yvar_t *crypt_pwd = ytable_get_key_data(json, A_JSON_CRYPT_PWD);
	// check hostname
	if (!hostname || !yvar_is_string(hostname) || !(ys = yvar_get_string(hostname)) || ys_empty(ys)) {
		if (permissive)
			goto cleanup;
		printf(YANSI_RED "Empty hostname in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.hostname = ys_copy(ys);
	// check organization key
	if (!org_key || !yvar_is_string(org_key) || !(ys = yvar_get_string(org_key)) || ys_empty(ys)) {
		if (permissive)
			goto cleanup;
		printf(YANSI_RED "Empty organization key in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.org_key = ys_copy(ys);
	// check archives path
	if (!archives_path || !yvar_is_string(archives_path) || !(ys = yvar_get_string(archives_path)) || ys_empty(ys)) {
		if (permissive)
			goto cleanup;
		printf(YANSI_RED "Empty archives path in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.archives_path = agent_getenv_static(A_ENV_ARCHIVES_PATH, ys);
	// check pre- and post-scripts authorization
	if (!scripts || !yvar_is_bool(scripts) || yvar_get_bool(scripts) == true) {
		agent->conf.scripts_allowed = true;
	}
	// check encryption password
	if (!crypt_pwd || !yvar_is_string(crypt_pwd) || !(ys = yvar_get_string(crypt_pwd)) || ys_empty(ys)) {
		if (permissive)
			goto cleanup;
		printf(YANSI_RED "Empty encryption password in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.crypt_pwd = agent_getenv_static(A_ENV_CRYPT_PWD, ys);
	// check logfile and initialize log's file descriptor
	if (!logfile || !yvar_is_string(logfile) || !(ys = yvar_get_string(logfile)) || ys_empty(ys)) {
		// no log file - write to stdout
		agent->conf.logfile = NULL;
		agent->log_fd = NULL;
		agent->conf.use_stdout = true;
	} else {
		agent->conf.logfile = agent_getenv_static(A_ENV_ARCHIVES_PATH, ys);
		agent->log_fd = fopen(agent->conf.logfile, "a");
		if (!agent->log_fd)
			agent->conf.use_stdout = true;
	}
	// check stdout
	ys = agent_getenv(A_ENV_STDOUT, NULL);
	if (ys) {
		if (STR_IS_TRUE(ys))
			agent->conf.use_stdout = true;
		else
			agent->conf.use_stdout = false;
		ys_free(ys);
	}
	// check syslog and initialize syslog connection
	ys = agent_getenv(A_ENV_SYSLOG, NULL);
	enum { A_SYSLOG_UNDEF, A_SYSLOG_FORCE, A_SYSLOG_AVOID } use_syslog = A_SYSLOG_UNDEF;
	if (ys) {
		if (STR_IS_TRUE(ys))
			use_syslog = A_SYSLOG_FORCE;
		else
			use_syslog = A_SYSLOG_AVOID;
		ys_free(ys);
	}
	if (use_syslog == A_SYSLOG_FORCE ||
	    (use_syslog == A_SYSLOG_UNDEF && syslog && yvar_is_bool(syslog) && yvar_get_bool(syslog) == true)) {
		agent->conf.use_syslog = true;
		openlog(A_SYSLOG_IDENT, LOG_CONS, LOG_USER);
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

