/**
 * @header	backup.h
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2024, Amaury Bouchard
 */
#pragma once

#include "yjson.h"
#include "agent.h"

/**
 * @function	exec_backup
 * @abstract	Main backup function.
 * @param	agent	Pointer to the agent structure.
 */
void exec_backup(agent_t *agent);

/* ********** PRIVATE DECLARATIONS ********** */
#ifdef __A_BACKUP_PRIVATE__
	/**
	 * @typedef	script_type_t
	 * @abstract	Type of script (pre or post).
	 * @constant	A_SCRIPT_TYPE_PRE	Pre-script.
	 * @constant	A_SCRIPT_TYPE_POST	Post-script.
	 */
	typedef enum {
		A_SCRIPT_TYPE_PRE = 0,
		A_SCRIPT_TYPE_POST
	} script_type_t;

	/**
	 * @function	backup_purge_local
	 * @abstract	Purge local archive files.
	 * @param	agent	Pointer to the agent structure.
	 * @return	YENOERR if everything went fine.
	 */
	static ystatus_t backup_purge_local(agent_t *agent);
	/**
	 * @function	backup_fetch_param
	 * @abstract	Fetch and process host backup parameter file.
	 * @param	agent	Pointer to the agent structure.
	 * @return	YENOERR if everything went fine;
	 *		YEAGAIN if no backup is schedule for the current execution time;
	 *		other status if an error occurred.
	 */
	static ystatus_t backup_fetch_params(agent_t *agent);
	/**
	 * @function	backup_create_output_directory
	 * @abstract	Create output directory.
	 * @param	agent	Pointer to the agent structure.
	 * @return	YENOERR if everything went fine.
	 */
	static ystatus_t backup_create_output_directory(agent_t *agent);
	/**
	 * @function	backup_exec_scripts
	 * @abstract	Execute each script in the list of pre- or post-scripts.
	 * @param	agent	Pointer to the agent structure.
	 * @param	type	Type of scripts to execute.
	 * @return	YENOERR if everything went fine.
	 */
	static ystatus_t backup_exec_scripts(agent_t *agent, script_type_t type);
	/**
	 * @function	backup_exec_pre_script
	 * @abstract	Execute a pre-script.
	 * @param	hash		Index in the list of scripts.
	 * @param	key		Always null.
	 * @param	data		Path to the script to execute.
	 * @param	user_data	Pointer to the agent structure.
	 * @return	YENOERR if the script exited withhout error (returned value == 0),
	 *		YEBADCONF if the configuration is not valid,
	 *		YEFAULT if the script exited with an error (returned value != 0).
	 */
	static ystatus_t backup_exec_pre_script(uint64_t hash, char *key, void *data, void *user_data);
	/**
	 * @function	backup_exec_post_script
	 * @abstract	Execute a post-script.
	 * @param	hash		Index in the list of scripts.
	 * @param	key		Always null.
	 * @param	data		Path to the script to execute.
	 * @param	user_data	Pointer to the agent structure.
	 * @return	YENOERR if the script exited withhout error (returned value == 0),
	 *		YEBADCONF if the configuration is not valid,
	 *		YEFAULT if the script exited with an error (returned value != 0).
	 */
	static ystatus_t backup_exec_post_script(uint64_t hash, char *key, void *data, void *user_data);
	/**
	 * @function	backup_files
	 * @abstract	Backup all listed files. They are tar'ed and compressed.
	 * @param	agent	Pointer to the agent structure.
	 */
	static void backup_files(agent_t *agent);
	/**
	 * @function	backup_file
	 * @abstract	Backup one file.
	 * @param	hash		Index in the list of files.
	 * @param	key		Always null.
	 * @param	data		Path to the file to backup.
	 * @param	user_data	Pointer to the agent structure.
	 * @return	YENOERR if the file was tar'ed and compressed successfully.
	 *		YEBADCONF if the configuration is not valid,
	 *		YENOEXEC if an error occurred during the backup.
	 */
	static ystatus_t backup_file(uint64_t hash, char *key, void *data, void *user_data);
	/**
	 * @function	backup_databases
	 * @abstract	Backup all listed databases. They are tar'ed and compressed.
	 * @param	agent	Pointer to the agent structure.
	 */
	static void backup_databases(agent_t *agent);
	/**
	 * @function	backup_database
	 * @abstract	Backup a database.
	 * @param	hash		Index in the list of MySQL databases.
	 * @param	key		Always null.
	 * @param	data		Associative array with database's information.
	 * @param	user_data	Pointer to the agent structure.
	 * @return	YENOERR if the database was dumped and compressed successfully.
	 *		YEBADCONF if the configuration is not valid,
	 *		YENOEXEC if an error occurred during the backup.
	 */
	static ystatus_t backup_database(uint64_t hash, char *key, void *data, void *user_data);
	/**
	 * @funuction	backup_mysql
	 * @abstract	Backup a MySQL database.
	 * @param	agent	Pointer to the agent structure.
	 * @param	db_data	Pointer to the table which contains the database configuration.
	 * @return	YENOERR if the database was dumped and compressed successfully.
	 */
	static ystatus_t backup_mysql(agent_t *agent, ytable_t *db_data);
	/**
	 * @function	backup_pgsql
	 * @abstract	Backup a PostgreSQL database.
	 * @param	agent	Pointer to the agent structure.
	 * @param	db_data	Pointer to the table which contains the database configuration.
	 * @return	YENOERR if the database was dumped and compressed successfully.
	 */
	static ystatus_t backup_pgsql(agent_t *agent, ytable_t *db_data);
	/**
	 * @function	backup_mongodb
	 * @abstract	Backup a MongoDB database.
	 * @param	agent	Pointer to the agent structure.
	 * @param	db_data	Pointer to the table which contains the database configuration.
	 * @return	YENOERR if the database was dumped and compressed successfully.
	 */
	static ystatus_t backup_mongodb(agent_t *agent, ytable_t *db_data);
	/**
	 * @function	backup_encrypt
	 * @abstract	Encrypt each backed up file.
	 * @param	agent	Pointer to the agent structure.
	 */
	static void backup_encrypt_files(agent_t *agent);
	/**
	 * @function	backup_encrypt_item
	 * @abstract	Encrypt one backed up item.
	 * @param	hash		Index in the list of scripts.
	 * @param	key		Always null.
	 * @param	data		Pointer to the item.
	 * @param	user_data	Pointer to the agent structure.
	 * @return	YENOERR if the item was encrypted successfully.
	 */
	static ystatus_t backup_encrypt_item(uint64_t hash, char *key, void *data, void *user_data);
	/**
	 * @function	backup_compress_file
	 * @abstract	Compress a backed up file.
	 * @param	agent	Pointer to the agent structure.
	 * @param	log	Pointer to the file's log entry.
	 * @return	YENOERR if the file was compressed successfully.
	 */
	static ystatus_t backup_compress_file(agent_t *agent, log_item_t *log);
	/**
	 * @function	backup_compute_checksums
	 * @abstract	Compute the checksum of each backed up file.
	 * @param	agent	Pointer to the agent structure.
	 */
	static void backup_compute_checksums(agent_t *agent);
	/**
	 * @function	backup_compute_checkum_item
	 * @abstract	Compute the checksum of a backed up item.
	 * @param	hash		Index in the list of items.
	 * @param	key		Always null.
	 * @param	data		Pointer to the item.
	 * @param	user_data	Pointer to the agent structure.
	 * @return	YENOERR if the checksum was computed successfully.
	 */
	static ystatus_t backup_compute_checksum_item(uint64_t hash, char *key, void *data, void *user_data);
#endif // __A_BACKUP_PRIVATE__

