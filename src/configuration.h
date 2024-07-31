/**
 * @header	configuration.h
 *
 * Definitions used for the generation of the configuration file.
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2024, Amaury Bouchard
 */
#pragma once

#include <stdbool.h>
#include "ystr.h"
#include "agent.h"

/**
 * @typedef	config_crontab_t
 * @abstract	Type of cron installation.
 * @field	A_CONFIG_CRON_HOURLY	/etc/cron.hourly
 * @field	A_CONFIG_CRON_D		/etc/cron.d
 * @field	A_CONFIG_CRON_CRONTAB	/etc/crontab
 */
typedef enum {
	A_CONFIG_CRON_HOURLY,
	A_CONFIG_CRON_D,
	A_CONFIG_CRON_CRONTAB
} config_crontab_t;

/**
 * @function	exec_configuration
 * @abstract	Main function for configuration file generation.
 * @param	agent	Pointer to the agent structure.
 */
void exec_configuration(agent_t *agent);

/* ********** PRIVATE FUNCTIONS ********** */
#ifdef __A_CONFIGURATION_PRIVATE__
	/**
	 * @function	config_ask_orgkey
	 * @abstract	Asks for the organization key.
	 * @param	agent	Pointer to the agent structure.
	 * @return	The key. Must be freed.
	 */
	static ystr_t config_ask_orgkey(agent_t *agent);
	/**
	 * @function	config_ask_hostname
	 * @abstract	Asks for the hostname.
	 * @param	agent	Pointer to the agent structure.
	 * @return	The hostname.
	 */
	static ystr_t config_ask_hostname(agent_t *agent);
	/**
	 * @function	config_ask_archives_path
	 * @abstract	Asks for the local archives path.
	 * @param	agent	Pointer to the agent structure.
	 * @return	The archives path.
	 */
	static ystr_t config_ask_archives_path(agent_t *agent);
	/**
	 * @function	config_ask_log_file
	 * @bastract	Asks for the log file.
	 * @param	agent	Pointer to the agent structure.
	 * @return	The log file's path.
	 */
	static ystr_t config_ask_log_file(agent_t *agent);
	/**
	 * @function	config_ask_syslog
	 * @abstract	Asks for syslog.
	 * @param	agent	Pointer to the agent structure.
	 * @return	True if syslog is used.
	 */
	static bool config_ask_syslog(agent_t *agent);
	/**
	 * @function	config_ask_encryption_password
	 * @abstract	Asks for the encryption password.
	 * @param	agent	Pointer to the agent structure.
	 * @return	The encryption password.
	 */
	static ystr_t config_ask_encryption_password(agent_t *agent);
	/**
	 * @function	config_write_json_file
	 * @abstract	Writes the JSON configuration file.
	 * @param	org_key		Organization key.
	 * @param	hostname	Hostname.
	 * @param	archives_path	Archives path.
	 * @param	logfile		Log file's path.
	 * @param	syslog		True if syslog is used.
	 * @param	crypt_pwd	Encryption password.
	 */
	static void config_write_json_file(const char *orgKey, const char *hostname, const char *archives_path,
	                                   const char *logfile, bool syslog, const char *cryptPwd);
	/**
	 * @function	config_add_to_crontab
	 * @abstract	Check if the agent execution is already in crontab. If not, add it.
	 * @param	agent		Pointer to the agent structure.
	 * @param	cron_type	Type of available crontab.
	 * @return	YENOERR if the agent execution was in crontab or has been added successfully.
	 */
	static void config_add_to_crontab(agent_t *agent, config_crontab_t cron_type);
	/**
	 * @function	config_add_to_logrotate
	 * @abstract	Add log file management by logrotate, if possible.
	 * @param	logfile	Log file's path.
	 */
	static void config_add_to_logrotate(const char *logfile);
#endif // __A_CONFIGURATION_PRIVATE__

