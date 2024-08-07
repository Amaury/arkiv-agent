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
#define A_ENV_DEBUG_MODE	"debug"
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
/** @const A_JSON_SCRIPTS	JSON key for pre- and post-scripts authorization. */
#define A_JSON_SCRIPTS		"scripts"
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
/** @const A_PARAM_PATH_DB			Path to the list of databases. */
#define A_PARAM_PATH_DB				"/db"
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
/** @const A_PARAM_KEY_STATUS			Key to a status value. */
#define A_PARAM_KEY_STATUS			"s"
/** @const A_PARAM_KEY_SIZE			Key to a file size. */
#define A_PARAM_KEY_SIZE			"sz"

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
/** @const A_DB_STR_MYSQL			MySQL database. */
#define A_DB_STR_MYSQL			"mysql"
/** @const A_DB_STR_PGSQL			PostgreSQL database. */
#define A_DB_STR_PGSQL			"pgsql"
/** @const A_DB_STR_MONGODB			MongoDB database. */
#define A_DB_STR_MONGODB		"mongodb"
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
 * @field	A_DB_MONGODB	MongoDB database.
 */
typedef enum {
	A_DB_MYSQL = 0,
	A_DB_PGSQL
} database_type_t;
/**
 * @typedef	agent_t
 * @abstract	Main structure of the Arkiv agent.
 * @field	exec_timestamp			Unix timestamp of execution start.
 * @field	agent_path			Realpath to the agent program.
 * @field	conf_path			Path to the configuration file.
 * @field	debug_mode			True if the debug mode was set.
 * @field	log_fd				File descriptor to the log file.
 * @field	datetime_chunk_path		Date and time string.
 * @field	backup_path			Real path to the backup directory.
 * @field	backup_files_path		Path to the files backup directory.
 * @field	backup_mysql_path		Path to the MySQL databases backup directory.
 * @field	backup_pgsql_path		Path to the PostgreSQL databases backup directory.
 * @field	backup_mongodb_path		Path to the MongoDB databases backup directory.
 * @field	conf.logfile			Log file's path.
 * @field	conf.archives_path		Root path to the local archives directory.
 * @field	conf.org_key			Organization key.
 * @field	conf.hostname			Hostname.
 * @field	conf.crypt_pwd			Encryption password.
 * @field	conf.scripts_allowed		True is pre- and post-scripts are allowed.
 * @field	conf.use_syslog			True if syslog is used.
 * @field	conf.use_stdout			True when log must be written on STDOUT.
 * @field	conf.use_ansi			False to disable ANSI escape sequences in log messages.
 * @field	bin.tar				Path to the tar program.
 * @field	bin.z				Path to the compression program.
 * @field	bin.crypt			Path to the encryption program.
 * @field	bin.checksum			Path to sha512sum.
 * @field	bin.mysqldump			Path to mysqldump.
 * @field	bin.pg_dump			Path to pg_dump.
 * @field	bin.pg_dumpall			Path to pg_dumpall.
 * @field	bin.mongodump			Path to mongodump.
 * @field	param.org_name			Organization name.
 * @field	param.encryption		Encryption algorithm.
 * @field	param.compression		Compression algorithm.
 * @field	param.local_retention_hours	Number of hours of the local retention.
 * @field	param.retention_type		Type of distant retention.
 * @field	param.retention_duration	Duration of the distant retention.
 * @field	param.savepack_id		Identifier of the used saepack.
 * @field	param.pre_scripts		List of pre-scripts.
 * @field	param.post_scripts		List of post-scripts.
 * @field	param.files			List of files to back up.
 * @field	param.databases			List of databases to back up.
 * @field	param.storage_name		Name of the used storage.
 * @field	param.storage			Associative array of storage parameters.
 * @field	param.storage_env		List of environment variables for the storage setting.
 * @field	exec_log.pre_scripts		List of executed pre-scripts, with a status.
 * @field	exec_log.backup_files		List of backed up files, with a status.
 * @field	exec_log.backup_databases	List of backed up databases, with a status.
 * @field	exec_log.post_scripts		List of executed post-scripts, with a status.
 * @field	exec_log.status_scripts		False is a scripts was asked but they are not locally allowed.
 * @field	exec_log.status_pre_scripts	Status of the pre-scripts execution.
 * @field	exec_log.status_files		Status of the files backup.
 * @field	exec_log.status_databases	Status of the databases backup.
 * @field	exeec_log.status_post_scripts	Status of the post-scripts execution.
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
	ystr_t backup_mongodb_path;
	struct {
		ystr_t logfile;
		ystr_t archives_path;
		ystr_t org_key;
		ystr_t hostname;
		ystr_t crypt_pwd;
		bool scripts_allowed;
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
		ystr_t mongodump;
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
		ytable_t *databases;
		ystr_t storage_name;
		uint64_t storage_id;
		ytable_t *storage;
		yarray_t storage_env;
	} param;
	struct {
		ytable_t *pre_scripts;
		ytable_t *backup_files;
		ytable_t *backup_databases;
		ytable_t *post_scripts;
		bool status_scripts;
		bool status_pre_scripts;
		bool status_files;
		bool status_databases;
		bool status_post_scripts;
	} exec_log;
} agent_t;

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
 * @param	permissive	True to have a permissive behaviour (no exit on error).
 */
void agent_load_configuration(agent_t *agent, bool permissive);
/**
 * @function	agent_free
 * @abstract	Frees a previously created agent structure.
 * @param	agent	Pointer to the allocated structure.
 */
void agent_free(agent_t *agent);

