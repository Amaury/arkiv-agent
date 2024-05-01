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
#define ADEBUG(...)	alog(agent, true, true __VA_ARGS__)
/** @define ADEBUG_RAW	Add a debug message to the log file (in debug mode), without time. */
#define ADEBUG_RAW(...)	alog(agent, true, false, __VA_ARGS__)

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

/**
 * @function	log
 *		Write a message in the log file. Use preferably ALOG() and ALOG_RAW().
 * @param	agent		Pointer to the agent structure.
 * @param	debug		True for debug messages.
 * @param	show_time	Print time.
 * @param	str		Message to write.
 * @param	...		Variable arguments.
 */
void alog(agent_t *agent, bool debug, bool show_time, const char *str, ...);
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

