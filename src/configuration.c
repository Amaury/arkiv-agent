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
#include "api.h"
#include "configuration.h"

/* Size of organization keys. */
#define _ORG_KEY_LENGTH			4
/* Default backup path. */
#define	_DEFAULT_BACKUP_PATH		"/var/archives"
/* Minimum size of encryption password. */
#define _MINIMUM_CRYPTPWD_LENGTH	24
/* Destination JSON path. */
#define	_JSON_PATH			"/opt/arkiv/etc/arkiv.json"
/* URL of the server declaration API. */
#define _SERVER_DECLARE_API		"https://api.arkiv.sh/v1/agent/declare"

/* Checks if the tar program is installed. */
static void _config_check_tar(void);
/* Checks if copmression programs are installed. */
static void _config_check_z(void);
/* Checks if encryption programs are installed. */
static void _config_check_crypt(void);
/* Checks if web communication programs are installed. */
static void _config_check_web(void);
/* Asks for the organization key. */
static ystr_t _config_ask_orgkey(void);
/* Asks for the hostname. */
static ystr_t _config_ask_hostname(void);
/* Asks for the backup path. */
static ystr_t _config_ask_backup_path(void);
/* Asks for the encryption password. */
static ystr_t _config_ask_encryption_password(void);
/* Writes the JSON configuration file. */
static bool _config_write_json_file(const char *orgKey, const char *hostname, const char *backupPath, const char *cryptPwd);
/* Declares the server to arkiv.sh API. */
static bool _config_declare_server(const char *orgKey, const char *hostname);
/* Add the agent execution to the crontab. */
static ystatus_t _config_add_to_crontab(void);

/* ********** PUBLIC FUNCTIONS ********** */
/**
 * @function	_config_declare_server
 * @abstract	Declares the server to arkiv.sh API.
 * @param	orgKey		Organization key.
 * @param	hostname	Hostname.
 * @return	True if everything is fine.
 */
bool _config_declare_server(const char *orgKey, const char *hostname) {
	
	printf("Declare the server to " YANSI_PURPLE "arkiv.sh" YANSI_RESET "... ");
	if (api_server_declare(hostname, orgKey) == YENOERR) {
		printf(YANSI_GREEN "done\n" YANSI_RESET);
		return (true);
	}
	printf(YANSI_RED "failed. Please try again.\n" YANSI_RESET);
	return (false);
}
/* Main function for configuration file generation. */
void exec_configuration() {
	ystr_t orgKey = NULL;
	ystr_t hostname = NULL;
	ystr_t backupPath = NULL;
	ystr_t cryptPwd = NULL;

	// splashscreen
	printf("\n");
	printf(YANSI_BG_BLUE "%70c" YANSI_RESET "\n", ' ');
	printf(YANSI_BG_BLUE YANSI_WHITE "%23c%s%22c" YANSI_RESET "\n", ' ', "Arkiv agent configuration", ' ');
	printf(YANSI_BG_BLUE "%70c" YANSI_RESET "\n", ' ');
	printf("\n");

	/* programs check */
	// tar
	_config_check_tar();
	// compression programs
	_config_check_z();
	// encryption programs
	_config_check_crypt();
	// web communication programs
	_config_check_web();

	/* needed user inputs */
	// ask for the organization key
	orgKey = _config_ask_orgkey();
	// hostname
	hostname = _config_ask_hostname();
	// backup path
	backupPath = _config_ask_backup_path();
	// encryption password
	cryptPwd = _config_ask_encryption_password();
	// write JSON file
	if (!_config_write_json_file(orgKey, hostname, backupPath, cryptPwd))
		goto cleanup;
	// declare the server to arkiv.sh
	if (!_config_declare_server(orgKey, hostname))
		goto cleanup;
	// add agent to crontab
	_config_add_to_crontab();
cleanup:
	ys_free(hostname);
	ys_free(orgKey);
	ys_free(backupPath);
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

/* ********** STATIC FUNCTIONS ********** */
/**
 * @function	_config_check_tar
 * @abstract	Checks if the tar program is installed. Aborts if not.
 */
static void _config_check_tar() {
	if (config_program_exists("tar"))
		return;
	printf(YANSI_RED "Unable to find 'tar' program on this server.\n\n" YANSI_RESET);
	printf("Please, install " YANSI_GOLD "tar" YANSI_RESET " in a standard location ("
	       YANSI_PURPLE "/bin/tar" YANSI_RESET ", " YANSI_PURPLE "/usr/bin/tar" YANSI_RESET " or "
	       YANSI_PURPLE "/usr/local/bin/tar" YANSI_RESET ") and try again.\n");
	printf(YANSI_FAINT "See " YANSI_LINK " for more information.\n\n" YANSI_RESET, "https://doc.arkiv.sh/tar", "the documentation");
	printf(YANSI_RED "Abort\n" YANSI_RESET);
	exit(1);
}
/**
 * @header	_config_check_z
 * @abstract	Checks if compression programs are installed.
 */
static void _config_check_z() {
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
static void _config_check_crypt() {
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
		exit(3);
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
static void _config_check_web() {
	bool hasWget = config_program_exists("wget");
	bool hasCurl = config_program_exists("curl");
	if (hasWget || hasCurl)
		return;
	printf("\n" YANSI_BG_RED " Unable to find any supported web communication program " YANSI_RESET "\n\n");
	printf("You must install " YANSI_GOLD "wget" YANSI_RESET " or " YANSI_GOLD "curl" YANSI_RESET
	       " in a standard location (" YANSI_PURPLE "/bin" YANSI_RESET ", " YANSI_PURPLE "/usr/bin" YANSI_RESET
	       " or " YANSI_PURPLE "/usr/local/bin" YANSI_RESET ").\n\n");
	printf(YANSI_RED "Abort.\n" YANSI_RESET);
	exit(4);
}
/**
 * @function	_config_ask_orgkey
 * @abstract	Asks for the organization key.
 * @return	The key. Must be freed.
 */
static ystr_t _config_ask_orgkey() {
	ystr_t ys = NULL;

	for (; ; ) {
		printf("Please, enter your organization key (40 characters-long string):\n" YANSI_BLUE);
		ys_gets(&ys, stdin);
		printf(YANSI_RESET);
		ys_trim(ys);
		if (ys_bytesize(ys) == _ORG_KEY_LENGTH)
			return (ys);
		printf(YANSI_RED "Bad key (should be %d characters long)\n\n" YANSI_RESET, _ORG_KEY_LENGTH);
	}
}
/**
 * @function	_config_ask_hostname
 * @abstract	Asks for the hostname.
 * @return	The hostname.
 */
static ystr_t _config_ask_hostname() {
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
 * @function	_config_ask_backup_path
 * @abstract	Asks for the backup path.
 * @return	The backup path.
 */
static ystr_t _config_ask_backup_path() {
	ystr_t ys = NULL;

	printf("Path to the local backup directory? [" YANSI_YELLOW _DEFAULT_BACKUP_PATH YANSI_RESET "]\n" YANSI_BLUE);
	ys_gets(&ys, stdin);
	printf(YANSI_RESET);
	ys_trim(ys);
	if (!ys_empty(ys))
		return (ys);
	ys_free(ys);
	return (ys_copy(_DEFAULT_BACKUP_PATH));
}
/**
 * @function	_config_ask_encryption_password
 * @abstract	Asks for the encryption password.
 * @return	The encryption password.
 */
static ystr_t _config_ask_encryption_password() {
	ystr_t ys = NULL;

	for (; ; ) {
		printf("\nPlease enter your encryption password. "
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
 * @param	orgKey		Organization key.
 * @param	hostname	Hostname.
 * @param	backupPath	Backup path.
 * @param	cryptPwd	Encryption password.
 * @return	True if everything is fine.
 */
static bool _config_write_json_file(const char *orgKey, const char *hostname, const char *backupPath, const char *cryptPwd) {
	bool res = true;
	yvar_t *config = yvar_new_table(NULL);
	ytable_t *table = yvar_get_table(config);
	ytable_set_key(table, "org_key", yvar_new_const_string(orgKey));
	ytable_set_key(table, "hostname", yvar_new_const_string(hostname));
	ytable_set_key(table, "backup_path", yvar_new_const_string(backupPath));
	ytable_set_key(table, "crypt_pwd", yvar_new_const_string(cryptPwd));
	printf("\nWriting configuration file " YANSI_PURPLE _JSON_PATH YANSI_RESET "... ");
	if (yfile_touch(_JSON_PATH, 0600, 0700) && yjson_write(_JSON_PATH, config, true) == YENOERR)
		printf(YANSI_GREEN "done\n" YANSI_RESET);
	else {
		printf(YANSI_RED "failed. Please try again.\n" YANSI_RESET);
		res = false;
	}
	ytable_free(table);
	return (res);
}
/**
 * @function	_config_add_to_crontab
 * @abstract	Check if the agent execution is already in crontab. If not, add it.
 * @return	YENOERR if the agent execution was in crontab or has been added successfully.
 */
static ystatus_t _config_add_to_crontab(void) {
	return (YENOERR);
}

