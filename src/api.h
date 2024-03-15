/**
 * @header	api.h
 * @abstract	Arkiv API management.
 * @author	Amaury Bouchard <amaury@amaury.net>
 */
#pragma once

#include "ystatus.h"

/**
 * @function	api_server_declare
 *		Declare the current server to arkiv.sh.
 * @param	hostname	Server name.
 * @param	orgKey		Organization key.
 * @return	YENOERR if the declaration went well.
 */
ystatus_t api_server_declare(const char *hostname, const char *orgKey);

