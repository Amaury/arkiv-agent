#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include "ymemory.h"
#include "ydefs.h"
#include "yjson.h"

/* Private functions */
static void _yjson_fsprint_string(char *output, ystr_t *str, FILE *stream);
static ystatus_t _yjson_table_print_elem(uint64_t index, char *key, void *data, void *user_data);
static void _yjson_value_fsprint(yvar_t *value, ystr_t *str, FILE *stream, uint32_t depth, bool linefeed);
static ystatus_t _yjson_remove_space(yjson_parser_t *json);
static yvar_t _yjson_parse_chunk(yjson_parser_t *json);
static void _yjson_parse_string(yjson_parser_t *json, yvar_t *value);
static void _yjson_parse_number(yjson_parser_t *json, yvar_t *value);
static void _yjson_parse_array(yjson_parser_t *json, yvar_t *value);
static void _yjson_parse_object(yjson_parser_t *json, yvar_t *value);

/* Map of special characters, used to parse strings. */
static unsigned char _yjson_special_chars[] = {
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //   0 -   7
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //   8 -  15
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //  16 -  23
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //  24 -  31
	'\0', '\0', '\"', '\0', '\0', '\0', '\0', '\'', //  32 -  39
	'\0', '\0', '\0', '\0', '\0', '\0', '\0',  '/', //  40 -  47
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //  48 -  55
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //  56 -  63
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //  64 -  71
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //  72 -  79
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', //  80 -  87
	'\0', '\0', '\0', '\0', '\\', '\0', '\0', '\0', //  88 -  95
	'\0', '\a', '\b', '\0', '\0', '\0', '\f', '\0', //  96 - 103
	'\0', '\0', '\0', '\0', '\0', '\0', '\n', '\0', // 104 - 111
	'\0', '\0', '\r', '\0', '\t', '\0', '\0', '\0', // 112 - 119
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', // 120 - 127
};

/** @define YJSON_INDENT Macro used for indentation purpose. */
#define YJSON_INDENT(depth, str, stream) do { for (uint32_t i = 0; i < depth; ++i) _yjson_fsprint_string("\t", str, stream); } while (0)

/* Initialiser of JSON parser. */
yjson_parser_t *yjson_new(void) {
	return (malloc0(sizeof(yjson_parser_t)));
}

/* Destroy a JSON parser. */
void yjson_free(yjson_parser_t *json) {
	free0(json);
}

/* Starts a JSON parser. */
yres_var_t yjson_parse(yjson_parser_t *json, char *input) {
	yres_var_t res;

	// check parameters
	if (!json || !input)
		return (YRESULT_ERR(yres_var_t, YEPARAM));
	// parser initialization
	*json = (yjson_parser_t){
		.input = input,
		.ptr = input,
		.status = YENOERR,
	};
	// parser execution
	res.value = _yjson_parse_chunk(json);
	// check status
	if (json->status != YENOERR)
		return (YRESULT_ERR(yres_var_t, json->status));
	if (json->status == YENOERR && *json->ptr != '\0') {
		yvar_delete(&res.value);
		yvar_init_undef(&res.value);
		return (YRESULT_ERR(yres_var_t, (json->status = YESYNTAX)));
	}
	// no error
	res.status = YENOERR;
	return (res);
}

/* ********** PRINT/WRITE JSON ********** */
/* Prints a JSON value node and its subnodes, with newlines and tabulations. */
void yjson_print(yvar_t *value, bool pretty) {
	_yjson_value_fsprint(value, NULL, stdout, 0, pretty);
	if (pretty)
		printf("\n");
}
/* Creates a string which contains the JSON stream of a value node. */
ystr_t yjson_sprint(yvar_t *value, bool pretty) {
	ystr_t ys = ys_new("");
	_yjson_value_fsprint(value, &ys, NULL, 0, pretty);
	return (ys);
}
/* Prints a JSON value node and its subnodes to a stream. */
void yjson_fprint(FILE *stream, yvar_t *value, bool pretty) {
	_yjson_value_fsprint(value, NULL, stream, 0, pretty);
	if (pretty)
		fprintf(stream, "\n");
}
/* Write a JSON stream in a file. */
ystatus_t yjson_write(const char *path, yvar_t *value, bool pretty) {
	FILE *stream = fopen(path, "w+");
	if (!stream) {
		if (errno == ENOMEM)
			return (YENOMEM);
		return (YEIO);
	}
	_yjson_value_fsprint(value, NULL, stream, 0, pretty);
	if (pretty)
		fprintf(stream, "\n");
	fclose(stream);
	return (YENOERR);
}
/* ---------- PRINT/WRITE PRIVATE FUNCTIONS ---------- */
/* private structure: parameters used to print a table element. */
typedef struct {
	ystr_t   *str;
	FILE     *stream;
	uint32_t depth;
	uint32_t len;
	bool     is_array;
	bool     pretty;
} _yjson_table_params;
/* Prints a JSON value node and its subnodes. */
static ystatus_t _yjson_table_print_elem(uint64_t index, char *key, void *data, void *user_data) {
	yvar_t *val = data;
	_yjson_table_params *p = user_data;

	if (p->pretty)
		YJSON_INDENT(p->depth + 1, p->str, p->stream);
	if (!p->is_array) {
		ystr_t ys = NULL;
		if (key) {
			ys = ys_create(strlen(key) + 4);
			ys_printf(&ys, "\"%s\": ", key);
		} else {
			ys = ys_create(12);
			ys_printf(&ys, "\"%lu\": ", index);
		}
		_yjson_fsprint_string(ys, p->str, p->stream);
		ys_free(ys);
	}
	_yjson_value_fsprint(val, p->str, p->stream, p->depth + 1, p->pretty);
	if (p->len > 1 && p->pretty)
		_yjson_fsprint_string(",\n", p->str, p->stream);
	else if (p->len > 1)
		_yjson_fsprint_string(",", p->str, p->stream);
	else if (p->len <= 1 && p->pretty)
		_yjson_fsprint_string("\n", p->str, p->stream);
	p->len--;
	return (YENOERR);
}
/* Write a string in a ystring or a stream. */
static void _yjson_fsprint_string(char *output, ystr_t *str, FILE *stream) {
	if (str)
		ys_append(str, output);
	if (stream)
		fprintf(stream, "%s", output);
}
/* Write a value in a string or a stream. */
static void _yjson_value_fsprint(yvar_t *value, ystr_t *str, FILE *stream, uint32_t depth, bool linefeed) {
	if (!value) {
		_yjson_fsprint_string("(unset)", str, stream);
	} else if (yvar_is_undef(value)) {
		_yjson_fsprint_string("(undef)", str, stream);
	} else if (yvar_is_null(value)) {
		_yjson_fsprint_string("null", str, stream);
	} else if (yvar_is_bool(value)) {
		_yjson_fsprint_string(yvar_get_bool(value) ? "true" : "false", str, stream);
	} else if (yvar_is_int(value)) {
		ystr_t ys = ys_printf(NULL, "%" PRId64, yvar_get_int(value));
		_yjson_fsprint_string(ys, str, stream);
		ys_free(ys);
	} else if (yvar_is_float(value)) {
		ystr_t ys = ys_printf(NULL, "%g", yvar_get_float(value));
		_yjson_fsprint_string(ys, str, stream);
		ys_free(ys);
	} else if (yvar_is_const_string(value)) {
		ystr_t ys = ys_printf(NULL, "\"%s\"", yvar_get_const_string(value));
		_yjson_fsprint_string(ys, str, stream);
		ys_free(ys);
	} else if (yvar_is_string(value)) {
		ystr_t ys = ys_printf(NULL, "\"%s\"", yvar_get_string(value));
		_yjson_fsprint_string(ys, str, stream);
		ys_free(ys);
	} else if (yvar_is_table(value)) {
		ytable_t *table = yvar_get_table(value);
		bool is_array = ytable_is_array(table);
		char *s = is_array ? (linefeed ? "[\n" : "[") : (linefeed ? "{\n" : "{");
		_yjson_fsprint_string(s, str, stream);
		_yjson_table_params params = {
			.str = str,
			.stream = stream,
			.depth = depth,
			.len = ytable_length(table),
			.is_array = is_array,
			.pretty = linefeed
		};
		ytable_foreach(table, _yjson_table_print_elem, &params);
		if (linefeed)
			YJSON_INDENT(depth, str, stream);
		_yjson_fsprint_string((is_array ? "]" : "}"), str, stream);
	} else if (yvar_is_pointer(value)) {
		_yjson_fsprint_string("(pointer)", str, stream);
	} else if (yvar_is_object(value)) {
		_yjson_fsprint_string("(object)", str, stream);
	} else {
		_yjson_fsprint_string("(unknown)", str, stream);
	}
}

/* Parse */
/* Remove spaces from a JSON string. */
static ystatus_t _yjson_remove_space(yjson_parser_t *json) {
	// process spaces
	while (isspace(*json->ptr)) {
		if (*json->ptr == LF)
			++json->line;
		++json->ptr;
	}
	// process single-line comments
	if (!strncmp(json->ptr, "//", 2)) {
		for (json->ptr += 2; *json->ptr != '\0' && *json->ptr != LF; ++json->ptr)
			;
		// remove remaining spaces
		return (_yjson_remove_space(json));
	}
	// process multi-lines comments
	if (strncmp(json->ptr, "/*", 2))
		return (json->status);
	bool found = false;
	for (json->ptr += 2; *json->ptr != '\0'; ++json->ptr) {
		if (!strncmp(json->ptr, "*/", 2)) {
			found = true;
			json->ptr += 2;
			break;
		}
	}
	if (!found) {
		json->status = YESYNTAX;
		return (json->status);
	}
	// remove remaining spaces
	return (_yjson_remove_space(json));
}
/* Parse a chunk of JSON. */
static yvar_t _yjson_parse_chunk(yjson_parser_t *json) {
	char c;
	yvar_t result;

	yvar_init_undef(&result);
	if (_yjson_remove_space(json) != YENOERR)
		goto end;
	c = *json->ptr;
	if (c == '\0')
		goto end;
	else if (c == '{') {
		// object
		json->ptr++;
		_yjson_parse_object(json, &result);
		goto end;
	} else if (c == '[') {
		// array
		json->ptr++;
		_yjson_parse_array(json, &result);
		goto end;
	} else if (c == '"') {
		// string
		json->ptr++;
		_yjson_parse_string(json, &result);
		goto end;
	} else if (!strncasecmp(json->ptr, "null", 4)) {
		// null
		json->ptr += 4;
		yvar_init_null(&result);
		goto end;
	} else if (!strncasecmp(json->ptr, "false", 5)) {
		// false
		json->ptr += 5;
		yvar_init_bool(&result, false);
		goto end;
	} else if (!strncasecmp(json->ptr, "true", 4)) {
		// true
		json->ptr += 4;
		yvar_init_bool(&result, true);
		goto end;
	} else {
		// number
		_yjson_parse_number(json, &result);
		goto end;
	}
end:
	if (json->status != YENOERR || _yjson_remove_space(json) != YENOERR) {
		yvar_init_undef(&result);
	}
	return (result);
}
/**
 * Parse a string.
 * @see	https://gist.github.com/MightyPork/52eda3e5677b4b03524e40c9f0ab1da5
 */
static void _yjson_parse_string(yjson_parser_t *json, yvar_t *value) {
	//char *ptr = json->ptr;
	// allocate a buffered string
	ystr_t str = ys_new("");
	if (!str) {
		yvar_init_undef(value);
		json->status = YENOMEM;
		return;
	}
	// loop on characters
	while (*json->ptr != '\0') {
		unsigned char next_c = *(json->ptr + 1);
		// end of string
		if (*json->ptr == '"')
			break;
		// count lines
		if (*json->ptr == '\n')
			++json->line;
		// escaped characters
		if (*json->ptr == '\\') {
			unsigned char final_c;
			if (next_c < sizeof(_yjson_special_chars) &&
			    (final_c = _yjson_special_chars[next_c])) {
				// regular escaped characters (\n, \r, \t...)
				ys_addc(&str, final_c);
				json->ptr += 2;
			} else if (next_c == 'u' && isxdigit(json->ptr[2]) &&
			           isxdigit(json->ptr[3]) && isxdigit(json->ptr[4]) &&
			           isxdigit(json->ptr[5])) {
				// unicode character => convert to UTF-8
				char s[5] = {
					json->ptr[2],
					json->ptr[3],
					json->ptr[4],
					json->ptr[5],
					'\0'
				};
				long cp = strtol(s, NULL, 16);
				if (cp <= 0x7F) {
					// ASCII character
					ys_addc(&str, (char)cp);
				} else if (cp <= 0x07FF) {
					// 2 bytes character
					unsigned char s[3] = {
						(unsigned char)(((cp >> 6) & 0x1F) | 0xC0),
						(unsigned char)(((cp >> 0) & 0x3F) | 0x80),
						'\0'
					};
					ys_append(&str, (char*)s);
				} else if (cp <= 0xFFFF) {
					// 3 bytes character
					unsigned char s[4] = {
						(unsigned char)(((cp >> 12) & 0x0F) | 0xE0),
						(unsigned char)(((cp >>  6) & 0x3F) | 0x80),
						(unsigned char)(((cp >>  0) & 0x3F) | 0x80),
						'\0'
					};
					ys_append(&str, (char*)s);
				} else if (cp <= 0x10FFFF) {
					// 4 bytes character
					unsigned char s[5] = {
						(unsigned char)(((cp >> 18) & 0x07) | 0xF0),
						(unsigned char)(((cp >> 12) & 0x3F) | 0x80),
						(unsigned char)(((cp >>  6) & 0x3F) | 0x80),
						(unsigned char)(((cp >>  0) & 0x3F) | 0x80),
						'\0'
					};
					ys_append(&str, (char*)s);
				} else {
					// error: remplacement character
					unsigned char s[4] = {
						(unsigned char)0xEF,
						(unsigned char)0xBF,
						(unsigned char)0xBD,
						'\0'
					};
					ys_append(&str, (char*)s);
				}
			} else {
				// syntax error
				goto syntax_error;
			}
		} else {
			ys_addc(&str, *json->ptr);
			++json->ptr;
		}
	}
	if (*json->ptr != '"')
		goto syntax_error;
	*json->ptr = '\0';
	++json->ptr;
	yvar_init_string(value, str);
	return;
syntax_error:
	yvar_init_undef(value);
	json->status = YESYNTAX;
}
/* Parse a number. */
static void _yjson_parse_number(yjson_parser_t *json, yvar_t *value) {
	/*
	 * Number are on the form:
	 * minus character (optional)
	 * one or more digits (0-9)
	 * dot character followed by one or more digits (optional)
	 * exponential part (optional)
	 *	"e|e+|e-|E|E+|E-" followed by one or more digits
	 */
	char *end;
	char *ptr = json->ptr;
	bool dot = false;

	// check syntax
	if (*ptr == '-')
		++ptr;
	if (!isdigit(*ptr)) {
		json->status = YESYNTAX;
		return;
	}
	while (isdigit(*ptr))
		++ptr;
	if (*ptr == '.') {
		dot = true;
		++ptr;
		if (!isdigit(*ptr)) {
			json->status = YESYNTAX;
			return;
		}
	}
	// conversion
	if (dot) {
		double float_value = strtold(json->ptr, &end);
		yvar_init_float(value, float_value);
	} else {
		int64_t int_value = (int64_t)strtoll(json->ptr, &end, 10);
		yvar_init_int(value, int_value);
	}
	json->ptr = end;
}
/* Parse an array. */
static void _yjson_parse_array(yjson_parser_t *json, yvar_t *value) {
	if (_yjson_remove_space(json) != YENOERR)
		return;
	ytable_t *table = ytable_new();
	while (*json->ptr != '\0') {
		// search for end of list
		if (*json->ptr == RBRACKET) {
			++json->ptr;
			yvar_init_table(value, table);
			return;
		}
		// get value
		yvar_t val = _yjson_parse_chunk(json);
		if (json->status != YENOERR)
			goto error;
		yvar_t *pval = yvar_clone(&val);
		if (!pval) {
			json->status = YENOMEM;
			goto error;
		}
		// add the value to the array
		ytable_add(table, pval);
		// skip spaces
		if (_yjson_remove_space(json) != YENOERR)
			goto error;
		// process the rest
		if (*json->ptr == RBRACKET) {
			continue;
		} else if (*json->ptr != COMMA) {
			json->status = YESYNTAX;
			goto error;
		}
		++json->ptr;
		if (_yjson_remove_space(json) != YENOERR)
			goto error;
	}
	// end of string
	json->status = YESYNTAX;
error:
	ytable_free(table);
}
/* Parse an object. */
static void _yjson_parse_object(yjson_parser_t *json, yvar_t *value) {
	ystr_t key = NULL;
	if (_yjson_remove_space(json) != YENOERR)
		return;
	ytable_t *table = ytable_create(8, NULL, json);
	while (*json->ptr != '\0') {
		// search for end of object
		if (*json->ptr == RBRACE) {
			++json->ptr;
			yvar_init_table(value, table);
			return;
		}
		// get key
		yvar_t val_key = _yjson_parse_chunk(json);
		if (json->status != YENOERR)
			goto error;
		if (!yvar_is_string(&val_key)) {
			json->status = YESYNTAX;
			goto error;
		}
		key = yvar_get_string(&val_key);
		if (_yjson_remove_space(json) != YENOERR)
			goto error;
		// search colon character
		if (*json->ptr != COLON) {
			json->status = YESYNTAX;
			goto error;
		}
		++json->ptr;
		// get value
		yvar_t val = _yjson_parse_chunk(json);
		if (json->status != YENOERR)
			goto error;
		yvar_t *pval = yvar_clone(&val);
		if (!pval) {
			json->status = YENOMEM;
			goto error;
		}
		// add to hashmap
		ytable_set_key(table, key, pval);
		// process the rest
		if (*json->ptr == RBRACE) {
			continue;
		} else if (*json->ptr != COMMA) {
			json->status = YESYNTAX;
			goto error;
		}
		++json->ptr;
		if (_yjson_remove_space(json) != YENOERR)
			goto error;
	}
error:
	ys_free(key);
	ytable_free(table);
}

