/**
 * @header	utils.h
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2024, Amaury Bouchard
 */
#pragma once

#include <stdbool.h>
#include "ystr.h"
#include "configuration.h"

/**
 * @function	check_program_exists
 *		Tells if a given program is installed on the local computer.
 *		It is search in '/bin', '/usr/bin' and '/usr/local/bin' directories,
 *		and with the 'which' program.
 * @param	bin_name	Name of the searched program.
 * @return	True if the program exists.
 */
bool check_program_exists(const char *bin_name);
/**
 * @function	get_program_path
 *		Returns the path to a program. The given program is searched in '/bin', '/usr/bin' and
 *		'/usr/local/bin' directories, and with the 'which' program.
 * @param	bin_name	Name of the searched program.
 * @return	The path to the program, or NULL.
 */
ystr_t get_program_path(const char *bin_name);
/**
 * @function	check_tar
 * @abstract	Checks if the tar program is installed. Aborts if not.
 */
void check_tar(void);
/**
 * @function	check_sh256sum
 * @abstract	Checks if the sha512sum program is installed. Aborts if not.
 */
void check_sha512sum(void);
/**
 * @header	check_z
 * @abstract	Checks if compression programs are installed.
 */
void check_z(void);
/**
 * @function	check_crypt
 * @abstract	Checks if encryption programs are installed.
 */
void check_crypt(void);
/**
 * @function	check_web
 * @abstract	Checks if web communication programs are installed.
 */
void check_web(void);
/**
 * @function	check_cron
 * @abstract	Checks if a crontab execution can be planned.
 * @return	The type of crontab installation.
 */
config_crontab_t check_cron(void);

