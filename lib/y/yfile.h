/**
 * @header	yfile.h
 * @abstract	Files and directories management.
 * @author	Amaury Bouchard <amaury@amaury.net>
 */
#pragma once

#if defined(_cplusplus) || defined(c_pluspuls)
extern "C" {
#endif /* _cplusplus || c_plusplus */

#include <stdbool.h>
#include <sys/stat.h>
#include "ybin.h"

/**
 * @function	yfile_exists
 *		Tell if a file exists.
 * @param	path	Path to the file.
 * @return	True if the file exists.
 */
bool yfile_exists(const char *path);
/**
 * @function	yfile_is_link
 *		Tell if a file is a symbolic link.
 * @param	path	Path to the file.
 * @return	True if the file is a link.
 */
bool yfile_is_link(const char *path);
/**
 * @function	yfile_is_dir
 *		Tell if a directory exists.
 * @param	path	Path to the file.
 * @return	True if the file is a directory.
 */
bool yfile_is_dir(const char *path);
/**
 * @function	yfile_is_readable
 *		Tell if a file could be read by the current user.
 * @param	path	Path to the file.
 * @return	True if the file is readable.
 */
bool yfile_is_readable(const char *path);
/**
 * @function	yfile_is_writable
 *		Tell if a file could be written by the current user.
 * @param	path	Path to the file.
 * @return	True if the file is writable.
 */
bool yfile_is_writable(const char *path);
/**
 * @function	yfile_is_executable
 *		Tell if a file could be executed by the current user.
 * @param	path	Path to the file.
 * @return	True if the file is executable.
 */
bool yfile_is_executable(const char *path);
/**
 * @function	yfile_mkpath
 *		Create a path of directories and subdirectories.
 * @param	path	Path to create.
 * @param	mode	New directories' mode (see man 2 mkdir).
 * @return	True if the path exists or if it was created.
 */
bool yfile_mkpath(const char *path, mode_t mode);
/**
 * @function	yfile_touch
 *		Create an empty file. If the path doesn't exist, it is created.
 * @param	path		Path to the file.
 * @param	file_mode	New file's mode.
 * @param	dir_mode	New directories' mode.
 * @return	True if the file exists or has been created.
 */
bool yfile_touch(const char *path, mode_t file_mode, mode_t dir_mode);
/**
 * @function	yfile_tmp
 *		Create a temporary file, randomly named from a prefix path.
 *		The created file is readable and writable only by the user.
 * @param	prefix	Path prefix. A suffix "-XXXXXX" will be added ("XXXXXX" will be replaced
 *			be a random string).
 * @return	The full name of the created file, or NULL if the file can't be created.
 *		The returned string must be freed.
 */
char *yfile_tmp(const char *prefix);
/**
 * @function	yfile_get_contents
 *		Read the full content of a file.
 * @param	path	File path.
 * @return	The content of the file, or NULL if the file is not readable.
 */
ybin_t *yfile_get_contents(const char *path);
/**
 * @function	yfile_get_string_contents
 *		Read the full textual content of a file.
 * @param	path	File path.
 * @return	The content of the file, or NULL if the file is not readable.
 */
ystr_t yfile_get_string_contents(const char *path);
/**
 * @function	yfile_put_contents
 *		Write some data in a file (mode 600).
 * @param	path	File path.
 * @param	data	Binary data.
 * @return	True if everything went fine.
 */
bool yfile_put_contents(const char *path, ybin_t *data);
/**
 * @function	yfile_put_string
 * @abstract	Write a string in a file (mode 600).
 * @param	path	File path.
 * @param	str	String.
 * @return	True if everything went fine.
 */
bool yfile_put_string(const char *path, const char *str);
/**
 * @function	yfile_append_contents
 * @abstract	Append data at the end of a file.
 * @param	path	File path.
 * @param	data	Binary data.
 * @return	True if everything went fine.
 */
bool yfile_append_contents(const char *path, ybin_t *data);
/**
 * @function	yfile_append_string
 * @abstract	Append a string at the end of a file.
 * @param	path	File path.
 * @param	str	String.
 * @return	True if everything went fine.
 */
bool yfile_append_string(const char *path, const char *str);
/**
 * @function	yfile_contains
 * @abstract	Tell if a file contains a given string. The file is read in one pass.
 * @param	path	File path.
 * @param	str	String searched in the file.
 * @return	True if the string was found.
 */
bool yfile_contains(const char *path, const char *str);

#if defined(_cplusplus) || defined(c_pluspuls)
}
#endif /* _cplusplus || c_plusplus */

