#define __A_API_PRIVATE__
#include "api.h"

#include "yansi.h"
#include "yvar.h"
#include "yfile.h"
#include "yarray.h"
#include "yjson.h"
#include "yresult.h"
#include "yexec.h"
#include "configuration.h"
#include "utils.h"
#include "log.h"
#include "agent.h"

/** @define _ARKIV_USER_AGENT	User-agent of the arkiv agent. */
#define	_ARKIV_USER_AGENT	"Arkiv/1.0"
/** @define _ARKIV_URL_DECLARE	URL for server declaration. */
#define	_ARKIV_URL_DECLARE	"api.arkiv.sh/server/declare"

/* ********** PUBLIC FUNCTIONS ********** */
/* Declare the current server to arkiv.sh. */
ystatus_t api_server_declare(const char *hostname, const char *orgKey) {
	ystatus_t st = YENOMEM;
	ystr_t agentVersion = NULL;
	yvar_t *var = NULL;
	// GET parameter
	ytable_t *params = ytable_create(1, NULL, NULL);
	if (!params)
		goto cleanup;
	ys_printf(&agentVersion, "%f", A_AGENT_VERSION);
	ytable_set_key(params, "version", agentVersion);
	// API call
	yres_pointer_t res = api_call(A_API_URL_SERVER_DECLARE, hostname, orgKey, params, true);
	st = YRES_STATUS(res);
	var = (yvar_t*)YRES_VAL(res);
	if (st != YENOERR)
		goto cleanup;
	if (!yvar_isset(var) || !yvar_is_bool(var) || !yvar_get_bool(var)) {
		st = YEUNDEF;
		goto cleanup;
	}
cleanup:
	ys_free(agentVersion);
	ytable_free(params);
	yvar_delete(var);
	return (st);
}
/* Fetch a host's parameters file. */
yvar_t *api_get_params_file(agent_t *agent) {
	// forge URL
	ystr_t url = ys_printf(NULL, A_API_URL_SERVER_PARAMS, agent->conf.org_key, agent->conf.hostname);
	ADEBUG("│ ├ " YANSI_FAINT "Download file: " YANSI_RESET "%s", url);
	// fetch file
	yres_pointer_t res = api_call(url, NULL, NULL, NULL, true);
	ys_free(url);
	ystatus_t st = YRES_STATUS(res);
	yvar_t *var = (yvar_t*)YRES_VAL(res);
	if (st != YENOERR) {
		ADEBUG("│ └ " YANSI_RED "Download error" YANSI_RESET);
		return (NULL);
	}
	ADEBUG("│ ├ " YANSI_FAINT "File downloaded" YANSI_RESET);
	if (!yvar_is_table(var)) {
		ADEBUG("│ └ " YANSI_RED "Bad file format" YANSI_RESET);
		yvar_release(var);
		return (NULL);
	}
	ADEBUG("│ └ " YANSI_FAINT "File deserialized" YANSI_RESET);
	return (var);
}

/* ********** STATIC FUNCTIONS ********** */
/* Do a web request. */
static yres_pointer_t api_call(const char *url, const char *user, const char *pwd, ytable_t *params, bool asJson) {
	ystr_t fullUrl = ys_new(url);
	yres_bin_t res = {0};
	ybin_t responseBin = {0};
	ystr_t responseStr = NULL;
	yvar_t *responseVar = NULL;
	yjson_parser_t *jsonParser = NULL;
	yres_pointer_t result = {0};

	// add GET parameters to the URL string
	if (ytable_length(params)) {
		api_get_param_t foreachParam = {
			.offset = 0,
			.url = &fullUrl,
		};
		ytable_foreach(params, api_url_add_param, (void*)&foreachParam);
	}
	// call the external program
	if (check_program_exists("curl"))
		res = api_curl(url, user, pwd);
	else if (check_program_exists("wget"))
		res = api_wget(url, user, pwd);
	else {
		result = YRESULT_ERR(yres_pointer_t, YENOEXEC);
		goto cleanup;
	}
	responseBin = YRES_VAL(res);
	// manage error
	if (YRES_STATUS(res) != YENOERR) {
		result = YRESULT_ERR(yres_pointer_t, YRES_STATUS(res));
		goto cleanup;
	}
	// process the response
	if (!asJson) {
		// returns a ystring
		if (!(responseStr = ys_copy(responseBin.data))) {
			result = YRESULT_ERR(yres_pointer_t, YENOMEM);
			goto cleanup;
		}
		responseVar = yvar_new_string(responseStr);
		result = YRESULT_VAL(yres_pointer_t, responseVar);
	} else {
		// returns the deserialized JSON stream
		if (!(jsonParser = yjson_new())) {
			result = YRESULT_ERR(yres_pointer_t, YENOMEM);
			goto cleanup;
		}
		responseVar = yjson_parse_simple(jsonParser, (char*)responseBin.data);
		if (responseVar) {
			result = YRESULT_VAL(yres_pointer_t, responseVar);
		} else {
			result = YRESULT_ERR(yres_pointer_t, YEUNDEF);
		}
	}
cleanup:
	ys_free(fullUrl);
	ybin_delete_data(&responseBin);
	yjson_free(jsonParser);
	return (result);
}
/* Do a web request using curl. */
static yres_bin_t api_curl(const char *url, const char *user, const char *pwd) {
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
	curlPath = get_program_path("curl");
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
/* Do a web request using wget. */
static yres_bin_t api_wget(const char *url, const char *user, const char *pwd) {
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
	wgetPath = get_program_path("wget");
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
 /* Function used to add a GET parameter to an URL. */
static ystatus_t api_url_add_param(uint64_t hash, char *key, void *data, void *user_data) {
	if (!key || !data)
		return (YEINVAL);
	api_get_param_t *param = user_data;
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

