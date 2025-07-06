#define __A_CONFIGURATION_PRIVATE__
#include "configuration.h"

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "yansi.h"
#include "ystr.h"
#include "yarray.h"
#include "yvar.h"
#include "yexec.h"
#include "yjson.h"
#include "yfile.h"
#include "agent.h"
#include "api.h"
#include "declare.h"
#include "utils.h"

/* ********** PUBLIC FUNCTIONS ********** */
/* Main function for configuration file generation. */
void exec_configuration(agent_t *agent) {
	config_crontab_t cron_type;
	ystr_t org_key = NULL;
	ystr_t hostname = NULL;
	ystr_t archives_path = NULL;
	bool scripts = true;
	ystr_t logfile = NULL;
	bool syslog = false;
	ystr_t crypt_pwd = NULL;

	// splashscreen
	printf("\n");
	printf(YANSI_BG_BLUE "%80c" YANSI_RESET "\n", ' ');
	printf(YANSI_BG_BLUE YANSI_WHITE "%28c%s%27c" YANSI_RESET "\n", ' ', "Arkiv agent configuration", ' ');
	printf(YANSI_BG_BLUE "%80c" YANSI_RESET "\n", ' ');
	printf("\n");

	/* programs check */
	// rclone
	check_rclone();
	// tar
	check_tar();
	// sha512sum
	check_sha512sum();
	// compression programs
	check_z();
	// encryption programs
	check_crypt();
	// web communication programs
	check_web();
	// crontab
	cron_type = check_cron();
	// database dump
	check_database_dump();

	/* needed user inputs */
	// ask for the organization key
	org_key = config_ask_orgkey(agent);
	// hostname
	hostname = config_ask_hostname(agent);
	// backup path
	archives_path = config_ask_archives_path(agent);
	// script authorization
	scripts = config_ask_scripts(agent);
	// log file
	logfile = config_ask_log_file(agent);
	// syslog
	syslog = config_ask_syslog(agent);
	// encryption password
	printf("\n");
	crypt_pwd = config_ask_encryption_password(agent);
	printf("\n");
	// write JSON file
	config_write_json_file(org_key, hostname, archives_path, scripts, logfile, syslog, crypt_pwd);
	// declare the server to arkiv.sh
	agent->conf.org_key = org_key;
	agent->conf.hostname = hostname;
	exec_declare(agent);
	// add agent to crontab
	config_add_to_crontab(agent, cron_type);
	// add log file management by logrotate
	config_add_to_logrotate(logfile);

	/* cleanup */
	agent->conf.org_key = agent->conf.hostname = NULL;
	ys_free(hostname);
	ys_free(org_key);
	ys_free(archives_path);
	ys_free(logfile);
	ys_free(crypt_pwd);
}

/* ********** PRIVATE FUNCTIONS ********** */
/* Asks for the organization key. */
static ystr_t config_ask_orgkey(agent_t *agent) {
	ystr_t ys = NULL;
	bool has_defined_key = false;

	if (agent->conf.org_key && ys_bytesize(agent->conf.org_key) == A_ORG_KEY_LENGTH)
		has_defined_key = true;
	for (; ; ) {
		printf("Please, enter your organization key (%d characters-long string):\n", A_ORG_KEY_LENGTH);
		if (has_defined_key)
			printf("[" YANSI_YELLOW "%s" YANSI_RESET "]\n", agent->conf.org_key);
		printf(YANSI_BLUE);
		fflush(stdout);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		fflush(stdout);
		ys_trim(ys);
		size_t key_size = ys_bytesize(ys);
		if (key_size == 0 && has_defined_key) {
			ys_free(ys);
			return (ys_dup(agent->conf.org_key));
		}
		if (key_size == A_ORG_KEY_LENGTH)
			return (ys);
		printf(YANSI_RED "Bad key (should be %d characters long)\n\n" YANSI_RESET, A_ORG_KEY_LENGTH);
	}
}
/* Asks for the hostname. */
static ystr_t config_ask_hostname(agent_t *agent) {
	ystr_t hostname = NULL;
	ystr_t ys = NULL;
	bool has_defined_hostname = false;

	if (!ys_empty(agent->conf.hostname))
		has_defined_hostname = true;
	// fetch hostname
	ystr_t path = get_program_path("hostname");
	ybin_t data = {0};
	ystatus_t res = yexec((path ? path : "/usr/bin/hostname"), NULL, NULL, &data, NULL);
	ys_free(path);
	if (res == YENOERR) {
		hostname = ys_copy(data.data);
		ys_trim(hostname);
	}
	ybin_delete_data(&data);
	if (res != YENOERR || ys_empty(hostname))
		ys_delete(&hostname);
	// ask user
	printf("What is the local computer name?");
	if (has_defined_hostname)
		printf(" [" YANSI_YELLOW "%s" YANSI_RESET "]", agent->conf.hostname);
	else if (hostname)
		printf(" [" YANSI_YELLOW "%s" YANSI_RESET "]", hostname);
	printf("\n" YANSI_BLUE);
	fflush(stdout);
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	if (!ys_empty(ys)) {
		ys_free(hostname);
		hostname = ys;
	} else if (has_defined_hostname) {
		ys_free(hostname);
		hostname = ys_dup(agent->conf.hostname);
	}
	return (hostname);
}
/* Asks for the local archives path. */
static ystr_t config_ask_archives_path(agent_t *agent) {
	ystr_t ys = NULL;

	printf("Path to the local archives directory? [" YANSI_YELLOW "%s" YANSI_RESET "]\n" YANSI_BLUE, agent->conf.archives_path);
	fflush(stdout);
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	if (!ys_empty(ys))
		return (ys);
	ys_free(ys);
	return (ys_dup(agent->conf.archives_path));
}
/* Asks if pre- and post-scripts are allowed. */
static bool config_ask_scripts(agent_t *agent) {
	ystr_t ys = NULL;
	bool result = false;

	for (; ; ) {
		printf(
			"Do you want to be able to execute pre- and post-scripts on this host? "
			"[" YANSI_YELLOW "%s" YANSI_RESET "/" YANSI_YELLOW "%s" YANSI_RESET "] " YANSI_BLUE,
			(agent->conf.scripts_allowed ? "Y" : "y"),
			(agent->conf.scripts_allowed ? "n" : "N")
		);
		fflush(stdout);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		if (!ys_empty(ys) && strcmp(ys, "n") && strcmp(ys, "N") && strcmp(ys, "y") && strcmp(ys, "Y")) {
			printf(YANSI_RED "Incorrect value. Try again." YANSI_RESET "\n");
			continue;
		}
		if (ys_empty(ys))
			result = agent->conf.scripts_allowed;
		else if (!strcasecmp(ys, "y"))
			result = true;
		ys_free(ys);
		return (result);
	}
}
/* Asks for the log file. */
static ystr_t config_ask_log_file(agent_t *agent) {
	ystr_t ys = NULL;

	printf("Path to the log file? [" YANSI_YELLOW "%s" YANSI_RESET "]\n" YANSI_BLUE, agent->conf.logfile);
	fflush(stdout);
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	if (!ys_empty(ys))
		return (ys);
	ys_free(ys);
	return (ys_dup(agent->conf.logfile));
}
/* Asks for syslog. */
static bool config_ask_syslog(agent_t *agent) {
	ystr_t ys = NULL;
	bool result = false;

	// ask for syslog
	for (; ; ) {
		printf(
			"Do you want to send logs to syslog? [" YANSI_YELLOW "%s" YANSI_RESET
			"/" YANSI_YELLOW "%s" YANSI_RESET "] " YANSI_BLUE,
			(agent->conf.use_syslog ? "Y" : "y"),
			(agent->conf.use_syslog ? "n" : "N")
		);
		fflush(stdout);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		if (!ys_empty(ys) && strcmp(ys, "n") && strcmp(ys, "N") && strcmp(ys, "y") && strcmp(ys, "Y")) {
			printf(YANSI_RED "Incorrect value. Try again." YANSI_RESET "\n");
			continue;
		}
		if (ys_empty(ys))
			result = agent->conf.use_syslog;
		else if (!strcasecmp(ys, "y"))
			result = true;
		ys_free(ys);
		return (result);
	}
}
/* Asks for the encryption password. */
static ystr_t config_ask_encryption_password(agent_t *agent) {
	ystr_t ys = NULL;
	bool has_defined_pwd = false;

	if (!ys_empty(agent->conf.crypt_pwd))
		has_defined_pwd = true;
	for (; ; ) {
		printf("Please enter your encryption password. "
		       "It must be at least 24 characters long (40 characters is recommended).\n");
		printf("You can generate a strong password with this command: "
		       YANSI_TEAL "head -c 32 /dev/urandom | base64" YANSI_RESET "\n");
		if (has_defined_pwd)
			printf("[" YANSI_YELLOW "%s" YANSI_RESET "]\n", agent->conf.crypt_pwd);
		printf(YANSI_BLUE);
		fflush(stdout);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		if (ys_empty(ys) && has_defined_pwd) {
			ys_free(ys);
			return (ys_dup(agent->conf.crypt_pwd));
		}
		if (ys_bytesize(ys) >= A_MINIMUM_CRYPT_PWD_LENGTH)
			return (ys);
		printf(YANSI_RED "Password too short." YANSI_RESET "\n");
	}
}
/* Writes the JSON configuration file. */
static void config_write_json_file(const char *org_key, const char *hostname, const char *archives_path,
                                   bool scripts_allowed, const char *logfile, bool syslog, const char *crypt_pwd) {
	yvar_t *config = yvar_new_table(NULL);
	ytable_t *table = yvar_get_table(config);
	ytable_set_key(table, A_JSON_ORG_KEY, yvar_new_const_string(org_key));
	ytable_set_key(table, A_JSON_HOSTNAME, yvar_new_const_string(hostname));
	ytable_set_key(table, A_JSON_ARCHIVES_PATH, yvar_new_const_string(archives_path));
	ytable_set_key(table, A_JSON_SCRIPTS, yvar_new_bool(scripts_allowed));
	ytable_set_key(table, A_JSON_LOGFILE, yvar_new_const_string(logfile));
	ytable_set_key(table, A_JSON_SYSLOG, yvar_new_bool(syslog));
	ytable_set_key(table, A_JSON_CRYPT_PWD, yvar_new_const_string(crypt_pwd));
	printf("‣ Writing configuration file " YANSI_PURPLE A_PATH_AGENT_CONFIG YANSI_RESET "... ");
	fflush(stdout);
	if (!yfile_touch(A_PATH_AGENT_CONFIG, 0600, 0700) ||
	    yjson_write(A_PATH_AGENT_CONFIG, config, true) != YENOERR) {
		printf(YANSI_RED "failed. Please try again." YANSI_RESET "\n\n");
		printf(YANSI_RED "Abort." YANSI_RESET "\n");
		fflush(stdout);
		exit(2);
	}
	printf(YANSI_GREEN "done" YANSI_RESET "\n");
	ytable_free(table);
}
/* Add the agent execution to the crontab. */
static void config_add_to_crontab(agent_t *agent, config_crontab_t cron_type) {
	ystr_t ys = NULL;

	// search for /etc/cron.hourly directory
	if (cron_type == A_CONFIG_CRON_HOURLY) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_HOURLY_PATH YANSI_RESET ")... ");
		fflush(stdout);
		ys = ys_printf(NULL, A_CRONTAB_SCRIPT, agent->agent_path);
		if (yfile_put_string(A_CRON_HOURLY_PATH, ys) &&
		    !chmod(A_CRON_HOURLY_PATH, 0755)) {
			printf(YANSI_GREEN "done" YANSI_RESET "\n");
			ys_free(ys);
			return;
		}
		unlink(A_CRON_HOURLY_PATH);
		printf(YANSI_RED "failed. Please try again." YANSI_RESET "\n\n");
		printf(YANSI_RED "Abort." YANSI_RESET "\n");
		exit(2);
	}
	// search for /etc/cron.d directory
	if (cron_type == A_CONFIG_CRON_D) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_D_PATH YANSI_RESET ")... ");
		fflush(stdout);
		ys = ys_printf(NULL, A_CRONTAB_LINE, agent->agent_path);
		if (yfile_put_string(A_CRON_D_PATH, ys) &&
		    !chmod(A_CRON_D_PATH, 0644)) {
			printf(YANSI_GREEN "done" YANSI_RESET "\n");
			ys_free(ys);
			return;
		}
		unlink(A_CRON_D_PATH);
		printf(YANSI_RED "failed. Please try again." YANSI_RESET "\n\n");
		printf(YANSI_RED "Abort." YANSI_RESET "\n");
		exit(2);
	}
	// search for /etc/crontab file
	if (cron_type == A_CONFIG_CRON_CRONTAB) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_ETC_PATH YANSI_RESET ")... ");
		fflush(stdout);
		ys = ys_printf(NULL, A_CRONTAB_LINE, agent->agent_path);
		if (yfile_contains(A_CRON_ETC_PATH, ys)) {
			printf(YANSI_GREEN "already done" YANSI_RESET "\n");
			ys_free(ys);
			return;
		}
		if (yfile_append_string(A_CRON_ETC_PATH, ys)) {
			printf(YANSI_GREEN "done" YANSI_RESET "\n");
			ys_free(ys);
			return;
		}
		printf(YANSI_RED "failed. Please try again." YANSI_RESET "\n\n");
		printf(YANSI_RED "Abort." YANSI_RESET "\n");
		exit(2);
	}
}
/* Add log file management by logrotate, if possible. */
static void config_add_to_logrotate(const char *logfile) {
	printf("‣ Add to logrotate (file " YANSI_PURPLE A_LOGROTATE_CONFIG_PATH YANSI_RESET ")... ");
	fflush(stdout);
	ystr_t ys = ys_printf(NULL, A_LOGROTATE_CONFIG_CONTENT, logfile);
	if (!ys) {
		printf(YANSI_RED "failed (memory error). " YANSI_RESET "No log rotation set.\n");
		return;
	}
	if (!yfile_put_string(A_LOGROTATE_CONFIG_PATH, ys) ||
	    chmod(A_LOGROTATE_CONFIG_PATH, 0644)) {
		unlink(A_LOGROTATE_CONFIG_PATH);
		printf(YANSI_RED "failed. " YANSI_RESET "No log rotation set.\n");
	} else {
		printf(YANSI_GREEN "done" YANSI_RESET "\n");
	}
	ys_free(ys);
}

