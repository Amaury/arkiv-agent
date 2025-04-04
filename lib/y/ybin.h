/*!
 * @header	ybin.h
 * @abstract	Basic definitions for binary data manipulation.
 * @author	Amaury Bouchard <amaury@amaury.net>
 */
#pragma once

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif /* __cplusplus || c_plusplus */

#include <stdlib.h>
#include <stdbool.h>
#include "ystatus.h"

/**
 * @typedef	ybin_t
 *		Structure used for binary data transmission.
 * @field	data		Pointer to data itselves.
 * @field	bytesize	Size of data.
 * @field	buffer_size	Size of the allocated buffer (different if bufferized).
 */
typedef struct {
	void *data;
	size_t bytesize;
	size_t buffer_size;
} ybin_t;

/**
 * @function	ybin_new
 *		Create a new ybin.
 * @return	An allocated ybin_t.
 */
ybin_t *ybin_new(void);
/**
 * @function	ybin_create
 *		Create a new ybin and set its content.
 *		The pointed data is copied, not just the pointer.
 * @param	data		Pointer to some data.
 * @param	bytesize	Length of the data in bytes.
 * @return	An allocated ybin_t.
 */
ybin_t *ybin_create(void *data, size_t bytesize);
/**
 * @function	ybin_create_bufferized
 *		Create a new ybin and set its content.
 *		The pointed data is copied in a larger buffer.
 * @param	data		Pointer to some data.
 * @param	bytesize	Length of the data in bytes.
 * @return	An allocated ybin_t.
 */
ybin_t *ybin_create_bufferized(void *data, size_t bytesize);
/**
 * @function	ybin_init
 *		Initialize a ybin. The given data is copied.
 * @param	bin		Pointer to an allocated ybin_t.
 * @param	data		Pointer to some data.
 * @param	bytesize	Length of the data in bytes.
 * @return	YENOERR if OK.
 */
ystatus_t ybin_init(ybin_t *bin, void *data, size_t bytesize);
/**
 * @function	ybin_init_bufferized
 *		Initialize a ybin. The given data is copied in a larger buffer.
 * @param	bin		Pointer to an allocated ybin_t.
 * @param	data		Pointer to some data.
 * @param	bytesize	Length of the data in bytes.
 * @return	YENOERR if OK.
 */
ystatus_t ybin_init_bufferized(ybin_t *bin, void *data, size_t bytesize);
/**
 * @function	ybin_clone
 *		Clone a ybin and its content.
 * @param	bin	A pointer to an allocated ybin.
 * @return	An allocated ybin_t.
 */
ybin_t *ybin_clone(ybin_t *bin);
/**
 * @function	ybin_empty
 * @abstract	Tell if a ybin is empty.
 * @param	bin	Pointer to a ybin_t.
 * @return	True if the ybin is empty.
 */
bool ybin_empty(ybin_t *bin);
/**
 * @function	ybin_copy
 *		Copy the content of a ybin inside another one.
 * @param	source	A pointer to the source ybin.
 * @param	dest	A pointer to the destination ybin.
 */
void ybin_copy(ybin_t *source, ybin_t *dest);
/**
 * @function	ybin_free
 *		Delete a ybin_t structure but not its enclosed data.
 * @param	bin	A pointer to a ybin_t.
 */
void ybin_free(ybin_t *bin);
/**
 * @function	ybin_delete
 *		Delete a ybin_t and its enclosed data.
 * @param	bin	A pointer to a ybin_t.
 */
void ybin_delete(ybin_t *bin);
/**
 * @function	ybin_delete_data
 *		Delete the enclosed data of a ybin but not the ybin itself.
 * @param	bin	A pointer to a ybin_t.
 */
void ybin_delete_data(ybin_t *bin);
/**
 * @function	ybin_set
 *		Set a ybin data pointer (data is not copied). Existing pointer is freed.
 * @param	bin		Pointer to a ybin_t.
 * @param	data		Pointer to some data.
 * @param	bytesize	Data length.
 */
void ybin_set(ybin_t *bin, void *data, size_t bytesize);
/**
 * @function	ybin_reset
 *		Reset a ybin_t.
 * @param	bin	A pointer to a ybin_t.
 */
void ybin_reset(ybin_t *bin);
/**
 * @function	ybin_append
 *		Add data at the end of a ybin_t.
 * @param	bin		A pointer to a ybin_t.
 * @param	data		A pointer to some data to add.
 * @param	bytesize	Data length.
 * @return	YENOERR if OK.
 */
ystatus_t ybin_append(ybin_t *bin, void *data, size_t bytesize);
/**
 * @function	ybin_prepend
 *		Add data at the beginning of a ybin_t.
 * @param	bin		A pointer to a ybin_t.
 * @param	data		A pointer to some data to add.
 * @param	bytesize	Data length.
 * @return	YENOERR if OK.
 */
ystatus_t ybin_prepend(ybin_t *bin, void *data, size_t bytesize);
/**
 * @function	ybin_set_nullend
 * @abstract	Add a '\0' character at the end of a binary, to be able to use it as a string.
 * @param	bin	A pointer to a ybin_t.
 */
void ybin_set_nullend(ybin_t *bin);
/**
 * @function	ybin_to_string
 * @abstract	Generates an allocated string from a ybin. The enclosed data must be UTF8-compatible.
 * @param	bin	A pointer to a ybin_t.
 * @return	An allocated string, or NULL.
 */
char *ybin_to_string(ybin_t *bin);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif /* __cplusplus || c_plusplus */

