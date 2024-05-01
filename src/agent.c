/**
 * Arkiv agent.
 * Command line:
 * ./agent
 * debug_mode=true ./agent
 * logfile=/var/log/arkiv/arkiv.log ./agent
 * debug_mode=true ./agent
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2024, Amaury Bouchard
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "ymemory.h"
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
	// set default configuration file
	agent->conf_path = agent_getenv_static(A_ENV_CONF, A_PATH_AGENT_CONFIG);
	// set default log file
	agent->conf.logfile = agent_getenv_static(A_ENV_LOGFILE, A_PATH_LOGFILE);
	// set default local archives path
	agent->conf.archives_path = agent_getenv_static(A_ENV_ARCHIVES_PATH, A_PATH_ARCHIVES);
	/*
	agent->log.backup_files = yarray_new();
	agent->log.backup_databases = yarray_new();
	agent->log.upload_s3 = yarray_new();
	*/
	return (agent);
}
/* Returns a copy of an environment variable, or a default value. */
ystr_t agent_getenv(char *envvar, ystr_t default_value) {
	char *value = secure_getenv(envvar);
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
	char *value = secure_getenv(envvar);
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
void agent_load_configuration(agent_t *agent) {
	ystr_t ys = NULL;
	// read the configuration file
	ys = yfile_get_string_contents(agent->conf_path);
	// parsing the configuration file
	yjson_parser_t *json_parser = yjson_new();
	yres_var_t res = yjson_parse(json_parser, ys);
	ys_free(ys);
	// check result
	if (YRES_STATUS(res) != YENOERR) {
		printf(YANSI_RED "Unable to read configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	yvar_t json_var = YRES_VAL(res);
	if (!yvar_is_table(&json_var)) {
		printf(YANSI_RED "Wrongly formatted configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	ytable_t *json = yvar_get_table(&json_var);
	// get JSON values
	yvar_t *hostname = ytable_get_key_data(json, A_JSON_HOSTNAME);
	yvar_t *org_key = ytable_get_key_data(json, A_JSON_ORG_KEY);
	yvar_t *archives_path = ytable_get_key_data(json, A_JSON_ARCHIVES_PATH);
	yvar_t *logfile = ytable_get_key_data(json, A_JSON_LOGFILE);
	yvar_t *syslog = ytable_get_key_data(json, A_JSON_SYSLOG);
	yvar_t *crypt_pwd = ytable_get_key_data(json, A_JSON_CRYPT_PWD);
	// check hostname
	if (!hostname || !yvar_is_string(hostname) || !(ys = yvar_get_string(hostname)) || ys_empty(ys)) {
		printf(YANSI_RED "Empty hostname in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.hostname = ys_copy(ys);
	// check organization key
	if (!org_key || !yvar_is_string(org_key) || !(ys = yvar_get_string(org_key)) || ys_empty(ys)) {
		printf(YANSI_RED "Empty organization key in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.org_key = ys_copy(ys);
	// check archives path
	if (!archives_path || !yvar_is_string(archives_path) || !(ys = yvar_get_string(archives_path)) || ys_empty(ys)) {
		printf(YANSI_RED "Empty archives path in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.archives_path = agent_getenv_static(A_ENV_ARCHIVES_PATH, ys);
	// check encryption password
	if (!crypt_pwd || !yvar_is_string(crypt_pwd) || !(ys = yvar_get_string(crypt_pwd)) || ys_empty(ys)) {
		printf(YANSI_RED "Empty encryption password in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.crypt_pwd = agent_getenv_static(A_ENV_CRYPT_PWD, ys);
	// check logfile
	if (!logfile || !yvar_is_string(logfile) || !(ys = yvar_get_string(logfile)) || ys_empty(ys)) {
		printf(YANSI_RED "Empty log file in configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	agent->conf.logfile = agent_getenv_static(A_ENV_ARCHIVES_PATH, ys);
	// check syslog
	bool bool_value = true;
	if (!syslog || (!yvar_is_bool(syslog) && !yvar_is_string(syslog)) ||
	    (yvar_is_bool(syslog) && (bool_value = yvar_get_bool(syslog)) == true) ||
	    (yvar_is_string(syslog) &&
	     (!(ys = yvar_get_string(syslog)) ||
	      (strcmp(ys, A_SYSLOG_USER) && strcmp(ys, A_SYSLOG_LOCAL0) && strcmp(ys, A_SYSLOG_LOCAL1) &&
	       strcmp(ys, A_SYSLOG_LOCAL2) && strcmp(ys, A_SYSLOG_LOCAL3) && strcmp(ys, A_SYSLOG_LOCAL4) &&
	       strcmp(ys, A_SYSLOG_LOCAL5) && strcmp(ys, A_SYSLOG_LOCAL6) && strcmp(ys, A_SYSLOG_LOCAL7))))) {
		printf(YANSI_RED "Incorrect syslog valuein configuration file '%s'.\n" YANSI_RESET, agent->conf_path);
		exit(3);
	}
	ystr_t final_value = agent_getenv(A_ENV_SYSLOG, NULL);
	if (!final_value && bool_value == false) {
		agent->conf.use_syslog = false;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_USER)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_USER))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_USER;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_LOCAL0)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_LOCAL0))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_LOCAL0;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_LOCAL1)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_LOCAL1))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_LOCAL1;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_LOCAL2)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_LOCAL2))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_LOCAL2;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_LOCAL3)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_LOCAL3))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_LOCAL3;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_LOCAL4)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_LOCAL4))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_LOCAL4;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_LOCAL5)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_LOCAL5))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_LOCAL5;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_LOCAL6)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_LOCAL6))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_LOCAL6;
	} else if ((final_value && !strcmp(final_value, A_SYSLOG_LOCAL7)) ||
	           (!final_value && !strcmp(ys, A_SYSLOG_LOCAL7))) {
		agent->conf.use_syslog = true;
		agent->conf.syslog_facility = LOG_LOCAL7;
	}
	ys_free(final_value);
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

