/**
 * @header	yvar.h
 * @abstract	General data wrapper.
 *
 * The yvar data wrapper can contain any type of data. By default, it manages the following
 * types: undef (means the yvar has no defined type), null, bool, int (64 bits signed integer),
 * float (64 bits floaint point number), binary (ybin_t data type), string (ystr_t data type),
 * table (ytable_t data type, could be used as an array or an ordered hash map), pointer
 * (simple void* pointer) and object (void* pointer associated to a destruction function).
 *
 * <h2>Creation</h2>
 * It is possible to create an undefined yvar using the yvar_new_undef() function, and
 * initialize it later using yvar_init_null(), yvar_init_bool(), yvar_init_int()...
 * It is also possible to directly create a yvar of the desired type using the functions
 * yvar_new_null(), yvar_new_bool(), yvar_new_int()...
 *
 * <h2>Destruction</h2>
 * yvar_free() must be called to destroy a yvar.
 * yvar_delete() should be used to recursively destroy a yvar.
 *
 * <h2>Creation from JSON</h2>
 * The Ylib's JSON parser can be used to get a root yvar.
 *
 * Example:
 * <code>
 * 	yjson_parser_t json;
 * 	yres_var_t result = yjson_parse(&json, "{\"aa\": 12, \"bb\": 23}");
 * 	YASSERT(result, "JSON parsing error line %d.", json.line);
 * 	yjson_print(YRES_VAL(result));
 * </code>
 * Result:
 * <code>
 * 	{
 * 	    "aa": 12,
 * 	    "bb": 23
 * 	}
 * </code>
 *
 * <h2>Path</h2>
 * From a given yvar, it is possible to extract deep data using a "vpath" similar to XPath for
 * XML.
 *
 * "/foo"
 * Returns the "foo" entry of the root yvar, which is an associative array (ytable).
 *
 * "/foo/bar"
 * Returns the "bar" entry of the ytable under the key "foo" in the root yvar.
 *
 * "/foo[0]"
 * Returns the first entry of the array (ytable) located under the "foo" key of the root yvar.
 */
#pragma once

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif /* __cplusplus || c_plusplus */

/** @typedef yvar_t Type from struct yvar_s. */
typedef struct yvar_s yvar_t;

#include "ybin.h" 
#include "ystr.h"
#include "ytable.h"

/** @typedef yvar_function_t	Function used to delete an object variable. */
typedef ystatus_t (*yvar_function_t)(void *pointer, void *user_data);
/** @typedef yvar_memory_function_t	Function used to copy a yvar. */
typedef yvar_t *(*yvar_memory_function_t)(yvar_t *var);

/**
 * @typedef	yvar_type_t
 *		Type of a yvar.
 * @constant	YVAR_UNDEF		The yvar was not defined.
 * @constant	YVAR_NULL		Null.
 * @constant	YVAR_BOOL		Boolean.
 * @constant	YVAR_INT		Integer.
 * @constant	YVAR_FLOAT		Floating-point number.
 * @constant	YVAR_CONST_BINARY	Binary data.
 * @constant	YVAR_BINARY		Bufferized binary data.
 * @constant	YVAR_CONST_STRING	Constant character string.
 * @constant	YVAR_STRING		Bufferized character string.
 * @constant	YVAR_TABLE		Table (ordered list of values or key/value pairs).
 * @constant	YVAR_POINTER		Any pointer.
 * @constant	YVAR_OBJECT		Any object.
 */
typedef enum {
	YVAR_UNDEF = 0,
	YVAR_NULL,
	YVAR_BOOL,
	YVAR_INT,
	YVAR_FLOAT,
	YVAR_CONST_BINARY,
	YVAR_BINARY,
	YVAR_CONST_STRING,
	YVAR_STRING,
	YVAR_TABLE,
	YVAR_POINTER,
	YVAR_OBJECT,
} yvar_type_t;
/**
 * @typedef	yvar_definition_t
 *		Structure that contains memory management functions of a given type.
 * @field	delete_function		Pointer to a delete function.
 * @field	clone_function		Pointer to a function used to clone the object.
 */
typedef struct {
	yvar_memory_function_t delete_function;
	yvar_memory_function_t clone_function;
} yvar_definition_t;
/**
 * @struct	yvar_s
 *		Data wrapper.
 * @field	dynamic_alloc	1 = dynamic allocation of the yvar.
 * @field	refcount	Reference count.
 *				Positive value = dynamic allocation
 *				Negative value = static allocation
 * @field	definition	Memory management functions.
 * @field	user_data	Pointer to user data.
 * @field	type		Data type.
 * @field	bool_value	Boolean value.
 * @field	int_valeur	Integer or datetime value.
 * @field	float_value	Floating-point value.
 * @field	binary_value	Binary data value.
 * @field	const_value	Constant character string value.
 * @field	string_value	Character string value.
 * @field	table_value	Table value.
 * @field	pointer_value	Pointer value.
 */
struct yvar_s {
	_Atomic int32_t refcount;
	yvar_definition_t *definition;
	void *user_data;
	yvar_type_t type;
	union {
		bool bool_value;
		int64_t int_value;
		double float_value;
		ybin_t binary_value;
		const char *const_value;
		ystr_t string_value;
		ytable_t *table_value;
		void *pointer_value;
	};
};

/**
 * @typedef	yres_var_t
 *		yres_t derived structure, for yvar_t data type.
 */
typedef struct {
	ystatus_t status;
	yvar_t value;
} yres_var_t;

/**
 * @function	yvar_new_undef
 *		Create a new undefined yvar.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_undef(void);
/**
 * @function	yvar_init_undef
 *		Initialize a yvar structure with an undefined value.
 * @param	var	A pointer to the structure that must be initialized.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_undef(yvar_t *var);
/**
 * @function	yvar_new_null
 *		Create a new null yvar.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_null(void);
/**
 * @function	yvar_init_null
 *		Initialize a yvar structure with a null value.
 * @param	var	A pointer to the structure that must be initialized.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_null(yvar_t *var);
/**
 * @function	yvar_new_bool
 *		Create a new boolean yvar.
 * @param	value	Boolean value.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_bool(bool value);
/**
 * @function	yvar_init_bool
 *		Initialize a yvar structure with a boolean value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Boolean value.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_bool(yvar_t *var, bool value);
/**
 * @function	yvar_new_int
 *		Create a new integer yvar.
 * @param	value	Integer value.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_int(int64_t value);
/**
 * @function	yvar_init_int
 *		Initialize a yvar structure with an integer value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Integer value.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_int(yvar_t *var, int64_t value);
/**
 * @function	yvar_new_float
 *		Create a new floating-point number yvar.
 * @param	value	Floating-point number value.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_float(double value);
/**
 * @function	yvar_init_float
 *		Initialize a yvar structure with a floating-point number value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Floating-point number value.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_float(yvar_t *var, double value);
/**
 * @function	yvar_new_const_binary
 *		Create a new constant binary value yvar.
 * @param	value	constant binary data that will not be freed.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_const_binary(ybin_t value);
/**
 * @function	yvar_init_binary
 *		Initialize a yvar structure with a binary value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Constant binary data that will not be freed.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_const_binary(yvar_t *var, ybin_t value);
/**
 * @function	yvar_new_binary
 *		Create a new binary value yvar.
 * @param	value	Binary data that should be freed when the yvar will be freed.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_binary(ybin_t value);
/**
 * @function	yvar_init_binary
 *		Initialize a yvar structure with a binary value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Binary data that should be freed when the yvar will be freed.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_binary(yvar_t *var, ybin_t value);
/**
 * @function	yvar_new_const_string
 *		Create a new constant character string yvar.
 *		The enclosed string will not be freed when the yvar is freed.
 * @param	value	Character string value, or NULL.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_const_string(const char *value);
/**
 * @function	yvar_init_const_string
 *		Initialize a yvar structure with a character string value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Character string value, or NULL.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_const_string(yvar_t *var, const char *value);
/**
 * @function	yvar_new_string
 *		Create a new character string yvar.
 * @param	value	Character string value, or NULL to create a new empty string.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_string(ystr_t value);
/**
 * @function	yvar_init_string
 *		Initialize a yvar structure with a character string value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Character string value, or NULL to create a new empty string.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_string(yvar_t *var, ystr_t value);
/**
 * @function	yvar_new_table
 *		Create a new table yvar.
 * @param	value	Table value, or NULL to create a new empty table.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_table(ytable_t *value);
/**
 * @function	yvar_init_table
 *		Initialize a yvar structure with a table value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Table value, or NULL to create a new empty table.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_table(yvar_t *var, ytable_t *value);
/**
 * @function	yvar_new_pointer
 *		Create a new pointer yvar.
 * @param	value	Pointer value.
 * @return	A pointer to the allocated yvar.
 */
yvar_t *yvar_new_pointer(void *value);
/**
 * @function	yvar_init_pointer
 *		Initialize a yvar structure with a pointer value.
 * @param	var	A pointer to the structure that must be initialized.
 * @param	value	Pointer value.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_pointer(yvar_t *var, void *value);
/**
 * @function	yvar_new_object
 *		Create a new object yvar.
 * @param	value		Pointer to an object.
 * @param	definition	Pointer to the object definition.
 * @return	A pointer to the initialized yvar.
 */
yvar_t *yvar_new_object(void *value, yvar_definition_t *definition);
/**
 * @function	yvar_init_object
 *		Initialize a yvar structure with an object value.
 * @param	var		A pointer to the structure that must be initialized.
 * @param	value		A pointer to the object.
 * @param	definition	Pointer to the object definition.
 * @return	A pointer to the initialized structure.
 */
yvar_t *yvar_init_object(yvar_t *var, void *value, yvar_definition_t *definition);
/**
 * @function	yvar_retain
 *		Increments the reference counter of a yvar.
 * @param	var	A pointer to the yvar.
 * @return	A pointer to the yvar.
 */
yvar_t *yvar_retain(yvar_t *var);
/**
 * @function	yvar_release
 *		Decrements the reference counter of a yvar.
 *		Frees the yvar if the counter reaches zero.
 * @param	var	A pointer to the yvar.
 * @return	A pointer to the yvar, or NULL if it has been freed.
 */
yvar_t *yvar_release(yvar_t *var);
/**
 * @function	yvar_clone
 *		Create a copy of a given yvar.
 * @param	var	A pointer to the yvar.
 * @return	A pointer to the newly allocated yvar.
 */
yvar_t *yvar_clone(yvar_t *var);
/**
 * @function	yvar_clone_copy
 * @abstract	Create a copy of a given yvar. Its content is copied, not duplicated.
 *		This function is useful to create an instanciated yvar from a static one.
 * @param	var	A pointer to the yvar.
 * @return	A pointer to the newly allocated yvar.
 */
yvar_t *yvar_clone_copy(yvar_t *var);

/* ********** DELETION ********** */
/**
 * @function	yvar_free
 *		Free the memory allocated for a yvar.
 *		The value must have been fetched and freed separately.
 * @param	var	A pointer to the allocated yvar.
 */
void yvar_free(yvar_t *var);
/**
 * @function	yvar_delete
 *		Recursively free a yvar.
 * @param	var	A pointer to the allocated yvar.
 */
void yvar_delete(yvar_t *var);

/* ********** TYPE ********** */
/**
 * @function	yvar_isset
 *		Tell if a yvar exists and is defined.
 * @param	var	A pointer to the yvar.
 * @return	True if the yvar exists and is defined.
 */
bool yvar_isset(const yvar_t *var);
/**
 * @function	yvar_type
 *		Return the type of a yvar.
 * @param	var	A pointer to the yvar.
 * @return	The yvar type.
 */
yvar_type_t yvar_type(const yvar_t *var);
/**
 * @function	yvar_is_a
 *		Tell if a yvar is of the given type.
 * @param	var	A pointer to the yvar.
 * @param	type	The type to check.
 * @return	True if the types are matching.
 */
bool yvar_is_a(const yvar_t *var, yvar_type_t type);
/** @function yvar_is_undef	Tell if a yvar is undef. */
bool yvar_is_undef(const yvar_t *var);
/** @function yvar_is_null	Tell if a yvar is null. */
bool yvar_is_null(const yvar_t *var);
/** @function yvar_is_bool	Tell if a yvar is a boolean value. */
bool yvar_is_bool(const yvar_t *var);
/** @function yvar_is_int	Tell if a yvar is an integer value. */
bool yvar_is_int(const yvar_t *var);
/** @function yvar_is_float	Tell if a yvar is a floating-point number value. */
bool yvar_is_float(const yvar_t *var);
/** @function yvar_is_const_binary	Tell if a yvar is a constant binary value. */
bool yvar_is_const_binary(const yvar_t *var);
/** @function yvar_is_binary	Tell if a yvar is a binary value. */
bool yvar_is_binary(const yvar_t *var);
/** @function yvar_is_const_string	Tell if a yvar is a constant character string value. */
bool yvar_is_const_string(const yvar_t *var);
/** @function yvar_is_string	Tell if a yvar is a character string value. */
bool yvar_is_string(const yvar_t *var);
/** @function yvar_is_table	Tell if a yvar is a table value. */
bool yvar_is_table(const yvar_t *var);
/** @function yvar_is_array	Tell if a yvar is a table value used as an array. */
bool yvar_is_array(const yvar_t *var);
/** @function yvar_is_pointer	Tell if a yvar is a pointer value. */
bool yvar_is_pointer(const yvar_t *var);
/** @function yvar_is_object	Tell if a yvar is an object value. */
bool yvar_is_object(const yvar_t *var);

/* ********** CAST ********** */
/**
 * @function	yvar_cast_to_bool
 *		Cast a yvar to a boolean value.
 * @param	var	A pointer to the yvar.
 * @return	YENOERR if OK.
 */
ystatus_t yvar_cast_to_bool(yvar_t *var);
/** @function yvar_cast_to_int	Cast a yvar to an integer value. */
ystatus_t yvar_cast_to_int(yvar_t *var);
/** @function yvar_cast_to_float	Cast a yvar to a floating-point number value. */
ystatus_t yvar_cast_to_float(yvar_t *var);
/** @function yvar_cast_to_string	Cast a yvar to a character string value. */
ystatus_t yvar_cast_to_string(yvar_t *var);

/* ********** GETTERS ********** */
/**
 * @function	yvar_get_bool
 *		Return the boolean value of a yvar.
 * @param	var	A pointer to the yvar.
 * @return	The result.
 */
bool yvar_get_bool(const yvar_t *var);
/** @function yvar_get_int	Return the integer value of a yvar. */
int64_t yvar_get_int(const yvar_t *var);
/** @function yvar_get_float	Return the floating-point number value of a yvar. */
double yvar_get_float(const yvar_t *var);
/** @function yvar_get_const_string	Return the const string value of a yvar. */
const char *yvar_get_const_string(const yvar_t *var);
/** @function yvar_get_string	Return the string value of a yvar. */
ystr_t yvar_get_string(const yvar_t *var);
/** @function yvar_get_table	Return the table value of a yvar. */
ytable_t *yvar_get_table(const yvar_t *var);
/** @function yvar_get_pointer	Return the pointer value of a yvar. */
void *yvar_get_pointer(const yvar_t *var);

/* ********** PATH ********** */
/**
 * @function	yvar_get_from_path
 *		Return a value from a yvar root element and a path (similar to XPath).
 * @param	root	JSON root element.
 * @param	path	Path selector, similar to XPath.
 * @return	The selectedd value.
 */
yvar_t *yvar_get_from_path(yvar_t *root, const char *path);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif /* __cplusplus || c_plusplus */

