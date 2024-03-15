/**
 * @header	yjson.h
 * @abstrat	JSON manipulation.
 * @version	1.0.0 Feb 24 2022
 * @author	Amaury Bouchard <amaury@amaury.net>
 */
#pragma once

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif /* __cplusplus || c_plusplus */

#include "ystatus.h"

/**
 * @typedef	yjson_parser_t
 * 		JSON parser structure.
 * @field	input	Pointer to the input string.
 * @field	ptr	Pointer to the currently parsed character.
 * @field	line	Number of the currently parsed line.
 * @field	status	Parsing status.
 */
typedef struct {
	char *input;
	char *ptr;
	unsigned int line;
	ystatus_t status;
} yjson_parser_t;

#include <stdbool.h>
#include "yvar.h"

/**
 * @function	yjson_new
 *		Create a new JSON parser.
 * @return	A pointer to the initialised JSON parser object.
 */
yjson_parser_t *yjson_new(void);

/**
 * @function	yjson_free
 *		Destroy a JSON parser.
 * @param	json	Pointer to a JSON parser object.
 */
void yjson_free(yjson_parser_t *json);

/**
 * @function	yjson_parse
 *		Starts a JSON parser.
 * @param	json	Pointer to the JSON parser object.
 * @param	input	Pointer to the string to parse.
 * @return	The root node value.
 */
yres_var_t yjson_parse(yjson_parser_t *json, char *input);

/**
 * @function	yjson_print
 *		Prints a JSON value node and its subnodes.
 * @param	value	Pointer to the value node.
 * @param	pretty	True to have newlines and indentations.
 */
void yjson_print(yvar_t *value, bool pretty);

/**
 * @function	yjson_sprint
 *		Creates a string which contains the JSON stream of a value node.
 * @param	value	Pointer to the value node.
 * @param	pretty	True to have newlines and indentations.
 * @return	Allocated ystring.
 */
ystr_t yjson_sprint(yvar_t *value, bool pretty);

/**
 * @function	yjson_fprint
 *		Prints a JSON value node and its subnodes to a stream.
 * @param	stream	Pointer to the stream.
 * @param	value	Pointer to the value node.
 * @param	pretty	True to have newlines and indentations.
 */
void yjson_fprint(FILE *stream, yvar_t *value, bool pretty);

/**
 * @function	yjson_write
 *		Write a JSON stream in a file.
 * @param	path	Path to the file.
 * @param	value	Pointer to the value node.
 * @param	pretty	True to have newlines and indentations.
 * @return	YENOERR if everything went fine.
 */
ystatus_t yjson_write(const char *path, yvar_t *value, bool pretty);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif /* __cplusplus || c_plusplus */
