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

/**
 * @function	exec_configuration
 *		Main function for configuration file generation.
 */
void exec_configuration(void);
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

