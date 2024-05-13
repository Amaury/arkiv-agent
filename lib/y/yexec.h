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
#include <errno.h>
#include "ymemory.h"
#include "yarray.h"
#include "ystatus.h"
#include "ybin.h"

/**
 * @function	Execute a sub-program and wait for its termination.
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

