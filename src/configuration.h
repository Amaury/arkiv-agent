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
 * @function	exec_configuration
 * @abstract	Main function for configuration file generation.
 * @param	agent	Pointer to the agent structure.
 */
void exec_configuration(agent_t *agent);
/**
 * @function	config_program_exists
 *		Tells if a given program is installed on the local computer.
 *		It is search in '/bin', '/usr/bin' and '/usr/local/bin' directories,
 *		and with the 'which' program.
 * @param	binName	Name of the searched program.
 * @return	True if the program exists.
 */
bool config_program_exists(const char *binName);
/**
 * @function	config_get_program_path
 *		Returns the path to a program. The given program is searched in '/bin', '/usr/bin' and
 *		'/usr/local/bin' directories, and with the 'which' program.
 * @param	binName	Name of the searched program.
 * @return	The path to the program, or NULL.
 */
ystr_t config_get_program_path(const char *binName);
/**
 * @function	config_declare_server
 * @abstract	Declares the server to arkiv.sh API.
 * @param	orgKey		Organization key.
 * @param	hostname	Hostname.
 */
void config_declare_server(const char *orgKey, const char *hostname);

