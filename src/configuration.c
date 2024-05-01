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
#include "configuration.h"

/** @const _ORG_KEY_LENGTH	 	Size of organization keys (45). */
#define _ORG_KEY_LENGTH			4
/** @const _MINIMUM_CRYPTPWD_LENGTH	Minimum size of encryption password. */
#define _MINIMUM_CRYPTPWD_LENGTH	24
/** @const _CRONTAB_SCRIPT		Content of the /etc/cron.hourly/arkiv_agent or /etc/cron.d/arkiv_agent file. */
#define _CRONTAB_SCRIPT			"#!/bin/sh\n\n" \
					"# Arkiv agent hourly execution\n" \
					"# This program backups the local computer, using the Arkiv.sh service\n" \
					"# See https://www.arkiv.sh for more information\n" \
					"/opt/arkiv/bin/agent backup\n"
/** @const _CRONTAB_LINE		Crontab execution line. */
#define _CRONTAB_LINE			"\n# Arkiv agent hourly execution\n" \
					"# This program backups the local computer, using the Arkiv.sh service\n" \
					"# See https://www.arkiv.sh for more information\n" \
					"0 * * * *    root    /opt/arkiv/bin/agent backup\n"

/**
 * @typedef	_config_crontab_t
 * @abstract	Type of cron installation.
 * @field	CONFIG_CRON_HOURLY	/etc/cron.hourly
 * @field	CONFIG_CRON_D		/etc/cron.d
 * @field	CONFIG_CRON_CRONTAB	/etc/crontab
 */
typedef enum {
	CONFIG_CRON_HOURLY,
	CONFIG_CRON_D,
	CONFIG_CRON_CRONTAB
} _config_crontab_t;

/* Checks if the tar program is installed. */
static void _config_check_tar(void);
/* Checks if the sha512sum program is installed. */
static void _config_check_sha512sum(void);
/* Checks if copmression programs are installed. */
static void _config_check_z(void);
/* Checks if encryption programs are installed. */
static void _config_check_crypt(void);
/* Checks if web communication programs are installed. */
static void _config_check_web(void);
/* Checks if a crontab execution can be planned. */
static _config_crontab_t _config_check_cron(void);
/* Asks for the organization key. */
static ystr_t _config_ask_orgkey(void);
/* Asks for the hostname. */
static ystr_t _config_ask_hostname(void);
/* Asks for the local archives path. */
static ystr_t _config_ask_archives_path(agent_t *agent);
/* Asks for the log file. */
static ystr_t _config_ask_log_file(agent_t *agent);
/* Asks for syslog. */
static ystr_t _config_ask_syslog(void);
/* Asks for the encryption password. */
static ystr_t _config_ask_encryption_password(void);
/* Writes the JSON configuration file. */
static void _config_write_json_file(const char *orgKey, const char *hostname, const char *archives_path, const char *syslog, const char *logfile, const char *cryptPwd);
/* Add the agent execution to the crontab. */
static void _config_add_to_crontab(_config_crontab_t cronType);

/* ********** PUBLIC FUNCTIONS ********** */
/* Main function for configuration file generation. */
void exec_configuration(agent_t *agent) {
	_config_crontab_t cronType;
	ystr_t orgKey = NULL;
	ystr_t hostname = NULL;
	ystr_t archives_path = NULL;
	ystr_t logfile = NULL;
	ystr_t syslog = NULL;
	ystr_t cryptPwd = NULL;

	// splashscreen
	printf("\n");
	printf(YANSI_BG_BLUE "%80c" YANSI_RESET "\n", ' ');
	printf(YANSI_BG_BLUE YANSI_WHITE "%28c%s%27c" YANSI_RESET "\n", ' ', "Arkiv agent configuration", ' ');
	printf(YANSI_BG_BLUE "%80c" YANSI_RESET "\n", ' ');
	printf("\n");

	/* programs check */
	// tar
	_config_check_tar();
	// sha512sum
	_config_check_sha512sum();
	// compression programs
	_config_check_z();
	// encryption programs
	_config_check_crypt();
	// web communication programs
	_config_check_web();
	// crontab
	cronType = _config_check_cron();

	/* needed user inputs */
	// ask for the organization key
	orgKey = _config_ask_orgkey();
	// hostname
	hostname = _config_ask_hostname();
	// backup path
	archives_path = _config_ask_archives_path(agent);
	// log file
	logfile = _config_ask_log_file(agent);
	// syslog
	syslog = _config_ask_syslog();
	// encryption password
	printf("\n");
	cryptPwd = _config_ask_encryption_password();
	printf("\n");
	// write JSON file
	_config_write_json_file(orgKey, hostname, archives_path, logfile, syslog, cryptPwd);
	// declare the server to arkiv.sh
	config_declare_server(orgKey, hostname);
	// add agent to crontab
	_config_add_to_crontab(cronType);

	/* cleanup */
	ys_free(hostname);
	ys_free(orgKey);
	ys_free(archives_path);
	ys_free(logfile);
	ys_free(syslog);
	ys_free(cryptPwd);
}
/* Tells if a given program is installed. */
bool config_program_exists(const char *binName) {
	ystr_t ys = config_get_program_path(binName);
	if (!ys)
		return (false);
	ys_free(ys);
	return (true);
}
/* Returns a path to a program. */
ystr_t config_get_program_path(const char *binName) {
	ystr_t path = NULL;

	if (!binName)
		return (NULL);
	// check /bin, /usr/bin, /usr/local/bin
	ys_printf(&path, "/bin/%s", binName);
	if (yfile_is_executable(path))
		return (path);
	ys_printf(&path, "/usr/bin/%s", binName);
	if (yfile_is_executable(path))
		return (path);
	ys_printf(&path, "/usr/local/bin/%s", binName);
	if (yfile_is_executable(path))
		return (path);
	ys_delete(&path);
	// use 'which' program
	if (!yfile_is_executable("/usr/bin/which"))
		return (NULL);
	ybin_t data = {0};
	yarray_t args = yarray_create(1);
	yarray_push(&args, (void*)binName);
	ystatus_t status = yexec("/usr/bin/which", args, NULL, &data, NULL);
	yarray_free(args);
	if (status == YENOERR) {
		path = ys_copy(data.data);
		ys_trim(path);
	}
	if (status != YENOERR || ys_empty(path))
		ys_delete(&path);
	ybin_delete_data(&data);
	return (path);
}
/* Declares the server to arkiv.sh API. */
void config_declare_server(const char *orgKey, const char *hostname) {
	printf("‣ Declare the server to " YANSI_PURPLE "arkiv.sh" YANSI_RESET "... ");
	if (api_server_declare(hostname, orgKey) != YENOERR) {
		printf(
			YANSI_RED "failed\n\n" YANSI_RESET
			YANSI_FAINT "  Check the organization key and try again.\n\n" YANSI_RESET
			YANSI_RED "Abort.\n" YANSI_RESET
		);
		exit(2);
	}
	printf(YANSI_GREEN "done\n" YANSI_RESET);
}

/* ********** STATIC FUNCTIONS ********** */
/**
 * @function	_config_check_tar
 * @abstract	Checks if the tar program is installed. Aborts if not.
 */
static void _config_check_tar(void) {
	if (config_program_exists("tar"))
		return;
	printf(YANSI_RED "Unable to find 'tar' program on this server.\n\n" YANSI_RESET);
	printf("Please, install " YANSI_GOLD "tar" YANSI_RESET " in a standard location ("
	       YANSI_PURPLE "/bin/tar" YANSI_RESET ", " YANSI_PURPLE "/usr/bin/tar" YANSI_RESET " or "
	       YANSI_PURPLE "/usr/local/bin/tar" YANSI_RESET ") and try again.\n");
	printf(YANSI_FAINT "See " YANSI_LINK " for more information.\n\n" YANSI_RESET, "https://doc.arkiv.sh/agent/install", "the documentation");
	printf(YANSI_RED "Abort\n" YANSI_RESET);
	exit(2);
}
/**
 * @function	_config_check_sh256sum
 * @abstract	Checks if the sha512sum program is installed. Aborts if not.
 */
static void _config_check_sha512sum(void) {
	if (config_program_exists("sha512sum"))
		return;
	printf(YANSI_RED "Unable to find 'sha512sum' program on this server.\n\n" YANSI_RESET);
	printf("Please, install " YANSI_GOLD "sha512sum" YANSI_RESET " in a standard location ("
	       YANSI_PURPLE "/bin/sha512sum" YANSI_RESET ", " YANSI_PURPLE "/usr/bin/sha512sum" YANSI_RESET " or "
	       YANSI_PURPLE "/usr/local/bin/sha512sum" YANSI_RESET ") and try again.\n");
	printf(YANSI_FAINT "See " YANSI_LINK " for more information.\n\n" YANSI_RESET, "https://doc.arkiv.sh/agent/install", "the documentation");
	printf(YANSI_RED "Abort\n" YANSI_RESET);
	exit(2);
}
/**
 * @header	_config_check_z
 * @abstract	Checks if compression programs are installed.
 */
static void _config_check_z(void) {
	bool hasGzip = (config_program_exists("gzip") && config_program_exists("gunzip")) ? true : false;
	bool hasBzip2 = (config_program_exists("bzip2") && config_program_exists("bunzip2")) ? true : false;
	bool hasXz = (config_program_exists("xz") && config_program_exists("unxz")) ? true : false;
	bool hasZstd = (config_program_exists("zstd") && config_program_exists("unzstd")) ? true : false;
	if (hasGzip && hasBzip2 && hasXz && hasZstd)
		return;
	if (!hasGzip && !hasBzip2 && !hasXz && !hasZstd) {
		printf(YANSI_RED "Unable to find any supported compression program.\n\n" YANSI_RESET);
		printf("You may install " YANSI_GOLD "gzip" YANSI_RESET ", " YANSI_GOLD "bzip2" YANSI_RESET ", "
		       YANSI_GOLD "xz" YANSI_RESET " or " YANSI_GOLD "zstd" YANSI_RESET " in a standard location ("
		       YANSI_PURPLE "/bin" YANSI_RESET ", " YANSI_PURPLE "/usr/bin" YANSI_RESET " or "
		       YANSI_PURPLE "/usr/local/bin" YANSI_RESET ") or proceed without compression.\n\n");
	} else {
		printf("Here are the compression software installed on this server:\n");
		printf("%s gzip    " YANSI_RESET, (hasGzip ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "(1992) A standard; decent speed and compression ratio\n" YANSI_RESET);
		printf("%s bzip2   " YANSI_RESET, (hasBzip2 ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "(1996) Commonly installed; very good compression level and decent speed\n" YANSI_RESET);
		printf("%s xz      " YANSI_RESET, (hasXz ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "(2009) Not installed everywhere; the best compression ratio, but rather slow\n" YANSI_RESET);
		printf("%s zstd    " YANSI_RESET, (hasZstd ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
		printf(YANSI_FAINT "(2015) Not much installed; good compression level and very high speed\n" YANSI_RESET);
	}
	printf("\nDo you want to continue? [" YANSI_YELLOW "Y" YANSI_RESET "/" YANSI_YELLOW "n" YANSI_RESET "] " YANSI_BLUE);
	ystr_t ys = NULL;
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	bool shouldContinue = (ys_empty(ys) || !strcasecmp(ys, "y") || !strcasecmp(ys, "yes")) ? true : false;
	ys_free(ys);
	if (!shouldContinue) {
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
}
/**
 * @function	_config_check_crypt
 * @abstract	Checks if encryption programs are installed.
 */
static void _config_check_crypt(void) {
	bool hasOpenssl = config_program_exists("openssl");
	bool hasScrypt = config_program_exists("scrypt");
	bool hasGpg = config_program_exists("gpg");
	if (hasOpenssl && hasScrypt && hasGpg)
		return;
	if (!hasOpenssl && !hasScrypt && !hasGpg) {
		printf("\n" YANSI_BG_RED " Unable to find any supported encryption program " YANSI_RESET "\n\n");
		printf("You must install " YANSI_GOLD "openssl" YANSI_RESET ", " YANSI_GOLD "scrypt" YANSI_RESET " or "
		       YANSI_GOLD "gpg" YANSI_RESET " in a standard location (" YANSI_PURPLE "/bin" YANSI_RESET ", "
		       YANSI_PURPLE "/usr/bin" YANSI_RESET " or " YANSI_PURPLE "/usr/local/bin" YANSI_RESET ").\n\n");
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
	printf("Here are the encryption software installed on this server:\n");
	printf("%s openssl  " YANSI_RESET, (hasOpenssl ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
	printf(YANSI_FAINT "Not designed for encrypting large files\n" YANSI_RESET);
	printf("%s scrypt   " YANSI_RESET, (hasScrypt ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
	printf(YANSI_FAINT "Very secure; slow by design\n" YANSI_RESET);
	printf("%s gpg      " YANSI_RESET, (hasGpg ? (YANSI_GREEN "✓ (installed)     ") : (YANSI_RED "✘ (not installled)")));
	printf(YANSI_FAINT "GNU's implementation of the OpenPGP standard\n" YANSI_RESET);
	printf("Do you want to continue? [" YANSI_YELLOW "Y" YANSI_RESET "/" YANSI_YELLOW "n" YANSI_RESET "] " YANSI_BLUE);
	ystr_t ys = NULL;
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	bool shouldContinue = (ys_empty(ys) || !strcasecmp(ys, "y") || !strcasecmp(ys, "yes")) ? true : false;
	ys_free(ys);
	if (!shouldContinue) {
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
}
/**
 * @function	_config_check_web
 * @abstract	Checks if web communication programs are installed.
 */
static void _config_check_web(void) {
	bool hasWget = config_program_exists("wget");
	bool hasCurl = config_program_exists("curl");
	if (hasWget || hasCurl)
		return;
	printf("\n" YANSI_BG_RED " Unable to find any supported web communication program " YANSI_RESET "\n\n");
	printf("You must install " YANSI_GOLD "wget" YANSI_RESET " or " YANSI_GOLD "curl" YANSI_RESET
	       " in a standard location (" YANSI_PURPLE "/bin" YANSI_RESET ", " YANSI_PURPLE "/usr/bin" YANSI_RESET
	       " or " YANSI_PURPLE "/usr/local/bin" YANSI_RESET ").\n\n");
	printf(YANSI_RED "Abort.\n" YANSI_RESET);
	exit(2);
}
/**
 * @function	_config_check_cron
 * @abstract	Checks if a crontab execution can be planned.
 * @return	The type of crontab installation.
 */
static _config_crontab_t _config_check_cron(void) {
	if (yfile_is_dir("/etc/cron.hourly") && yfile_is_writable("/etc/cron.hourly"))
		return (CONFIG_CRON_HOURLY);
	if (yfile_is_dir("/etc/cron.d") && yfile_is_writable("/etc/cron.d"))
		return (CONFIG_CRON_D);
	if (yfile_exists("/etc/crontab") && yfile_is_writable("/etc/crontab"))
		return (CONFIG_CRON_CRONTAB);
	printf("\n" YANSI_BG_RED " Unable to find any writable crontab file " YANSI_RESET "\n\n");
	printf("It should be available under the directories " YANSI_YELLOW "/etc/cron.hourly" YANSI_RESET
	       " or " YANSI_YELLOW "/etc/cron.d" YANSI_RESET ",\nor the file " YANSI_YELLOW "/etc/crontab" YANSI_RESET
	       ".\n\n");
	printf(YANSI_BOLD "Maybe you forgot to execute the agent program as root?\n\n" YANSI_RESET);
	printf(YANSI_RED "Abort.\n" YANSI_RESET);
	exit(2);
}
/**
 * @function	_config_ask_orgkey
 * @abstract	Asks for the organization key.
 * @return	The key. Must be freed.
 */
static ystr_t _config_ask_orgkey(void) {
	ystr_t ys = NULL;

	for (; ; ) {
		printf("Please, enter your organization key (45 characters-long string):\n" YANSI_BLUE);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		size_t keySize = ys_bytesize(ys);
		if (keySize == _ORG_KEY_LENGTH)
			return (ys);
		printf(YANSI_RED "Bad key (should be %d characters long)\n\n" YANSI_RESET, _ORG_KEY_LENGTH);
	}
}
/**
 * @function	_config_ask_hostname
 * @abstract	Asks for the hostname.
 * @return	The hostname.
 */
static ystr_t _config_ask_hostname(void) {
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
/**
 * @function	_config_ask_archives_path
 * @abstract	Asks for the local archives path.
 * @param	agent	Pointer to the agent structure.
 * @return	The archives path.
 */
static ystr_t _config_ask_archives_path(agent_t *agent) {
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
/**
 * @function	_config_ask_log_file
 * @bastract	Asks for the log file.
 * @param	agent	Pointer to the agent structure.
 * @return	The log file's path.
 */
static ystr_t _config_ask_log_file(agent_t *agent) {
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
/**
 * @function	_config_ask_syslog
 * @abstract	Asks for syslog.
 * @return	The syslog facility, or NULL if syslog is not used.
 */
static ystr_t _config_ask_syslog(void) {
	ystr_t ys = NULL;

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
		if (ys_empty(ys) || !strcmp(ys, "n") || !strcmp(ys, "N")) {
			ys_free(ys);
			return (NULL);
		}
		break;
	}
	// ask for syslog facility
	while (true) {
		printf(
			"Which syslog facility do you want to use? ["
			YANSI_YELLOW "USER" YANSI_RESET ", "
			YANSI_YELLOW "LOCAL0" YANSI_RESET " to "
			YANSI_YELLOW "LOCAL7" YANSI_RESET
			"] (press ENTER for USER)\n"
			YANSI_BLUE
		);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		// check answer
		if (!ys_empty(ys) && strcmp(ys, "USER") && strcmp(ys, "LOCAL0") && strcmp(ys, "LOCAL1") &&
		    strcmp(ys, "LOCAL2") && strcmp(ys, "LOCAL3") && strcmp(ys, "LOCAL4") &&
		    strcmp(ys, "LOCAL5") && strcmp(ys, "LOCAL6") && strcmp(ys, "LOCAL7")) {
			printf(YANSI_RED "Incorrect value. Try again.\n" YANSI_RESET);
			continue;
		}
		// manage default value
		if (ys_empty(ys) && ys_append(&ys, "USER") != YENOERR) {
			printf(YANSI_RED "Memory allocation error. Abort.\n" YANSI_RESET);
			exit(2);
		}
		break;
	}
	return (ys);
}
/**
 * @function	_config_ask_encryption_password
 * @abstract	Asks for the encryption password.
 * @return	The encryption password.
 */
static ystr_t _config_ask_encryption_password(void) {
	ystr_t ys = NULL;

	for (; ; ) {
		printf("Please enter your encryption password. "
		       "It must be at least 24 characters long (40 characters is recommended).\n");
		printf("You can generate a strong password with this command: "
		       YANSI_TEAL "head -c 32 /dev/urandom | base64\n" YANSI_RESET YANSI_BLUE);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		if (ys_bytesize(ys) < _MINIMUM_CRYPTPWD_LENGTH) {
			printf(YANSI_RED "Password too short.\n" YANSI_RESET);
			continue;
		}
		return (ys);
	}
}
/**
 * @function	_config_write_json_file
 * @abstract	Writes the JSON configuration file.
 * @param	org_key		Organization key.
 * @param	hostname	Hostname.
 * @param	archives_path	Archives path.
 * @param	logfile		Log file's path.
 * @param	syslog		Syslog facility, or NULL if syslog is not used.
 * @param	crypt_pwd	Encryption password.
 */
static void _config_write_json_file(const char *org_key, const char *hostname, const char *archives_path,
                                    const char *logfile, const char *syslog, const char *crypt_pwd) {
	yvar_t *config = yvar_new_table(NULL);
	ytable_t *table = yvar_get_table(config);
	ytable_set_key(table, A_JSON_ORG_KEY, yvar_new_const_string(org_key));
	ytable_set_key(table, A_JSON_HOSTNAME, yvar_new_const_string(hostname));
	ytable_set_key(table, A_JSON_ARCHIVES_PATH, yvar_new_const_string(archives_path));
	ytable_set_key(table, A_JSON_LOGFILE, yvar_new_const_string(logfile));
	ytable_set_key(table, A_JSON_SYSLOG, syslog ? yvar_new_const_string(syslog) : yvar_new_bool(false));
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
/**
 * @function	_config_add_to_crontab
 * @abstract	Check if the agent execution is already in crontab. If not, add it.
 * @param	cronType	Type of available crontab.
 * @return	YENOERR if the agent execution was in crontab or has been added successfully.
 */
static void _config_add_to_crontab(_config_crontab_t cronType) {
	// search for /etc/cron.hourly directory
	if (cronType == CONFIG_CRON_HOURLY) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_HOURLY_PATH YANSI_RESET ")... ");
		if (yfile_put_string(A_CRON_HOURLY_PATH, _CRONTAB_SCRIPT) &&
		    !chmod(A_CRON_HOURLY_PATH, 0755)) {
			printf(YANSI_GREEN "done\n" YANSI_RESET);
			return;
		}
		unlink(A_CRON_HOURLY_PATH);
		printf(YANSI_RED "failed. Please try again.\n\n" YANSI_RESET);
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
	// search for /etc/cron.d directory
	if (cronType == CONFIG_CRON_D) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_D_PATH YANSI_RESET ")... ");
		if (yfile_put_string(A_CRON_D_PATH, _CRONTAB_LINE) &&
		    !chmod(A_CRON_D_PATH, 0644)) {
			printf(YANSI_GREEN "done\n" YANSI_RESET);
			return;
		}
		unlink(A_CRON_D_PATH);
		printf(YANSI_RED "failed. Please try again.\n\n" YANSI_RESET);
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
	// search for /etc/crontab file
	if (cronType == CONFIG_CRON_CRONTAB) {
		printf("‣ Add to crontab (file " YANSI_PURPLE A_CRON_ETC_PATH YANSI_RESET ")... ");
		if (yfile_contains(A_CRON_ETC_PATH, _CRONTAB_LINE)) {
			printf(YANSI_GREEN "already done\n" YANSI_RESET);
			return;
		}
		if (yfile_append_string(A_CRON_ETC_PATH, _CRONTAB_LINE)) {
			printf(YANSI_GREEN "done\n" YANSI_RESET);
			return;
		}
		printf(YANSI_RED "failed. Please try again.\n\n" YANSI_RESET);
		printf(YANSI_RED "Abort.\n" YANSI_RESET);
		exit(2);
	}
}

