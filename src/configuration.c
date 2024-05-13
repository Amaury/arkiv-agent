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

	/* needed user inputs */
	// ask for the organization key
	org_key = config_ask_orgkey();
	// hostname
	hostname = config_ask_hostname();
	// backup path
	archives_path = config_ask_archives_path(agent);
	// log file
	logfile = config_ask_log_file(agent);
	// syslog
	syslog = config_ask_syslog();
	// encryption password
	printf("\n");
	crypt_pwd = config_ask_encryption_password();
	printf("\n");
	// write JSON file
	config_write_json_file(org_key, hostname, archives_path, logfile, syslog, crypt_pwd);
	// declare the server to arkiv.sh
	agent->conf.org_key = org_key;
	agent->conf.hostname = hostname;
	exec_declare(agent);
	// add agent to crontab
	config_add_to_crontab(agent, cron_type);

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
static ystr_t config_ask_orgkey(void) {
	ystr_t ys = NULL;

	for (; ; ) {
		printf("Please, enter your organization key (45 characters-long string):\n" YANSI_BLUE);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		size_t keySize = ys_bytesize(ys);
		if (keySize == A_ORG_KEY_LENGTH)
			return (ys);
		printf(YANSI_RED "Bad key (should be %d characters long)\n\n" YANSI_RESET, A_ORG_KEY_LENGTH);
	}
}
/* Asks for the hostname. */
static ystr_t config_ask_hostname(void) {
	ystr_t hostname = NULL;
	ystr_t ys = NULL;

	// fetch hostname
	ybin_t data = {0};
	ystatus_t res = yexec("/usr/bin/hostname", NULL, NULL, &data, NULL);
	if (res == YENOERR) {
		hostname = ys_copy(data.data);
		ys_trim(hostname);
	}
	ybin_delete_data(&data);
	if (res != YENOERR || ys_empty(hostname)) {
		hostname = ys_free(hostname);
		printf(YANSI_RED "Unable to get local host name.\n" YANSI_RESET);
	}
	// ask user
	printf("What is the local computer name?");
	if (res == YENOERR && hostname)
		printf(" [" YANSI_YELLOW "%s" YANSI_RESET "]", hostname);
	printf("\n" YANSI_BLUE);
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	if (!ys_empty(ys)) {
		ys_free(hostname);
		hostname = ys;
	}
	return (hostname);
}
/* Asks for the local archives path. */
static ystr_t config_ask_archives_path(agent_t *agent) {
	ystr_t ys = NULL;

	printf("Path to the local archives directory? [" YANSI_YELLOW "%s" YANSI_RESET "]\n" YANSI_BLUE, agent->conf.archives_path);
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	if (!ys_empty(ys))
		return (ys);
	ys_free(ys);
	return (ys_copy(A_PATH_ARCHIVES));
}
/* Asks for the log file. */
static ystr_t config_ask_log_file(agent_t *agent) {
	ystr_t ys = NULL;

	printf("Path to the log file? [" YANSI_YELLOW "%s" YANSI_RESET "]\n" YANSI_BLUE, agent->conf.logfile);
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	if (!ys_empty(ys))
		return (ys);
	ys_free(ys);
	return (ys_copy(A_PATH_LOGFILE));
}
/* Asks for syslog. */
static bool config_ask_syslog(void) {
	ystr_t ys = NULL;
	bool result = false;

	// ask for syslog
	while (true) {
		printf("Do you want to send logs to syslog? [" YANSI_YELLOW "y" YANSI_RESET
		       "/" YANSI_YELLOW "N" YANSI_RESET "]\n" YANSI_BLUE);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		if (!ys_empty(ys) && strcmp(ys, "n") && strcmp(ys, "N") && strcmp(ys, "y") && strcmp(ys, "Y")) {
			printf(YANSI_RED "Incorrect value. Try again.\n" YANSI_RESET);
			continue;
		}
		if (!ys_empty(ys) && !strcasecmp(ys, "y"))
			result = true;
		ys_free(ys);
		return (result);
	}
}
/* Asks for the encryption password. */
static ystr_t config_ask_encryption_password(void) {
	ystr_t ys = NULL;

	for (; ; ) {
		printf("Please enter your encryption password. "
		       "It must be at least 24 characters long (40 characters is recommended).\n");
		printf("You can generate a strong password with this command: "
		       YANSI_TEAL "head -c 32 /dev/urandom | base64\n" YANSI_RESET YANSI_BLUE);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		if (ys_bytesize(ys) < A_MINIMUM_CRYPT_PWD_LENGTH) {
			printf(YANSI_RED "Password too short.\n" YANSI_RESET);
			continue;
		}
		return (ys);
	}
}
/* Writes the JSON configuration file. */
static void config_write_json_file(const char *org_key, const char *hostname, const char *archives_path,
                                   const char *logfile, bool syslog, const char *crypt_pwd) {
	yvar_t *config = yvar_new_table(NULL);
	ytable_t *table = yvar_get_table(config);
	ytable_set_key(table, A_JSON_ORG_KEY, yvar_new_const_string(org_key));
	ytable_set_key(table, A_JSON_HOSTNAME, yvar_new_const_string(hostname));
	ytable_set_key(table, A_JSON_ARCHIVES_PATH, yvar_new_const_string(archives_path));
	ytable_set_key(table, A_JSON_LOGFILE, yvar_new_const_string(logfile));
	ytable_set_key(table, A_JSON_SYSLOG, yvar_new_bool(syslog));
	ytable_set_key(table, A_JSON_CRYPT_PWD, yvar_new_const_string(crypt_pwd));
	printf("‣ Writing configuration file " YANSI_PURPLE A_PATH_AGENT_CONFIG YANSI_RESET "... ");
	if (!yfile_touch(A_PATH_AGENT_CONFIG, 0600, 0700) ||
	    yjson_write(A_PATH_AGENT_CONFIG, config, true) != YENOERR) {
		printf(YANSI_RED "failed. Please try again.\n\n" YANSI_RESET);
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
	printf(YANSI_GREEN "done\n" YANSI_RESET);
	ytable_free(table);
}
/* Add the agent execution to the crontab. */
static void config_add_to_crontab(agent_t *agent, config_crontab_t cron_type) {
	ystr_t ys = NULL;

	// search for /etc/cron.hourly directory
	if (cron_type == A_CONFIG_CRON_HOURLY) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_HOURLY_PATH YANSI_RESET ")... ");
		ys = ys_printf(NULL, A_CRONTAB_SCRIPT, agent->agent_path);
		if (yfile_put_string(A_CRON_HOURLY_PATH, ys) &&
		    !chmod(A_CRON_HOURLY_PATH, 0755)) {
			printf(YANSI_GREEN "done\n" YANSI_RESET);
			ys_free(ys);
			return;
		}
		unlink(A_CRON_HOURLY_PATH);
		printf(YANSI_RED "failed. Please try again.\n\n" YANSI_RESET);
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
	// search for /etc/cron.d directory
	if (cron_type == A_CONFIG_CRON_D) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_D_PATH YANSI_RESET ")... ");
		ys = ys_printf(NULL, A_CRONTAB_LINE, agent->agent_path);
		if (yfile_put_string(A_CRON_D_PATH, ys) &&
		    !chmod(A_CRON_D_PATH, 0644)) {
			printf(YANSI_GREEN "done\n" YANSI_RESET);
			ys_free(ys);
			return;
		}
		unlink(A_CRON_D_PATH);
		printf(YANSI_RED "failed. Please try again.\n\n" YANSI_RESET);
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
	// search for /etc/crontab file
	if (cron_type == A_CONFIG_CRON_CRONTAB) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_ETC_PATH YANSI_RESET ")... ");
		ys = ys_printf(NULL, A_CRONTAB_LINE, agent->agent_path);
		if (yfile_contains(A_CRON_ETC_PATH, ys)) {
			printf(YANSI_GREEN "already done\n" YANSI_RESET);
			ys_free(ys);
			return;
		}
		if (yfile_append_string(A_CRON_ETC_PATH, ys)) {
			printf(YANSI_GREEN "done\n" YANSI_RESET);
			ys_free(ys);
			return;
		}
		printf(YANSI_RED "failed. Please try again.\n\n" YANSI_RESET);
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
}

