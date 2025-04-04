/**
 * @header	log.h
 *
 * Definitions used in the Arkiv agent.
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2019-2024, Amaury Bouchard
 */
#pragma once

#include "agent.h"

/** @define ALOG	Add a message to the log file. */
#define ALOG(...)	alog(agent, false, true, __VA_ARGS__)
/** @define ALOG_RAW	Add a message to the log file, without time. */
#define ALOG_RAW(...)	alog(agent, false, false, __VA_ARGS__)
/** @define ADEBUG	Add a debug message to the log file (in debug mode). */
#define ADEBUG(...)	alog(agent, true, true, __VA_ARGS__)
/** @define ADEBUG_RAW	Add a debug message to the log file (in debug mode), without time. */
#define ADEBUG_RAW(...)	alog(agent, true, false, __VA_ARGS__)

/**
 * @typedef	log_script_t
 * @abstract	Structure used to store the log of a script execution.
 * @field	command	The command to execute.
 * @field	success	True if the execution succeed.
 */
typedef struct {
	ystr_t command;
	bool success;
} log_script_t;
/**
 * @typedef	log_item_t
 * @abstract	Structure used to store the log of a (file or database) backup.
 * @field	type		Type of the item (file or database).
 * @field	item		Path to the file or directory to backup, or name of the database to backup.
 * @field	archive_name	Name of the archive file (tar + compress + encrypt'ed file).
 * @field	archive_path	Path to the archive file (tar + compress + encrypt'ed file).
 * @field	archive_size	Size of the archive file, in bytes.
 * @field	checksum_name	Name of the checksum file.
 * @field	checksum_path	Path to the checksum file.
 * @field	success		True if the whole backup succeed.
 * @field	dump_status	Status of the tar or db dump execution.
 * @field	compress_status	Status of the compression.
 * @field	encrypt_status	Status of the encryption.
 * @field	checksum_status	Status of the file's checksum computing.
 * @field	upload_status	Status of the upload.
 */
typedef struct {
	enum {
		A_ITEM_TYPE_FILE = 0,
		A_ITEM_TYPE_DB_MYSQL,
		A_ITEM_TYPE_DB_PGSQL,
		A_ITEM_TYPE_DB_MONGODB
	} type;
	ystr_t item;
	ystr_t archive_name;
	ystr_t archive_path;
	uint64_t archive_size;
	ystr_t checksum_name;
	ystr_t checksum_path;
	bool success;
	ystatus_t dump_status;
	ystatus_t compress_status;
	ystatus_t encrypt_status;
	ystatus_t checksum_status;
	ystatus_t upload_status;
} log_item_t;

/**
 * @function	alog
 *		Write a message in the log file. Use preferably ALOG() and ALOG_RAW().
 * @param	agent		Pointer to the agent structure.
 * @param	debug		True for debug messages.
 * @param	show_time	Print time.
 * @param	str		Message to write.
 * @param	...		Variable arguments.
 */
void alog(agent_t *agent, bool debug, bool show_time, const char *str, ...);
/**
 * @function	log_create_pre_script
 * @abstract	Creates a log entry for a pre-script execution.
 * @param	agent	Pointer to the agent structure.
 * @param	command	The executed command.
 * @return	A pointer to the created log entry.
 */
log_script_t *log_create_pre_script(agent_t *agent, ystr_t command);
/**
 * @function	log_create_post_script
 * @abstract	Creates a log entry for a post-script execution.
 * @param	agent	Pointer to the agent structure.
 * @param	command	The executed command.
 * @return	A pointer to the created log entry.
 */
log_script_t *log_create_post_script(agent_t *agent, ystr_t command);
/**
 * @function	log_create_file
 * @abstract	Creates a log entry for a file backup.
 * @param	agent	Pointer to the agent structure.
 * @param	path	Path of the backed up file.
 * @return	A pointer to the created log entry.
 */
log_item_t *log_create_file(agent_t *agent, ystr_t path);
/**
 * @function	log_create_mysql
 * @abstract	Creates a log entry for a MySQL database backup.
 * @param	agent	Pointer to the agent structure.
 * @param	dbname	Name of the database.
 * @return	A pointer to the created log entry.
 */
log_item_t *log_create_mysql(agent_t *agent, ystr_t dbname);
/**
 * @function	log_create_pgsql
 * @abstract	Creates a log entry for a PostgreSQL database backup.
 * @param	agent	Pointer to the agent structure.
 * @param	dbname	Name of the database.
 * @return	A pointer to the created log entry.
 */
log_item_t *log_create_pgsql(agent_t *agent, ystr_t dbname);

