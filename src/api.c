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
ystatus_t api_server_declare(agent_t *agent) {
	char *hostname = agent->conf.hostname;
	char *orgKey = agent->conf.org_key;
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
	if (agent->debug_mode)
		printf("URL called: %s\n", A_API_URL_SERVER_DECLARE);
	yres_pointer_t res = api_call(A_API_URL_SERVER_DECLARE, hostname, orgKey, params, NULL, true);
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
/** Send the report of a backup. */
ystatus_t api_backup_report(agent_t *agent) {
	ystatus_t st = YENOERR;
	bool st_global = true, st_pre = true, st_post = true, st_files = true, st_db = true;
	yvar_t *report = yvar_new_table(NULL);
	if (!report)
		return (YENOMEM);
	ytable_t *root = yvar_get_table(report);
	yvar_t *var = NULL;

	// timestamp
	if (!(var = yvar_new_int((uint64_t)agent->exec_timestamp))) {
		st = YENOMEM;
		goto cleanup;
	}
	ytable_set_key(root, "t", var);
	// compression type
	char *z = (agent->param.compression == A_COMP_GZIP) ? "g" :
	          (agent->param.compression == A_COMP_BZIP2) ? "b" :
	          (agent->param.compression == A_COMP_XZ) ? "x" :
	          (agent->param.compression == A_COMP_ZSTD) ? "s" : "n";
	if (!(var = yvar_new_const_string(z))) {
		st = YENOMEM;
		goto cleanup;
	}
	if ((st = ytable_set_key(root, "z", var)) != YENOERR)
		goto cleanup;
	var = NULL;
	// encryption type
	char *e = (agent->param.encryption == A_CRYPT_OPENSSL) ? "o" :
	          (agent->param.encryption == A_CRYPT_SCRYPT) ? "s" :
	          (agent->param.encryption == A_CRYPT_GPG) ? "g" : "u";
	if (!(var = yvar_new_const_string(e))) {
		st = YENOMEM;
		goto cleanup;
	}
	if ((st = ytable_set_key(root, "e", var)) != YENOERR)
		goto cleanup;
	// retention
	if (agent->param.retention_type != A_RETENTION_INFINITE &&
	    agent->param.retention_duration) {
		char *rt = (agent->param.retention_type == A_RETENTION_DAYS) ? "d" :
		           (agent->param.retention_type == A_RETENTION_WEEKS) ? "w" :
		           (agent->param.retention_type == A_RETENTION_MONTHS) ? "m" : "y";
		if (!(var = yvar_new_const_string(rt))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "rt", var)) != YENOERR)
			goto cleanup;
		if (!(var = yvar_new_int((int64_t)agent->param.retention_duration))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "rd", var)) != YENOERR)
			goto cleanup;
	}
	// storage ID
	if (!(var = yvar_new_int(agent->param.storage_id))) {
		st = YENOMEM;
		goto cleanup;
	}
	if ((st = ytable_set_key(root, "st", var)) != YENOERR)
		goto cleanup;
	// pre-scripts
	if (!ytable_empty(agent->exec_log.pre_scripts)) {
		if (!(var = yvar_new_table(NULL))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "pre", var)) != YENOERR)
			goto cleanup;
		void *user_data[2] = {(void*)var, (void*)&st_pre};
		ytable_foreach(agent->exec_log.pre_scripts, api_report_process_script, user_data);
	}
	// post-scripts
	if (!ytable_empty(agent->exec_log.post_scripts)) {
		if (!(var = yvar_new_table(NULL))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "post", var)) != YENOERR)
			goto cleanup;
		void *user_data[2] = {(void*)var, (void*)&st_post};
		ytable_foreach(agent->exec_log.post_scripts, api_report_process_script, user_data);
	}
	// files
	if (!ytable_empty(agent->exec_log.backup_files)) {
		if (!(var = yvar_new_table(NULL))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "files", var)) != YENOERR)
			goto cleanup;
		void *user_data[2] = {(void*)var, (void*)&st_files};
		ytable_foreach(agent->exec_log.backup_files, api_report_process_item, user_data);
	}
	// databases
	if (!ytable_empty(agent->exec_log.backup_databases)) {
		if (!(var = yvar_new_table(NULL))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "db", var)) != YENOERR)
			goto cleanup;
		void *user_data[2] = {(void*)var, (void*)&st_db};
		ytable_foreach(agent->exec_log.backup_databases, api_report_process_item, user_data);
	}
	// statuses
	if (!st_pre || !st_post || !st_files || !st_db)
		st_global = false;
	if (!(var = yvar_new_bool(st_global))) {
		st = YENOMEM;
		goto cleanup;
	}
	if ((st = ytable_set_key(root, "st_global", var)) != YENOERR)
		goto cleanup;
	if (!ytable_empty(agent->exec_log.pre_scripts)) {
		if (!(var = yvar_new_bool(st_pre))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "st_pre", var)) != YENOERR)
			goto cleanup;
	}
	if (!ytable_empty(agent->exec_log.post_scripts)) {
		if (!(var = yvar_new_bool(st_post))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "st_post", var)) != YENOERR)
			goto cleanup;
	}
	if (!ytable_empty(agent->exec_log.backup_files)) {
		if (!(var = yvar_new_bool(st_files))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "st_files", var)) != YENOERR)
			goto cleanup;
	}
	if (!ytable_empty(agent->exec_log.backup_databases)) {
		if (!(var = yvar_new_bool(st_db))) {
			st = YENOMEM;
			goto cleanup;
		}
		if ((st = ytable_set_key(root, "st_db", var)) != YENOERR)
			goto cleanup;
	}
	// API call
	if (agent->debug_mode) {
		ystr_t ys = yjson_sprint(report, true);
		if (ys) {
			ADEBUG("│ └ " YANSI_FAINT "Report:\n" YANSI_RESET YANSI_YELLOW "%s" YANSI_RESET, ys);
			ys_free(ys);
		}
	}
	yres_pointer_t res = api_call(
		A_API_URL_BACKUP_REPORT,
		agent->conf.hostname,
		agent->conf.org_key,
		NULL,
		report,
		true
	);
	st = YRES_STATUS(res);
	var = (yvar_t*)YRES_VAL(res);
	if (st != YENOERR)
		goto cleanup;
	if (!yvar_isset(var) || !yvar_is_bool(var) || !yvar_get_bool(var)) {
		st = YEUNDEF;
		goto cleanup;
	}
cleanup:
	yvar_delete(report);
	return (st);
}
/* Fetch a host's parameters file. */
yvar_t *api_get_params_file(agent_t *agent) {
	// forge URL
	ystr_t url = ys_printf(NULL, A_API_URL_SERVER_PARAMS, agent->conf.org_key, agent->conf.hostname);
	ADEBUG("│ ├ " YANSI_FAINT "Download file: " YANSI_RESET "%s", url);
	// fetch file
	yres_pointer_t res = api_call(url, NULL, NULL, NULL, NULL, true);
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
static yres_pointer_t api_call(const char *url, const char *user, const char *pwd,
                               const ytable_t *params, const yvar_t *post_data, bool asJson) {
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
	if (check_program_exists("curl")) {
		res = api_curl(url, post_data, user, pwd);
	} else if (check_program_exists("wget")) {
		res = api_wget(url, post_data, user, pwd);
	} else {
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
		ybin_set_nullend(&responseBin);
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
static yres_bin_t api_curl(const char *url, const yvar_t *post_data, const char *user, const char *pwd) {
	yres_bin_t result = {0};
	ystr_t curlPath = NULL;
	ystr_t fileContent = NULL;
	char *configFilePath = NULL;
	char *postFilePath = NULL;
	ystr_t postFileOption = NULL;
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
	configFilePath = yfile_tmp("/tmp/arkiv");
	if (!configFilePath) {
		result = YRESULT_ERR(yres_bin_t, YEIO);
		goto cleanup;
	}
	yfile_put_string(configFilePath, fileContent);
	// create POST data temporary file
	if (post_data) {
		postFilePath = yfile_tmp("/tmp/arkiv");
		postFileOption = ys_printf(NULL, "@%s", postFilePath);
		if (!postFilePath || !postFileOption) {
			result = YRESULT_ERR(yres_bin_t, YEIO);
			goto cleanup;
		}
		yjson_write(postFilePath, post_data, false);
	}
	// create argument list
	args = yarray_create(6);
	if (post_data) {
		yarray_push(&args, "-X");
		yarray_push(&args, "POST");
		yarray_push(&args, "--data-binary");
		yarray_push(&args, postFileOption);
	}
	yarray_push(&args, "--config");
	yarray_push(&args, configFilePath);
	// call curl
	ystatus_t status = yexec(curlPath, args, NULL, &responseBin, NULL);
	if (status == YENOERR) {
		result = YRESULT_VAL(yres_bin_t, responseBin);
	} else {
		result = YRESULT_ERR(yres_bin_t, status);
	}
cleanup:
	ys_free(curlPath);
	if (configFilePath)
		unlink(configFilePath);
	free0(configFilePath);
	if (postFilePath)
		unlink(postFilePath);
	free0(postFilePath);
	ys_free(postFileOption);
	ys_free(fileContent);
	yarray_free(args);
	return (result);
}
/* Do a web request using wget. */
static yres_bin_t api_wget(const char *url, const yvar_t *post_data, const char *user, const char *pwd) {
	yres_bin_t result = {0};
	ystr_t wgetPath = NULL;
	ystr_t fullUrl = NULL;
	char *urlFilePath = NULL;
	char *postFilePath = NULL;
	yarray_t args = NULL;
	ybin_t responseBin = {0};
	bool https = true;
	const char *usedUrl = url;
	ystatus_t status;

	// check parmeter
	if (!url || !strlen(url))
		return (YRESULT_ERR(yres_bin_t, YEPARAM));
	// find wget
	wgetPath = get_program_path("wget");
	if (!wgetPath)
		return (YRESULT_ERR(yres_bin_t, YENOEXEC));
	// check for HTTP or HTTPS
	if (!strncmp0(url, "http://", 7)) {
		https = false;
		usedUrl = url + 7;
	} else {
		usedUrl = url + 8;
	}
	// create the full URL string
	fullUrl = ys_printf(
		NULL,
		"%s://%s%s%s%s%s",
		https ? "https" : "http",
		(user && pwd) ? user : "",
		(user && pwd) ? ":" : "",
		(user && pwd) ? pwd : "",
		(user && pwd) ? "@" : "",
		usedUrl
	);
	// create URL temporary file
	urlFilePath = yfile_tmp("/tmp/arkiv");
	if (!urlFilePath) {
		result = YRESULT_ERR(yres_bin_t, YEIO);
		goto cleanup;
	}
	yfile_put_string(urlFilePath, fullUrl);
	// create POST data temporary file
	if (post_data) {
		if (!(postFilePath = yfile_tmp("/tmp/arkiv"))) {
			result = YRESULT_ERR(yres_bin_t, YEIO);
			goto cleanup;
		}
		if ((status = yjson_write(postFilePath, post_data, false)) != YENOERR) {
			result = YRESULT_ERR(yres_bin_t, status);
			goto cleanup;
		}
	}
	// create argument list
	args = yarray_create(10);
	yarray_push(&args, "-nv");
	yarray_push(&args, "--auth-no-challenge");
	yarray_push(&args, "-U");
	yarray_push(&args, _ARKIV_USER_AGENT);
	if (post_data) {
		yarray_push(&args, "--post-file");
		yarray_push(&args, postFilePath);
	}
	yarray_push(&args, "-i");
	yarray_push(&args, urlFilePath);
	yarray_push(&args, "-O");
	yarray_push(&args, "-");
	// call wget
	status = yexec(wgetPath, args, NULL, &responseBin, NULL);
	if (status == YENOERR) {
		result = YRESULT_VAL(yres_bin_t, responseBin);
	} else {
		result = YRESULT_ERR(yres_bin_t, status);
	}
cleanup:
	ys_free(wgetPath);
	if (urlFilePath)
		unlink(urlFilePath);
	free0(urlFilePath);
	if (postFilePath)
		unlink(postFilePath);
	free0(postFilePath);
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
/* Add a script to the report. */
static ystatus_t api_report_process_script(uint64_t hash, char *key, void *data, void *user_data) {
	void **user_data_array = (void**)user_data;
	yvar_t *var = (yvar_t*)user_data_array[0];
	ytable_t *scripts = yvar_get_table(var);
	ystatus_t *st_script = (ystatus_t*)user_data_array[1];
	log_script_t *entry = (log_script_t*)data;

	if (!entry->success)
		*st_script = false;
	if (!(var = yvar_new_bool(entry->success)))
		return (YENOMEM);
	return (ytable_set_key(scripts, entry->command, var));
}
/** Add a file or database to the report. */
static ystatus_t api_report_process_item(uint64_t hash, char *key, void *data, void *user_data) {
	void **user_data_array = (void**)user_data;
	yvar_t *var = (yvar_t*)user_data_array[0];
	ytable_t *items = yvar_get_table(var);
	bool *st_items = (bool*)user_data_array[1];
	log_item_t *item = (log_item_t*)data;

	if (item->success) {
		// item successfully backed up
		if (!(var = yvar_new_bool(true)))
			return (YENOMEM);
	} else {
		// there was an error during item's backup
		*st_items = false;
		if (item->dump_status != YENOERR && item->compress_status != YENOERR &&
		    item->encrypt_status != YENOERR && item->checksum_status != YENOERR &&
		    item->upload_status != YENOERR) {
			// complete failure
			if (!(var = yvar_new_bool(false)))
				return (YENOMEM);
		} else {
			// incomplete failure
			ystr_t ys = ys_new("");
			if (!ys)
				return (YENOMEM);
			if (item->dump_status == YENOERR)
				ys_addc(&ys, 'd');
			if (item->compress_status == YENOERR)
				ys_addc(&ys, 'z');
			if (item->encrypt_status == YENOERR)
				ys_addc(&ys, 'e');
			if (item->checksum_status == YENOERR)
				ys_addc(&ys, 'c');
			if (item->upload_status == YENOERR)
				ys_addc(&ys, 'u');
			if (!(var = yvar_new_string(ys)))
				return (YENOMEM);
		}
	}
	return (ytable_set_key(items, item->item, var));
}

