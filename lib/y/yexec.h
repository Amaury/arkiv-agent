/**
 * @header	utils_exec.h
 * @abstract	Execution of external programs.
 * @author	Amaury Bouchard <amaury@amaury.net>
 */
#pragma once

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "ymemory.h"
#include "yarray.h"
#include "ystatus.h"
#include "ybin.h"
#include "yfile.h"

/**
 * @function	yexec
 * @abstract	Execute a sub-program and wait for its termination.
 * @param	command		Path to the sub-program to execute.
 * @param	args		List of arguments.
 * @param	env		List of environment variables.
 * @param	out_memory	Pointer to a ybin_t that will be filled with the
 *				standard output. The string is allocated, thus must
 *				be freed. Could be set to NULL.
 * @param	out_file	Path to a file where the data will be written.
 *				Could be nulL.
 * @return	YENOERR if OK.
 */
ystatus_t yexec(const char *command, yarray_t args, yarray_t env,
                ybin_t *out_memory, const char *out_file);
/**
 * @function	yexec_stdin
 * @abstract	Execute a sub-program, sending data to its stdin, and wait for its termination.
 * @param	command		Path to the sub-program to execute.
 * @param	args		List of arguments.
 * @param	env		List of environment variables.
 * @param	stdin_str	Pointer to a null-terminated string that will be sent to the program's stdin.
 *				Could be null.
 * @param	stdin_bin	Pointer to a ybin_t that will be sent to the program's stdin.
 *				Not used if the stdin_str parameter is not null. Could be null.
 * @param	stdin_file	Path to a file which content will be sent to the program's stdin.
 *				Not used if the stdin_str or stdin_bin parameters are not null. Could be null.
 * @param	out_memory	Pointer to a ybin_t that will be filled with the
 *				standard output. The string is allocated, thus must
 *				be freed. Could be set to NULL.
 * @param	out_file	Path to a file where the data will be written.
 *				Could be nulL.
 * @return	YENOERR if OK.
 */
ystatus_t yexec_stdin(const char *command, yarray_t args, yarray_t env,
                      const char *stdin_str, ybin_t *stdin_bin, const char *stdin_file,
                      ybin_t *out_memory, const char *out_file);

