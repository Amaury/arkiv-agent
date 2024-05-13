/**
 * @header	api.h
 * @abstract	Arkiv API management.
 * @author	Amaury Bouchard <amaury@amaury.net>
 */
#pragma once

#include "ystatus.h"
#include "yvar.h"
#include "agent.h"

/**
 * @function	api_server_declare
 * @abstract	Declare the current server to arkiv.sh.
 * @param	hostname	Server name.
 * @param	orgKey		Organization key.
 * @return	YENOERR if the declaration went well.
 */
ystatus_t api_server_declare(const char *hostname, const char *orgKey);
/**
 * @function	api_get_params_file
 * @abstract	Fetch a host's parameters file.
 * @param	agent	Pointer to the agent structure.
 * @return	The deserialized JSON content, or NULL if an error occurred.
 */
yvar_t *api_get_params_file(agent_t *agent);

/* ********** PRIVATE DECLARATIONS ********** */
#ifdef __A_API_PRIVATE__
	/**
	 * @typedef	api_get_param_t
	 * @abstract	Structure used for GET parameters construction.
	 * @field	offset	Offset of the parameter.
	 * @field	url	Constructed URL.
	 */
	typedef struct {
		uint16_t offset;
		ystr_t   *url;
	} api_get_param_t;

	/**
	 * @function	api_call
	 * @abstract	Do a web request using wget or curl.
	 * @param	url	URL with no protocol (the 'https://' protocol will be added).
	 * @param	user	Username (or NULL if no authentication is required).
	 * @param	pwd	Password (or NULL if no authentication is required).
	 * @param	params	GET parameters (or NULL if no parameters).
	 * @param	asJson	True to process the response as a JSON stream.
	 * @return	The result of the request. If the request is successful, the status is YENOERR.
	 *		The value is a pointer to a yvar (a string or the result of the JSON deserialization).
	 */
	static yres_pointer_t api_call(const char *url, const char *user, const char *pwd,
	                               ytable_t *params, bool asJson);
	/**
	 * @function	api_curl
	 * @abstract	Do a web request using curl.
	 * @param	url	URL with no protocol (the 'https://' protocol will be added) but with the GET parameters.
	 * @param	user	Username (or NULL if no authentication is required).
	 * @param	pwd	Password (or NULL if no authentication is required).
	 * @return	The result of the request. If the request is successful, the status is YENOERR.
	 */
	static yres_bin_t api_curl(const char *url, const char *user, const char *pwd);
	/**
	 * @function	api_wget
	 * @abstract	Do a web request using wget.
	 * @param	url	URL with no protocol (the 'https://' protocol will be added) but with the GET parameters.
	 * @param	user	Username (or NULL if no authentication is required).
	 * @param	pwd	Password (or NULL if no authentication is required).
	 * @return	The result of the request. If the request is successful, the status is YENOERR.
	 */
	static yres_bin_t api_wget(const char *url, const char *user, const char *pwd);
	/**
	 * @function	api_url_add_param
	 *		Function used to add a GET parameter to an URL.
	 * @param	hash		Not used.
	 * @param	key		Name of the GET parameter.
	 * @param	data		Value of the GET parameter.
	 * @param	user_data	Pointer to a structure with the built URL and the parameter's offset.
	 */
	static ystatus_t api_url_add_param(uint64_t hash, char *key, void *data, void *user_data);
#endif // __A_API_PRIVATE__

