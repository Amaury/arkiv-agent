#include "yvar.h"
#include "yfile.h"
#include "yarray.h"
#include "yjson.h"
#include "yresult.h"
#include "yexec.h"
#include "configuration.h"
#include "api.h"
#include "agent.h"

/** @define _ARKIV_USER_AGENT	User-agent of the arkiv agent. */
#define	_ARKIV_USER_AGENT	"Arkiv/1.0"
/** @define _ARKIV_URL_DECLARE	URL for server declaration. */
#define	_ARKIV_URL_DECLARE	"api.arkiv.sh/server/declare"

/* Do a web request. */
static yres_var_t _api_call(const char *url, const char *user, const char *pwd, ytable_t *params, bool asJson);
/* Do a web request using curl. */
static yres_bin_t _api_curl(const char *url, const char *user, const char *pwd);
/* Do a web request using wget. */
static yres_bin_t _api_wget(const char *url, const char *user, const char *pwd);
 /* Function used to add a GET parameter to an URL. */
ystatus_t _api_url_add_param(uint64_t hash, char *key, void *data, void *user_data);

/* ********** PUBLIC FUNCTIONS ********** */
/* Declare the current server to arkiv.sh. */
ystatus_t api_server_declare(const char *hostname, const char *orgKey) {
	ystatus_t st = YENOMEM;
	ystr_t agentVersion = NULL;
	yvar_t var = {0};
	// GET parameter
	ytable_t *params = ytable_create(1, NULL, NULL);
	if (!params)
		goto cleanup;
	ys_printf(&agentVersion, "%f", A_AGENT_VERSION);
	ytable_set_key(params, "version", agentVersion);
	// API call
	yres_var_t res = _api_call(A_API_URL_SERVER_DECLARE, hostname, orgKey, params, true);
	st = YRES_STATUS(res);
	var = YRES_VAL(res);
	if (st != YENOERR)
		goto cleanup;
	if (!yvar_isset(&var) || !yvar_is_bool(&var) || !yvar_get_bool(&var)) {
		st = YEUNDEF;
		goto cleanup;
	}
cleanup:
	ys_free(agentVersion);
	ytable_free(params);
	yvar_delete(&var);
	return (st);
}

/* ********** STATIC FUNCTIONS ********** */
/**
 * @typedef	_api_get_param_t
 * @abstract	Structure used for GET parameters construction.
 * @field	offset	Offset of the parameter.
 * @field	url	Constructed URL.
 */
typedef struct {
	uint16_t offset;
	ystr_t   *url;
} _api_get_param_t;
/**
 * @function	_api_call
 * @abstract	Do a web request using wget or curl.
 * @param	url	URL with no protocol (the 'https://' protocol will be added).
 * @param	user	Username (or NULL if no authentication is required).
 * @param	pwd	Password (or NULL if no authentication is required).
 * @param	params	GET parameters (or NULL if no parameters).
 * @param	asJson	True to process the response as a JSON stream.
 * @return	The result of the request. If the request is successful, the status is YENOERR.
 *		The value is a string or the result of the JSON deserialization.
 */
static yres_var_t _api_call(const char *url, const char *user, const char *pwd, ytable_t *params, bool asJson) {
	ystr_t fullUrl = ys_new(url);
	yres_bin_t res = {0};
	ybin_t responseBin = {0};
	ystr_t responseStr = NULL;
	yvar_t responseVar = {0};
	yjson_parser_t *jsonParser = NULL;
	yres_var_t result = {0};

	// add GET parameters to the URL string
	if (ytable_length(params)) {
		_api_get_param_t foreachParam = {
			.offset = 0,
			.url = &fullUrl,
		};
		ytable_foreach(params, _api_url_add_param, (void*)&foreachParam);
	}
	// call the external program
	if (config_program_exists("curl"))
		res = _api_curl(url, user, pwd);
	else if (config_program_exists("wget"))
		res = _api_wget(url, user, pwd);
	else {
		result = YRESULT_ERR(yres_var_t, YENOEXEC);
		goto cleanup;
	}
	responseBin = YRES_VAL(res);
	// manage error
	if (YRES_STATUS(res) != YENOERR) {
		result = YRESULT_ERR(yres_var_t, YRES_STATUS(res));
		goto cleanup;
	}
	// process the response
	if (!asJson) {
		// returns a ystring
		if (!(responseStr = ys_copy(responseBin.data))) {
			result = YRESULT_ERR(yres_var_t, YENOMEM);
			goto cleanup;
		}
		yvar_init_string(&responseVar, responseStr);
		result = YRESULT_VAL(yres_var_t, responseVar);
	} else {
		// returns the deserialized JSON stream
		if (!(jsonParser = yjson_new())) {
			result = YRESULT_ERR(yres_var_t, YENOMEM);
			goto cleanup;
		}
		result = yjson_parse(jsonParser, (char*)responseBin.data);
	}
cleanup:
	ys_free(fullUrl);
	ybin_delete_data(&responseBin);
	yjson_free(jsonParser);
	return (result);
}
/**
 * @function	_api_curl
 * @abstract	Do a web request using curl.
 * @param	url	URL with no protocol (the 'https://' protocol will be added) but with the GET parameters.
 * @param	user	Username (or NULL if no authentication is required).
 * @param	pwd	Password (or NULL if no authentication is required).
 * @return	The result of the request. If the request is successful, the status is YENOERR.
 */
static yres_bin_t _api_curl(const char *url, const char *user, const char *pwd) {
	yres_bin_t result = {0};
	ystr_t curlPath = NULL;
	ystr_t fileContent = NULL;
	char *tmpPath = NULL;
	yarray_t args = NULL;
	ybin_t responseBin = {0};

	// check parmeter
	if (!url || !strlen(url))
		return (YRESULT_ERR(yres_bin_t, YEPARAM));
	// find curl
	curlPath = config_get_program_path("curl");
	if (!curlPath)
		return (YRESULT_ERR(yres_bin_t, YENOEXEC));
	// create the file content
	fileContent = ys_printf(
		NULL,
		"url = \"%s\"\n"
		"user-agent = \"" _ARKIV_USER_AGENT "\"\n"
		"%s%s%s%s%s",
		url,
		(user && pwd) ? "user = \"" : "",
		(user && pwd) ? user : "",
		(user && pwd) ? ":" : "",
		(user && pwd) ? pwd : "",
		(user && pwd) ? "\"\n" : ""
	);
	// create temporary file
	tmpPath = yfile_tmp("/tmp/arkiv");
	if (!tmpPath) {
		result = YRESULT_ERR(yres_bin_t, YEIO);
		goto cleanup;
	}
	yfile_put_string(tmpPath, fileContent);
	// call curl
	args = yarray_create(2);
	yarray_push(&args, "--config");
	yarray_push(&args, tmpPath);
	ystatus_t status = yexec(curlPath, args, NULL, &responseBin, NULL);
	if (status == YENOERR) {
		result = YRESULT_VAL(yres_bin_t, responseBin);
	} else {
		result = YRESULT_ERR(yres_bin_t, status);
	}
cleanup:
	ys_free(curlPath);
	if (tmpPath)
		unlink(tmpPath);
	free0(tmpPath);
	ys_free(fileContent);
	yarray_free(args);
	return (result);

}
/**
 * @function	_api_wget
 * @abstract	Do a web request using wget.
 * @param	url	URL with no protocol (the 'https://' protocol will be added) but with the GET parameters.
 * @param	user	Username (or NULL if no authentication is required).
 * @param	pwd	Password (or NULL if no authentication is required).
 * @return	The result of the request. If the request is successful, the status is YENOERR.
 */
static yres_bin_t _api_wget(const char *url, const char *user, const char *pwd) {
	yres_bin_t result = {0};
	ystr_t wgetPath = NULL;
	ystr_t fullUrl = NULL;
	ystr_t ua = NULL;
	char *tmpPath = NULL;
	yarray_t args = NULL;
	ybin_t responseBin = {0};

	// check parmeter
	if (!url || !strlen(url))
		return (YRESULT_ERR(yres_bin_t, YEPARAM));
	// find wget
	wgetPath = config_get_program_path("wget");
	if (!wgetPath)
		return (YRESULT_ERR(yres_bin_t, YENOEXEC));
	// create the full URL string
	fullUrl = ys_printf(
		NULL,
		"https://%s%s%s%s%s",
		(user && pwd) ? user : "",
		(user && pwd) ? ":" : "",
		(user && pwd) ? pwd : "",
		(user && pwd) ? "@" : "",
		url
	);
	// create temporary file
	tmpPath = yfile_tmp("/tmp/arkiv");
	if (!tmpPath) {
		result = YRESULT_ERR(yres_bin_t, YEIO);
		goto cleanup;
	}
	yfile_put_string(tmpPath, fullUrl);
	// call wget
	ua = ys_printf(NULL, "--user-agent=\"%s\"", _ARKIV_USER_AGENT);
	args = yarray_create(7);
	if (!ua || !args) {
		result = YRESULT_ERR(yres_bin_t, YENOMEM);
		goto cleanup;
	}
	yarray_push(&args, "-nv");
	yarray_push(&args, "--auth-no-challenge");
	yarray_push(&args, ua);
	yarray_push(&args, "-i");
	yarray_push(&args, tmpPath);
	yarray_push(&args, "-O");
	yarray_push(&args, "-");
	ystatus_t status = yexec(wgetPath, args, NULL, &responseBin, NULL);
	if (status == YENOERR) {
		result = YRESULT_VAL(yres_bin_t, responseBin);
	} else {
		result = YRESULT_ERR(yres_bin_t, status);
	}
cleanup:
	ys_free(wgetPath);
	ys_free(ua);
	if (tmpPath)
		unlink(tmpPath);
	free0(tmpPath);
	ys_free(fullUrl);
	yarray_free(args);
	return (result);
}
/**
 * @function	_api_url_add_param
 *		Function used to add a GET parameter to an URL.
 * @param	
 */
ystatus_t _api_url_add_param(uint64_t hash, char *key, void *data, void *user_data) {
	if (!key || !data)
		return (YEINVAL);
	_api_get_param_t *param = user_data;
	ystatus_t status = ys_append(param->url, (param->offset ? "&" : "?"));
	if (status != YENOERR)
		return (status);
	if ((status = ys_append(param->url, key)) != YENOERR ||
	    (status = ys_append(param->url, "=")) != YENOERR)
		return (status);
	ystr_t encoded = ys_urlencode(data);
	if (!encoded)
		return (YENOMEM);
	if ((status = ys_append(param->url, encoded)) != YENOERR) {
		ys_free(encoded);
		return (status);
	}
	param->offset++;
	return (YENOERR);
}

