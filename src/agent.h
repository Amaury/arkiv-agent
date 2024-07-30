/**
 * @header	agent.h
 *
 * Definitions used in the Arkiv agent.
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2019-2024, Amaury Bouchard
 */
#pragma once

#include <stdbool.h>
#include <time.h>
#include <syslog.h>
#include "ystr.h"
#include "yarray.h"
#include "ytable.h"

/** @const A_AGENT_VERSION	Version of the agent. */
#define A_AGENT_VERSION	1.0

/* ********** COMMAND-LINE OPTIONS ********** */
/** @const A_OPT_VERSION	CLI option for version number display. */
#define A_OPT_VERSION		"version"
/** @const A_OPT_CONFIG		CLI option for configuration. */
#define	A_OPT_CONFIG		"config"
/** @const A_OPT_DECLARE	CLI option for declaration. */
#define A_OPT_DECLARE		"declare"
/** @const A_OPT_BACKUP		CLI option for backup. */
#define	A_OPT_BACKUP		"backup"
/** @const A_OPT_RESTORE	CLI option for restore. */
#define	A_OPT_RESTORE		"restore"
/** @const A_ENV_CONF		Environment variable for the configuration file's path. */
#define A_ENV_CONF		"conf"
/** @const A_ENV_LOGFILE	Environment variable for the log file's path. */
#define A_ENV_LOGFILE		"logfile"
/** @const A_ENV_STDOUT		Environment variable for STDOUT usage. */
#define A_ENV_STDOUT		"stdout"
/** @const A_ENV_SYSLOG		Environment variable for syslog usage. */
#define A_ENV_SYSLOG		"syslog"
/** @const A_ENV_DEBUG_MODE	Environment variable for the debug mode. */
#define A_ENV_DEBUG_MODE	"debug_mode"
/** @const A_ENV_ARCHIVES_PATH	Environment variable for the local archives path. */
#define A_ENV_ARCHIVES_PATH	"archives_path"
/** @const A_ENV_CRYPT_PWD	Environment variable for the encryption password. */
#define A_ENV_CRYPT_PWD		"crypt_pwd"
/** @const A_ENV_ANSI		Environment variable for ANSI control characters in log. */
#define A_ENV_ANSI		"ansi"

/* ********** DEFAULT PATHS ************ */
/** @const A_PATH_ROOT		Arkiv root path. */
#define A_PATH_ROOT		"/opt/arkiv"
/** @const A_PATH_BIN		Arkiv bin path. */
#define A_PATH_BIN		"/opt/arkiv/bin"
/** @const A_PATH_ETC		Arkiv configuration path. */
#define A_PATH_ETC		"/opt/arkiv/etc"
/** @const A_PATH_ARCHIVES	Arkiv archives path. */
#define A_PATH_ARCHIVES		"/var/archives"
/** @const A_PATH_AGENT_CONFIG	Path to the configuration file. */
#define A_PATH_AGENT_CONFIG	"/opt/arkiv/etc/agent.json"
/** @const A_PATH_PARAMS	Path to the backup parameters file. */
#define A_PATH_BACKUP_PARAMS	"/opt/arkiv/etc/backup.json"
/** @const A_PATH_LOGFILE	Path to the log file. */
#define A_PATH_LOGFILE		"/var/log/arkiv.log"
/** @const A_PATH_RCLONE	Path to the rclone executable file. */
#define A_EXE_RCLONE		"/opt/arkiv/bin/rclone"

/* ********** CRON CONFIGURATION ********** */
/** @const A_CRON_HOURLY_PATH	Path to the /etc/cron.hourly/arkiv_agent file. */
#define A_CRON_HOURLY_PATH	"/etc/cron.hourly/arkiv_agent"
/** @const A_CRON_D_PATH	Path to the /etc/cron.d/arkiv_agent file. */
#define A_CRON_D_PATH		"/etc/cron.d/arkiv_agent"
/** @const A_CRON_ETC_PATH	Path to the /etc/crontab file. */
#define A_CRON_ETC_PATH		"/etc/crontab"
/** @const A_CRONTAB_SCRIPT	Content of the /etc/cron.hourly/arkiv_agent or /etc/cron.d/arkiv_agent file. */
#define A_CRONTAB_SCRIPT	"#!/bin/sh\n\n" \
				"# Arkiv agent hourly execution.\n" \
				"# This program backups the local computer, using the Arkiv.sh service.\n" \
				"# More information: https://www.arkiv.sh\n\n" \
				"%s backup\n"
/** @const A_CRONTAB_LINE	Crontab execution line. */
#define A_CRONTAB_LINE		"\n# Arkiv agent hourly execution\n" \
				"# This program backups the local computer, using the Arkiv.sh service\n" \
				"# More information: https://www.arkiv.sh\n\n" \
				"0 * * * *    root    %s backup\n"

/* ********** LOGROTATE ********** */
/** @const A_LOGROTATE_CONFIG_PATH	Path to the Arkiv's logorate configuration. */
#define A_LOGROTATE_CONFIG_PATH	"/etc/logrotate.d/arkiv.log"
/** @const A_LOGROTATE_CONFIG_CONTENT	Content of the Arkiv's lorotate configuration. */
#define A_LOGROTATE_CONFIG_CONTENT	"# logrotate configuration for Arkiv.sh agent\n" \
					"%s {\n" \
					"	daily\n" \
					"	rotate 7\n" \
					"	missingok\n" \
					"	compress\n" \
					"	delaycompress\n" \
					"}\n"

/* ********** JSON KEYS ********** */
/** @const A_JSON_ORG_KEY	JSON key for organisation key. */
#define A_JSON_ORG_KEY		"org_key"
/** @const A_JSON_HOSTNAME	JSON key for hostname. */
#define A_JSON_HOSTNAME		"hostname"
/** @const A_JSON_ARCHIVES_PATH	JSON key for archives path. */
#define A_JSON_ARCHIVES_PATH	"archives_path"
/** @const A_JSON_LOGFILE	JSON key for log file. */
#define A_JSON_LOGFILE		"logfile"
/** @const A_JSON_SYSLOG	JSON key for syslog. */
#define A_JSON_SYSLOG		"syslog"
/** @const A_JSON_CRYPT_PWD	JSON key for encryption password. */
#define A_JSON_CRYPT_PWD	"crypt_pwd"

/* ********** SYSLOG STRINGS ********** */
/** @const A_SYSLOG_IDENT	Syslog identity. */
#define A_SYSLOG_IDENT		"arkiv_agent"

/* ********** API URLS ********** */
#ifdef DEV_MODE
	/** @const A_API_URL_PARAMS		API URL used to get server parameters. Parameters: org key, server name. */
	#define A_API_URL_SERVER_PARAMS		"https://conf-dev.arkiv.sh/v1/%s/%s/backup.json"
	/** @const A_API_URL_SERVER_DECLARE	API URL for server declaration. */
	#define A_API_URL_SERVER_DECLARE	"http://api.dev.arkiv.sh/v1/server/declare"
	/** @const A_API_URL_BACKUP_REPORT	API URL for backup reporting. */
	#define A_API_URL_BACKUP_REPORT		"http://api.dev.arkiv.sh/v1/backup/report"
#else
	/** @const A_API_URL_PARAMS		API URL used to get server parameters. Parameters: org key, server name. */
	#define A_API_URL_SERVER_PARAMS		"https://conf.arkiv.sh/v1/%s/%s/backup.json"
	/** @const A_API_URL_SERVER_DECLARE	API URL for server declaration. */
	#define A_API_URL_SERVER_DECLARE	"https://api.arkiv.sh/v1/server/declare"
	/** @const A_API_URL_BACKUP_REPORT	API URL for backup reporting. */
	#define A_API_URL_BACKUP_REPORT		"https://api.arkiv.sh/v1/backup/report"
#endif // DEV_MODE

/* ********** CONFIGURATION VALUES ********** */
/** @const A_ORG_KEY_LENGTH	 	Size of organization keys (45). */
#define A_ORG_KEY_LENGTH		45
/** @const A_MINIMUM_CRYPT_PWD_LENGTH	Minimum size of encryption password. */
#define A_MINIMUM_CRYPT_PWD_LENGTH	24
/** @const A_DEFAULT_LOCAL_RETENTION	Default value for the local retention duration in hours. */
#define A_DEFAULT_LOCAL_RETENTION	24

/* ********** PARAMETERS FILE VARPATH ********** */
/** @const A_PARAM_PATH_RETENTION_HOURS		Path to the local retention duration in hours. */
#define A_PARAM_PATH_RETENTION_HOURS		"/r"
/** @const A_PARAM_PATH_ENCRYPTION_STRING	Path to the encryption string parameter. */
#define A_PARAM_PATH_ENCRYPTION_STRING		"/e"
/** @const A_PARAM_PATH_COMPRESSION_STRING	Path to the compression string parameter. */
#define A_PARAM_PATH_COMPRESSION_STRING		"/z"
/** @const A_PARAM_PATH_SCHEDULES		Path to the schedules parameter. */
#define A_PARAM_PATH_SCHEDULES			"/sch"
/** @const A_PARAM_PATH_SCHEDULE_NAME		Path to the schedule name. */
#define A_PARAM_PATH_SCHEDULE_NAME		"/schn"
/** @const A_PARAM_PATH_RETENTION_TYPE		Path to the retention type. */
#define A_PARAM_PATH_RETENTION_TYPE		"/rt"
/** @const A_PARAM_PATH_RETENTION_DURATION	Path to the retention duration. */
#define A_PARAM_PATH_RETENTION_DURATION		"/rd"
/** @const A_PARAM_PATH_STORAGES		Path to the storages parameter. */
#define A_PARAM_PATH_STORAGES			"/st"
/** @const A_PARAM_PATH_SAVEPACKS		Path to the savepacks parameter. */
#define A_PARAM_PATH_SAVEPACKS			"/sp"
/** @const A_PARAM_PATH_NAME			Path to a name element. */
#define A_PARAM_PATH_NAME			"/n"
/** @const A_PARAm_PATH_PRE			Path to pre-scripts parameter. */
#define A_PARAM_PATH_PRE			"/pre"
/** @const A_PARAM_PATH_POST			Path to post-scripts parameter. */
#define A_PARAM_PATH_POST			"/post"
/** @const A_PARAM_PATH_FILE			Path to the list of files. */
#define A_PARAM_PATH_FILE			"/file"
/** @const A_PARAM_PATH_MYSQL			Path to the list of MySQL databases. */
#define A_PARAM_PATH_MYSQL			"/mysql"
/** @const A_PARAM_PATH_PGSQL			Path to the list of PostgreSQL databases. */
#define A_PARAM_PATH_PGSQL			"/pgsql"
/** @const A_PARAM_KEY_TYPE			Key to a type element. */
#define A_PARAM_KEY_TYPE			"t"
/** @const A_PARAM_KEY_ACCESS_KEY		Key to an access key element. */
#define A_PARAM_KEY_ACCESS_KEY			"ac"
/** @const A_PARAM_KEY_SECRET_KEY		Key to a secret key element. */
#define A_PARAM_KEY_SECRET_KEY			"se"
/** @const A_PARAM_KEY_REGION			Key to a region element. */
#define A_PARAM_KEY_REGION			"re"
/** @const A_PARAM_KEY_BUCKET			Key to a bucket element. */
#define A_PARAM_KEY_BUCKET			"bu"
/** @const A_PARAM_KEY_PATH			Key to a path element. */
#define A_PARAM_KEY_PATH			"pa"
/** @const A_PARAM_KEY_HOST			Key to a host element. */
#define A_PARAM_KEY_HOST			"ho"
/** @const A_PARAM_KEY_PORT			Key to a port element. */
#define A_PARAM_KEY_PORT			"po"
/** @const A_PARAM_KEY_USER			Key to an user element. */
#define A_PARAM_KEY_USER			"us"
/** @const A_PARAM_KEY_PWD			Key to a password element. */
#define A_PARAM_KEY_PWD				"pw"
/** @const A_PARAM_KEY_KEYFILE			Key to a key file. */
#define A_PARAM_KEY_KEYFILE			"ke"
/** @const A_PARAM_KEY_DB			Key to a database name. */
#define A_PARAM_KEY_DB				"db"

/* ********** ENCRYPTION METHOD PARAM CHARACTERS ********** */
/** @const A_CRYPT_OPENSSL	OpenSSL. */
#define A_CHAR_CRYPT_OPENSSL	'o'
/** @const A_CRYPT_SCRYPT	Scrypt. */
#define A_CHAR_CRYPT_SCRYPT	's'
/** @const A_CRYPT_GPG		GPG. */
#define A_CHAR_CRYPT_GPG	'g'

/* ********** COMPRESSION ALGORITHM PARAM CHARACTERS ********** */
/** @const A_COMPRESS_NONE	Flat tar'ed file without compression. */
#define A_CHAR_COMP_NONE	'n'
/** @const A_COMPRESS_GZIP	Gzip. */
#define A_CHAR_COMP_GZIP	'g'
/** @const A_COMPRESS_BZIP2	bzip2. */
#define A_CHAR_COMP_BZIP2	'b'
/** @const A_COMPRESS_XZ	xz. */
#define A_CHAR_COMP_XZ		'x'
/** @const A_COMPRESS_ZSTD	zstd. */
#define A_CHAR_COMP_ZSTD	's'

/* ********** STORAGE TYPES ********** */
/** @const A_STORAGE_TYPE_AWS_S3	Storage type for AWS S3. */
#define A_STORAGE_TYPE_AWS_S3		"aws_s3"
/** @const A_STORAGE_TYPE_SFTP		Storage type for SFTP. */
#define A_STORAGE_TYPE_SFTP		"sftp"

/* ********** DATABASE MACROS ********** */
/** @const A_DB_ALL_DATABASES_DEFINITION	All databases. */
#define A_DB_ALL_DATABASES_DEFINITION	"*"
/** @const A_DB_ALL_DATABASES_FILENAME		Name of the dumpfile for all databases. */
#define A_DB_ALL_DATABASES_FILENAME	"__all_databases__"

/* ********** WEB PROGAMS ********** */
/**
 * @typedef	web_program_t
 * @abstract	Definition of the usable web program.
 * @field	A_WEB_CURL	Curl.
 * @field	A_WEB_WGET	Wget.
 */
typedef enum {
	A_WEB_CURL,
	A_WEB_WGET
} web_program_t;

/* ********** ERROR STATUS MANAGEMENT ********** */
/** @define AERROR_OVERRIDE	Returns the first status that is not YENOERR. */
#define AERROR_OVERRIDE(a, b)	(((a) != YENOERR) ? (a) : (b))

/* ********** TYPE DEFINITIONS ********** */
/**
 * @typedef	encrypt_type_t
 * @abstract	Defines a type of encryption.
 * @field	A_CRYPT_UNDEF	Undefined encryption.
 * @field	A_CRYPT_OPENSSL	OpenSSL.
 * @field	A_CRYPT_SCRYPT	scrypt.
 * @field	A_CRYPT_GPG	Gnu Privacy Guard.
 */
typedef enum {
	A_CRYPT_UNDEF = 0,
	A_CRYPT_OPENSSL,
	A_CRYPT_SCRYPT,
	A_CRYPT_GPG
} encrypt_type_t;
/**
 * @typedef	compress_type_t
 * @abstract	Defines a type of compression.
 * @field	A_COMP_NONE	No compression.
 * @field	A_COMP_GZIP	gzip.
 * @field	A_COMP_BZIP2	bzip2.
 * @field	A_COMP_XZ	xz.
 * @field	A_COMP_ZSTD	zstd.
 */
typedef enum {
	A_COMP_NONE = 0,
	A_COMP_GZIP,
	A_COMP_BZIP2,
	A_COMP_XZ,
	A_COMP_ZSTD
} compress_type_t;
/**
 * @typedef	retention_type_t
 * @abstract	Defines a type of retention.
 * @field	A_RETENTION_INFINITE	Infinite retention.
 * @field	A_RETENTION_DAYS	Retention in days.
 * @field	A_RETENTION_WEEKS	Retention in weeks.
 * @field	A_RETENTION_MONTHS	Retention in months.
 * @field	A_RETENTION_YEARS	Retention in years.
 */
typedef enum {
	A_RETENTION_INFINITE = 0,
	A_RETENTION_DAYS,
	A_RETENTION_WEEKS,
	A_RETENTION_MONTHS,
	A_RETENTION_YEARS
} retention_type_t;
/**
 * @typedef	database_type_t
 * @abstract	Defines a type of database.
 * @field	A_DB_MYSQL	MySQL database.
 * @field	A_DB_PGSQL	PostgreSQL database.
 */
typedef enum {
	A_DB_MYSQL = 0,
	A_DB_PGSQL
} database_type_t;
/**
 * @typedef	agent_t
 * @abstract	Main structure of the Arkiv agent.
 * @field	exec_timestamp		Unix timestamp of execution start.
 * @field	agent_path		Realpath to the agent program.
 * @field	conf_path		Path to the configuration file.
 * @field	debug_mode		True if the debug mode was set.
 * @field	log_fd			File descriptor to the log file.
 * @field	datetime_chunk_path	Date and time string.
 * @field	backup_path		Real path to the backup directory.
 * @field	backup_files_path	Path to the files backup directory.
 * @field	backup_mysql_path	Path to the MySQL databases backup directory.
 * @field	backup_pgsql_path	Path to the PostgreSQL databases backup directory.
 * @field	conf.logfile		Log file's path.
 * @field	conf.archives_path	Root path to the local archives directory.
 * @field	conf.org_key		Organization key.
 * @field	conf.hostname		Hostname.
 * @field	conf.crypt_pwd		Encryption password.
 * @field	conf.use_syslog		True if syslog is used.
 * @field	conf.use_stdout		True when log must be written on STDOUT.
 * @field	conf.use_ansi		False to disable ANSI escape sequences in log messages.
 * @field	bin.tar			Path to the tar program.
 * @field	bin.z			Path to the compression program.
 * @field	bin.crypt		Path to the encryption program.
 * @field	bin.checksum		Path to sha512sum.
 * @field	bin.mysqldump		Path to mysqldump.
 * @field	bin.pg_dump		Path to pg_dump.
 * @field	bin.pg_dumpall		Path to pg_dumpall.
 * @field	param.org_name		Organization name.
 * @field	param.encryption	Encryption algorithm.
 * @field	param.compression	Compression algorithm.
 * @field	local_retention_hours	Number of hours of the local retention.
 * @field	retention_type		Type of distant retention.
 * @field	retention_duration	Duration of the distant retention.
 * @field	savepack_id		Identifier of the used saepack.
 * @field	param.pre_scripts	List of pre-scripts.
 * @field	param.post_scripts	List of post-scripts.
 * @field	param.files		List of files to back up.
 * @field	param.mysql		List of MySQL databases to back up.
 * @field	param.pgsql		List of PostgreSQL databases to back up.
 * @field	param.storage_name	Name of the used storage.
 * @field	param.storage		Associative array of storage parameters.
 * @field	param.storage_env	List of environment variables for the storage setting.
 * @field	log.status		Global execution status.
 * @field	log.backup_files	List of backed up files, with a status.
 * @field	log.backup_mysql	List of backed up MySQL databases, with a status.
 * @field	log.backup_pgsql	List of backed up PostgreSQL databases, with a status.
 * @field	log.upload_s3		List of S3 uploads, with a status.
 */
typedef struct agent_s {
	time_t exec_timestamp;
	char *agent_path;
	ystr_t conf_path;
	bool debug_mode;
	FILE *log_fd;
	ystr_t datetime_chunk_path;
	ystr_t backup_path;
	ystr_t backup_files_path;
	ystr_t backup_mysql_path;
	ystr_t backup_pgsql_path;
	struct {
		ystr_t logfile;
		ystr_t archives_path;
		ystr_t org_key;
		ystr_t hostname;
		ystr_t crypt_pwd;
		bool use_syslog;
		bool use_stdout;
		bool use_ansi;
	} conf;
	struct {
		ystr_t find;
		ystr_t tar;
		ystr_t z;
		ystr_t crypt;
		ystr_t checksum;
		ystr_t mysqldump;
		ystr_t pg_dump;
		ystr_t pg_dumpall;
	} bin;
	struct {
		ystr_t org_name;
		encrypt_type_t encryption;
		compress_type_t compression;
		uint16_t local_retention_hours;
		retention_type_t retention_type;
		uint8_t retention_duration;
		uint64_t savepack_id;
		ytable_t *pre_scripts;
		ytable_t *post_scripts;
		ytable_t *files;
		ytable_t *mysql;
		ytable_t *pgsql;
		ystr_t storage_name;
		uint64_t storage_id;
		ytable_t *storage;
		yarray_t storage_env;
	} param;
	struct {
		ytable_t *pre_scripts;
		ytable_t *backup_files;
		ytable_t *backup_mysql;
		ytable_t *backup_pgsql;
		ytable_t *post_scripts;
		bool status_pre_scripts;
		bool status_files;
		bool status_databases;
		bool status_post_scripts;
	} exec_log;
} agent_t;

#if 0
/* ********** STRUCTURES ************ */
/**
 * @typedef	agent_t
 * 		Main structure of the Arkiv agent.
 * @field	logfile			Pointer to the log file.
 * @field	debug_mode		True if the debug mode was set.
 * @field	backup_path		Root path of the archive directory.
 * @field	conf.hostname		Host name.
 * @field	conf.public_key		User's public key on Arkiv.sh.
 * @field	conf.private_key	User's private key on Arkiv.sh.
 * @field	conf.crypt_passphrase	Passphrase for encryption.
 * @field	conf.crypt_keyflie	Path to the symmetric key file.
 * @field	conf.encryption		Encryption configuration.
 * @field	conf.backup_id		Identifier of the backup.
 * @field	conf.comp		Compression method.
 * @field	bin_path.mysqldump	Path to mysqldump.
 * @field	bin_path.xtrabackup	Path to xtrabackup.
 * @field	bin_path.pg_dump	Path to pg_dump.
 * @field	bin_path.awscli		Path to awscli.
 * @field	path			List of path to backup.
 * @field	mysqldump.active	Activation of MySQL dump.
 * @field	mysqldump.host		MySQL host.
 * @field	mysqldump.user		MySQL user login.
 * @field	mysqldump.pwd		MySQL password.
 * @field	mysqldump.all_db	True if all databases must be backed up.
 * @field	mysqldump.db		List of databases to backup.
 * @field	xtrabackup.active	Activation of Xtrabackup.
 * @field	xtrabackup.host		Xtrabackup host.
 * @field	xtrabackup.user		Xtrabackup user.
 * @field	xtrabackup.pwd		Xtrabackup password.
 * @field	pgdump.active		Activation of pgdump.
 * @field	pgdump.host		MySQL host.
 * @field	pgdump.user		MySQL user login.
 * @field	pgdump.pwd		MySQL password.
 * @field	pgdump.all_db		True if all databases must be backed up.
 * @field	pgdump.db		List of databases to backup.
 * @field	aws.public_key		AWS public key.
 * @field	aws.private_key		AWS private key.
 * @field	aws.bucket		Name of the S3 bucket.
 * @field	aws.region		Region of the S3 bucket.
 * @field	aws.deep_archive	True if the files uploaded to S3 must have the
 *					DEEP_ARCHIVE storage class.
 * @field	aws.glacier_vault	Name of the Glacier vault.
 * @field	log.status		Global execution status.
 * @field	log.backup_files	List of backed up files, with a status.
 * @field	log.backup_databases	List of backup up databases, with a status.
 * @field	log.upload_s3		List of S3 uploads, with a status.
 */
typedef struct agent_s {
	FILE *logfile;
	bool debug_mode;
	ystr_t backup_path;
	struct {
		const char *hostname;
		const char *public_key;
		const char *private_key;
		const char *crypt_passphrase;
		const char *crypt_keyfile;
		enum {
			ENCRYPT_NO,
			ENCRYPT_OPT,
			ENCRYPT_MANDATORY
		} encryption;
		char *backup_id;
		char comp;
	} conf;
	struct {
		const char *mysqldump;
		const char *xtrabackup;
		const char *pg_dump;
		const char *pg_dumpall;
		const char *awscli;
	} bin_path;
	yarray_t path;
	struct {
		bool active;
		char *host;
		char *user;
		char *pwd;
		bool all_db;
		yarray_t db;
	} mysqldump;
	struct {
		bool active;
		char *host;
		char *user;
		char *pwd;
	} xtrabackup;
	struct {
		bool active;
		char *host;
		char *user;
		char *pwd;
		bool all_db;
		yarray_t db;
	} pg_dump;
	struct {
		char *public_key;
		char *private_key;
		char *bucket;
		char *region;
		bool deep_archive;
	} aws;
	struct {
		ystatus_t status;
		yarray_t backup_files;
		yarray_t backup_databases;
		yarray_t upload_s3;
	} log;
} agent_t;

/**
 * @typedef	log_item_t
 *		Structure used to store an element (a string) with an associated
 *		status. The string is allocated and thus must be freed.
 * @field	orig_path	Path to the original file.
 *				File: Path to the backed up file.
 *				Database: Name of the backed up database.
 *				S3: Path of the local file copied to S3.
 *				Glacier: Path of the local file copied to Glacier.
 * @field	result_path	Path to the result file.
 *				File: Path to the tar'ed archive.
 *				Database: Path to the tar'ed archive.
 *				S3: Distant path of the file on S3.
 *				Glacier: Distant identifier.
 * @field	status		The global backup status.
 * @field	compress_status	Compression status.
 * @field	encrypt_status	Encryption status.
 */
typedef struct {
	char *orig_path;
	char *result_path;
	ystatus_t status;
	ystatus_t compress_status;
	ystatus_t encrypt_status;
} log_item_t;
#endif

/**
 * @function	agent_new
 * @abstract	Creates a new agent structure.
 * @param	exe_path	Executable path.
 * @return	The allocated and initialized agent.
 */
agent_t *agent_new(char *exe_path);
/**
 * @function	agent_getenv
 * @abstract	Returns a copy of an environment variable, or a default value.
 * @param	envvar		Name of the environment variable.
 * @param	default_value	The default value returned if the environment variable is not found.
 * @return	The value.
 */
ystr_t agent_getenv(char *envvar, ystr_t default_value);
/**
 * @function	agent_getenv_static
 * @abstract	Returns a copy of an environment varialbe, or a copy of a default value.
 * @param	envvar		Name of the environment variable.
 * @param	default_value	The default value, copied if the environment variable is not found.
 * @return	The value.
 */
ystr_t agent_getenv_static(char *envvar, const char *default_value);
/**
 * @function	agent_load_configuration
 * @abstract	Reads the configuration file.
 * @param	agent	Pointer to the agent structure.
 */
void agent_load_configuration(agent_t *agent);
/**
 * @function	agent_free
 * @abstract	Frees a previously created agent structure.
 * @param	agent	Pointer to the allocated structure.
 */
void agent_free(agent_t *agent);

#if 0
/* ********** PUBLIC FUNCTIONS ********** */
/* ---------- backup.c ---------- */
/**
 * @function	backup_files
 *		Backup files.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if OK.
 */
ystatus_t backup_files(agent_t *agent);
/**
 * @function	backup_database_mysqldump
 *		Backup MySQL databases using mysqldump.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if OK.
 */
ystatus_t backup_database_mysqldump(agent_t *agent);
/**
 * @function	backup_database_xtrabackup
 *		Backup MySQL databases using xtrabackup.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if OK.
 */
ystatus_t backup_database_xtrabackup(agent_t *agent);
/**
 * @function	backup_database_pgdump
 *		Backup PostgreSQL databases using pg_dump.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if OK.
 */
ystatus_t backup_database_pgdump(agent_t *agent);

/* ---------- upload.c ---------- */
/**
 * @function	upload_aws_s3
 *		Upload all backed up files to Amazon S3.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if OK.
 */
ystatus_t upload_aws_s3(agent_t *agent);

/* ---------- install.c ---------- */
/**
 * @function	install
 *		Start the installation process.
 * @param	exe_path	Path of the current program.
 * @return	0 if OK, 1 if something went wrong.
 */
int install(char *exe_path);

/* ---------- init.c ---------- */
/**
 * @function	init_log
 *		Initialize the log file.
 * @param	agent	Pointer to the agent structure.
 */
void init_log(agent_t *agent);
/**
 * @function	init_check_debug
 *		Check if the debug option was defined. If yes, activate
 *		the debug mode.
 * @param	agent	Pointer to the agent structure.
 * @param	argc	Program's arguments count.
 * @param	argv	Program's arguments list.
 */
void init_check_debug(agent_t *agent, int argc, char *argv[]);
/**
 * @function	init_check_mandatory_encryption
 *		Check if encryption is mandatory but impossible to do.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if encryption is not mandatory, or if it's mandatory and
 *		possible.
 */
ystatus_t init_check_mandatory_encryption(agent_t *agent);
/**
 * @function	init_getenv
 *		Returns the value of an environment variable if it's defined, or
 *		a default value if it's not defined.
 * @param	envvar		Name of the environment variable.
 * @param	default_value	Default value.
 * @return	The value or NULL if nothing is suitable.
 */
char *init_getenv(char *envvar, char *default_value);
/**
 * @function	init_read_conf
 *		Read local configuration.
 * @param	agent	Pointer to the agent structure.
 * @param	argc	Program's arguments count.
 * @param	argv	Program's arguments list.
 * @return	YENOERR if OK.
 */
ystatus_t init_read_conf(agent_t *agent, int argc, char *argv[]);
/**
 * @function	init_fetch_params
 *		Read distant parameters.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if OK.
 */
ystatus_t init_fetch_params(agent_t *agent);
/**
 * @function	init_create_output_directory
 *		Create the output directory.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if OK.
 */
ystatus_t init_create_output_directory(agent_t *agent);

/* ---------- compress.c ---------- */
/**
 * @function	compress_create_archive
 *		Create an archive.
 * @param	agent		Pointer to the agent structure.
 * @param	src		Path to the source.
 * @param	dst		Path to the destination.
 * @param	algo		Algorithm to use for compression.
 *				t=tar, g=gzip, b=bzip2, x=xz
 * @param	archive_path	Pointer to a buffer that will be filled with the
 *				compressed file's path.
 * @return	YENOERR if OK.
 */
ystatus_t compress_create_archive(agent_t *agent, const char *src, const char *dst,
                                  char algo, char *archive_path);
/**
 * @function	compress_file
 *		Create a compressed file.
 * @param	agent		Pointer to the agent structure.
 * @param	path		Path to the source file. The string will be modified
 *				to add the compressed file's extension at the end.
 * @param	algo		Algorithm to use for compression.
 *				t=tar, g=gzip, b=bzip2, x=xz
 * @param	compressed_path	Pointer to a buffer that will be filled with the
 *				the compressed file's path.
 * @return	YENOERR if OK.
 */
ystatus_t compress_file(agent_t *agent, const char *path, char algo,
                        char *compressed_path);

/* ---------- encrypt.c ---------- */
/**
 * @function	encrypt_file
 *		Encrypt a file.
 * @param	agent		Pointer to the agent structure.
 * @param	path		Path to the file to encrypt.
 * @param	encrypted_path	Pointer to a buffer that will be filled with the
 *				encrypted file's path.
 * @return	YENOERR if OK.
 */
ystatus_t encrypt_file(agent_t *agent, const char *path, char *encrypted_path);

/* ---------- sha256.c ---------- */
/**
 * @function	backup_create_sha256sum
 *		Create sha256sum hash file.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if OK.
 */
ystatus_t sha256sum_create(agent_t *agent);

/* ---------- log.c ---------- */
/**
 * @function	log
 *		Write a message in the log file. Use preferably ALOG() and ALOG_RAW().
 * @param	agent		Pointer to the agent structure.
 * @param	show_time	Print time.
 * @param	str		Message to write.
 * @param	...		Variable arguments.
 */
void alog(agent_t *agent, bool show_time, const char *str, ...);
/**
 * @function	log_file_backup
 *		Add a log item entry for a backed up file.
 * @param	agent		Pointer to the agent structure.
 * @param	filename	Name of the backed up file (copied).
 * @param	tarname		Path to the tar'ed file (copied).
 * @param	status		Status of the backup.
 * @param	compress_status	Compression status.
 * @param	encrypt_status	Encryption status.
 */
void log_file_backup(agent_t *agent, char *filename, char *tarname, ystatus_t status,
                     ystatus_t compress_status, ystatus_t encrypt_status);
/**
 * @function	log_database_backup
 *		Add a log item entry for a backed up database.
 * @param	agent	Pointer to the agent structure.
 * @param	db	Name of the backed up database (copied).
 * @param	tarname	Path to the tar'ed file (copied).
 * @param	status	Status of the backup.
 * @param	compress_status	Compression status.
 * @param	encrypt_status	Encryption status.
 */
void log_database_backup(agent_t *agent, char *db, char *tarname, ystatus_t status,
                         ystatus_t compress_status, ystatus_t encrypt_status);
/**
 * @function	log_s3_upload
 *		Add a log item entry for a S3 upload.
 * @param	agent	Pointer to the agent structure.
 * @param	local	Path of the local file (copied).
 * @param	distant	Path of the file on S3 (copied).
 * @param	status	Status of the upload.
 */
void log_s3_upload(agent_t *agent, char *local, char *distant, ystatus_t status);

/* ---------- utils.c ---------- */
/**
 * @function	str_remove_substring
 *		Remove all occurences of a substring inside a string.
 * @param	str	The source string.
 * @param	substr	The substring to remove.
 */
void str_remove_substring(char *str, const char *substr);
/**
 * @function	copy_file
 *		Copy a file to the given location.
 * @param	from	Path of the source file.
 * @param	to	Path of the destination file.
 * @return	YENOERR if OK.
 */
ystatus_t copy_file(const char *from, const char *to);
/**
 * @function	create_empty_file
 *		Create an empty file (or truncate it if it already exists) and set
 *		0600 mode.
 * @param	path	Path to the file.
 * @return	YENOERR if OK.
 */
ystatus_t create_empty_file(const char *path);
/**
 * @function	create_directory
 *		Create a directory (if it doesn't exists) and set 0700 mode.
 * @param	path	Path to the directory.
 * @return	YENOERR if OK.
 */
ystatus_t create_directory(const char *path);
/**
 * @function	bin2hex
 *		Generate an hexadecimal string from a binary input.
 * @param	input		Pointer to binary data.
 * @param	input_len	Length of binary data.
 * @param	output		Pointer to a string, long enough to get the result.
 *				If NULL, a new string will be allocated and returned.
 * @return	A pointer to the output, or NULL if an error occured.
 */
char *bin2hex(const void *input, size_t input_len, char *output);
/**
 * @function	filenamize
 *		Create a string from another one, suitable to be a file name.
 * @param	input	Input string.
 * @return	An allocated string.
 */
char *filenamize(const char *input);
/**
 * @function	Execute a sub-program and wait for its termination.
 * @param	command	Path to the sub-program to execute.
 * @param	args		List of arguments.
 * @param	env		List of environment variables.
 * @param	out_memory	Pointer to a ybin_t that will be filled with the
 *				standard output. The string is allocated, thus must
 *				be freed. Could be set to NULL.
 * @param	out_file	Path to a file where the data will be written.
 *				Could be nulL.
 * @return	YENOERR if OK.
 */
ystatus_t exec_wait(const char *command, yarray_t args, yarray_t env,
                    ybin_t *out_memory, const char *out_file);
/**
 * @function	check_root_user
 *		Check if the current user is root.
 * @return	YENOERR if OK.
 */
ystatus_t check_root_user(void);
/**
 * @function	callback_free_raw
 *		Free a pointer stored in a yvector.
 * @param	ptr	Pointer that must be freed.
 * @param	data	Pointer to unused data.
 */
void callback_free_raw(void *ptr, void *data);
/**
 * @function	callback_free_log_item
 *		Free a log item stored in a yvector.
 * @param	ptr	Pointer to the log item.
 * @param	data	Pointer to unused data.
 */
void callback_free_log_item(void *ptr, void *data);

#endif // 0

