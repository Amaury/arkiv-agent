/**
 * @header	agent.h
 *
 * Definitions used in the Arkiv agent.
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2019-2024, Amaury Bouchard
 */
#pragma once

/** @const A_AGENT_VERSION	Version of the agent. */
#define A_AGENT_VERSION	1.0

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
#define A_PATH_AGENT_CONFIG	"/opt/arkiv/etc/arkiv.json"
/** @const A_PATH_PARAMS	Path to the backup parameters file. */
#define A_PATH_BACKUP_PARAMS	"/opt/arkiv/etc/backup.json"
/** @const A_PATH_LOGFILE	Path to the log file. */
#define A_PATH_LOGFILE		"/var/log/arkiv.log"
/** @const A_PATH_EXE		Path to the executable file. */
#define A_EXE_FILE		"/opt/arkiv/bin/agent"

/* ********** CRON CONFIGURATION ********** */
/** @const A_CRON_HOURLY_PATH	Path to the cron.hourly directory. */
#define A_CRON_HOURLY_PATH	"/etc/cron.hourly/arkiv-agent"
/** @const A_CRON_D_PATH	Path to the cron.d directory. */
#define A_CRON_D_PATH		"/etc/cron.d"
/** @const A_CRON_FILENAME	Name of the cron file (in cron.hourly or cron.d directories). */
#define A_CRON_FILENAME		"arkiv_agent"
/** @const A_CRON_TXT		Content of the crontab file. */
#define A_CRON_TXT		"#!/bin/sh\n\n/opt/arkiv/bin/agent backup\n"
/** @const A_CRON_DECLARATION	Definition of a full crontab line. */
#define A_CRON_DECLARATION	"* * * * *	/opt/arkiv/bin/agent backup\n"

/* ********** API URLS ********** */
#ifdef DEV_MODE
	/** @const A_API_URL_PARAMS		API URL used to get server parameters. Parameters: org key, server name. */
	#define A_API_URL_SERVER_PARAMS		"https://conf-dev.arkiv.sh/%s/%s/params.json"
	/** @const A_API_URL_SERVER_DECLARE	API URL for server declaration. */
	#define A_API_URL_SERVER_DECLARE	"https://api.dev.arkiv.sh/v1/server/declare"
	/** @const A_API_URL_BACKUP_REPORT	API URL for backup reporting. */
	#define A_API_URL_BACKUP_REPORT		"https://api.dev.arkiv.sh/v1/backup/report"
#else
	/** @const A_API_URL_PARAMS		API URL used to get server parameters. Parameters: org key, server name. */
	#define A_API_URL_SERVER_PARAMS		"https://conf.arkiv.sh/%s/%s/params.json"
	/** @const A_API_URL_SERVER_DECLARE	API URL for server declaration. */
	#define A_API_URL_SERVER_DECLARE	"https://api.arkiv.sh/v1/server/declare"
	/** @const A_API_URL_BACKUP_REPORT	API URL for backup reporting. */
	#define A_API_URL_BACKUP_REPORT		"https://api.arkiv.sh/v1/backup/report"
#endif // DEV_MODE

/* ********** COMPRESSION ALGORITHMS ********** */
/** @const A_COMPRESS_NONE	Flat tar'ed file without compression. */
#define A_COMPRESS_NONE		'n'
/** @const A_COMPRESS_GZIP	Gzip. */
#define A_COMPRESS_GZIP		'g'
/** @const A_COMPRESS_BZIP2	bzip2. */
#define A_COMPRESS_BZIP2	'b'
/** @const A_COMPRESS_XZ	xz. */
#define A_COMPRESS_XZ		'x'
/** @const A_COMPRESS_ZSTD	zstd. */
#define A_COMPRESS_ZSTD		's'

/* ********** ENCRYPTION METHODS ********** */
/** @const A_CRYPT_OPENSSL	OpenSSL. */
#define A_CRYPT_OPENSSL		'o'
/** @const A_CRYPT_SCRYPT	Scrypt. */
#define A_CRYPT_SCRYPT		's'
/** @const A_CRYPT_GPG		GPG. */
#define A_CRYPT_GPG		'g'

/* ********** LOG ********** */
/** @define ALOG	Add a message to the log file. */
#define ALOG(...)	alog(agent, true, __VA_ARGS__)
/** @define ALOG_RAW	Add a message to the log file, without time. */
#define ALOG_RAW(...)	alog(agent, false, __VA_ARGS__)

/* ********** ERROR STATUS MANAGEMENT ********** */
/** @define AERROR_OVERRIDE	Returns the first status that is not YENOERR. */
#define AERROR_OVERRIDE(a, b)	(((a) != YENOERR) ? (a) : (b))

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

