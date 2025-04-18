#include "yansi.h"
#include "yexec.h"
#include "ystr.h"
#include "yjson.h"
#include "ydefs.h"
#include "log.h"

#define __A_UPLOAD_PRIVATE__
#include "upload.h"

/* Upload backed up files to cloud storage. */
void upload_files(agent_t *agent) {
	ystatus_t st_files = YENOERR;
	ystatus_t st_mysql = YENOERR, st_pgsql = YENOERR;
	ytable_function_t upload_callback = NULL;

	ALOG("Upload files to " YANSI_FAINT "%s" YANSI_RESET, agent->param.storage_name);
	// check storage parameters
	if (!agent->param.storage) {
		ALOG("└ " YANSI_RED "No parameters" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}
	// extract storage data
	ystr_t storage_type = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_TYPE));
	if (!storage_type || ys_empty(storage_type)) {
		ALOG("└ " YANSI_RED "No defined storage" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	} else if (!strcmp0(storage_type, A_STORAGE_TYPE_AWS_S3)) {
		agent->param.storage_env = upload_create_env_aws_s3(agent);
		upload_callback = upload_item_aws_s3;
	} else if (!strcmp0(storage_type, A_STORAGE_TYPE_SFTP)) {
		agent->param.storage_env = upload_create_env_sftp(agent);
		upload_callback = upload_item_sftp;
	}
	if (!agent->param.storage_env || !upload_callback) {
		ALOG("└ " YANSI_RED "Unknown storage type '" YANSI_RESET "%s" YANSI_RED "'" YANSI_RESET, storage_type);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}

	// upload backed up files
	if (!ytable_empty(agent->exec_log.backup_files)) {
		ADEBUG("├ " YANSI_FAINT "Upload backed up files" YANSI_RESET);
		st_files = ytable_foreach(agent->exec_log.backup_files, upload_callback, agent);
		if (st_files == YENOERR)
			ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
	}
	// upload backed up databases
	if (!ytable_empty(agent->exec_log.backup_databases)) {
		ADEBUG("├ " YANSI_FAINT "Upload backed up databases" YANSI_RESET);
		st_mysql = ytable_foreach(agent->exec_log.backup_databases, upload_callback, agent);
		if (st_mysql == YENOERR)
			ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
	}
	// log
	if (st_files == YENOERR && st_mysql == YENOERR && st_pgsql == YENOERR)
		ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	else
		ALOG("└ " YANSI_RED "Error" YANSI_RESET);

	// free environment
	void *pt;
	while ((pt = yarray_pop(agent->param.storage_env)))
		free0(pt);
	yarray_free(agent->param.storage_env);
}

/* ********** PRIVATE FUNCTIONS ********** */
/* Generates the list of environment variables for AWS S3 upload. */
static yarray_t upload_create_env_aws_s3(agent_t *agent) {
	yarray_t env = NULL;
	char *type = NULL, *provider = NULL, *acl = NULL, *access_key = NULL, *secret_key = NULL, *region = NULL;
	ystr_t ys = NULL;

	// creation of environment array
	if (!agent || !agent->param.storage || !(env = yarray_create(6)))
		return (NULL);
	// type, provider, acl
	if (!(type = strdup("RCLONE_CONFIG_STORAGE_TYPE=s3")) ||
	    !(provider = strdup("RCLONE_CONFIG_STORAGE_PROVIDER=AWS")) ||
	    !(acl = strdup("RCLONE_CONFIG_STORAGE_ACL=private")))
		goto cleanup;
	// access key
	if (!(ys = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_ACCESS_KEY))) ||
	    asprintf(&access_key, "RCLONE_CONFIG_STORAGE_ACCESS_KEY_ID=%s", ys) == -1) {
		access_key = NULL;
		goto cleanup;
	}
	// secret key
	if (!(ys = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_SECRET_KEY))) ||
	    asprintf(&secret_key, "RCLONE_CONFIG_STORAGE_SECRET_ACCESS_KEY=%s", ys) == -1) {
		secret_key = NULL;
		goto cleanup;
	}
	// region
	if (!(ys = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_REGION))) ||
	    asprintf(&region, "RCLONE_CONFIG_STORAGE_REGION=%s", ys) == -1) {
		region = NULL;
		goto cleanup;
	}
	// env creation
	ystatus_t st = yarray_push_multi(
		&env,
		6,
		type,
		provider,
		acl,
		access_key,
		secret_key,
		region
	);
	if (st == YENOERR)
		return (env);
cleanup:
	yarray_free(env);
	free0(type);
	free0(provider);
	free0(acl);
	free0(access_key);
	free0(secret_key);
	free0(region);
	return (NULL);
}
/* Upload a backed up file or database to AWS S3. */
static ystatus_t upload_item_aws_s3(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	log_item_t *item = (log_item_t*)data;
	agent_t *agent = (agent_t*)user_data;
	yarray_t args = NULL;
	ystr_t bucket = NULL;
	ystr_t root_path = NULL;
	ystr_t dest_path = NULL;

	if (!item->success)
		return (YENOERR);
	ADEBUG("│ ├ " YANSI_FAINT "Upload file " YANSI_RESET "%s", item->archive_path);
	// bucket
	bucket = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_BUCKET));
	if (!bucket || ys_empty(bucket)) {
		ADEBUG("│ ├ " YANSI_RED "No S3 bucket given." YANSI_RESET);
		status = YEBADCONF;
		goto cleanup;
	}
	// root path
	root_path = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_PATH));
	if (root_path) {
		// remove starting slashes
		while (!ys_empty(root_path) && root_path[0] == SLASH) {
			ys_lshift(root_path);
		}
		// remove trailing slashes
		while (!ys_empty(root_path) && root_path[ys_bytesize(root_path) - 1] == SLASH) {
			ys_rshift(root_path);
		}
		// check if the string is still not empty
		if (ys_empty(root_path)) {
			ys_free(root_path);
			root_path = NULL;
		}
	}
	// destination path
	dest_path = ys_printf(
		NULL,
		"storage:%s/%s%s%s/%s/%s/%s/%s",
		bucket,
		root_path ? root_path : "",
		root_path ? "/" : "",
		agent->param.org_name,
		agent->conf.hostname,
		agent->datetime_chunk_path,
		(item->type == A_ITEM_TYPE_FILE) ? "files" : "databases",
		item->archive_name
	);
	if (!dest_path) {
		ADEBUG("│ ├ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		item->upload_status = status;
		item->success = false;
		goto cleanup;
	}
	ADEBUG("│ │ └ " YANSI_FAINT "To " YANSI_RESET "%s", dest_path);
	// create argument list for backed up file
	if (!(args = yarray_create(2))) {
		ADEBUG("│ ├ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		item->upload_status = status;
		item->success = false;
		goto cleanup;
	}
	yarray_push_multi(
		&args,
		3,
		"copyto",
		item->archive_path,
		dest_path
	);
	// upload the backed up file
	status = yexec(A_EXE_RCLONE, args, agent->param.storage_env, NULL, NULL);
	if (status != YENOERR) {
		ADEBUG("│ └ " YANSI_RED "Failed" YANSI_RESET);
		item->upload_status = status;
		item->success = false;
		goto cleanup;
	}
	/* upload the checksum file */
	// destination path
	dest_path = ys_printf(
		&dest_path,
		"storage:%s/%s%s%s/%s/%s/%s/%s",
		bucket,
		root_path ? root_path : "",
		root_path ? "/" : "",
		agent->param.org_name,
		agent->conf.hostname,
		agent->datetime_chunk_path,
		(item->type == A_ITEM_TYPE_FILE) ? "files" : "databases",
		item->checksum_name
	);
	// create argument list for checksum file
	args[1] = item->checksum_path;
	args[2] = dest_path;
	// upload the checksum file
	ADEBUG("│ ├ " YANSI_FAINT "Upload checksum file " YANSI_RESET "%s", item->checksum_path);
	ADEBUG("│ │ └ " YANSI_FAINT "To " YANSI_RESET "%s", dest_path);
	status = yexec(A_EXE_RCLONE, args, agent->param.storage_env, NULL, NULL);
	if (status != YENOERR) {
		ADEBUG("│ └ " YANSI_RED "Failed" YANSI_RESET);
		item->upload_status = status;
		item->success = false;
		goto cleanup;
	}
	item->upload_status = YENOERR;
cleanup:
	ys_free(dest_path);
	return (status);
}
/* Generates the list of environment variables for SFTP upload. */
static yarray_t upload_create_env_sftp(agent_t *agent) {
	yarray_t env = NULL;
	char *type = NULL, *host = NULL, *port = NULL, *user = NULL, *pass = NULL, *keyfile = NULL;
	ystr_t ys = NULL;

	// creation of environment array
	if (!agent || !agent->param.storage || !(env = yarray_create(6)))
		return (NULL);
	// type
	if (!(type = strdup("RCLONE_CONFIG_STORAGE_TYPE=sftp")))
		goto cleanup;
	// host
	if (!(ys = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_HOST))) ||
	    asprintf(&host, "RCLONE_CONFIG_STORAGE_HOST=%s", ys) == -1) {
		host = NULL;
		goto cleanup;
	}
	// port
	if (!(ys = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_PORT))) ||
	    asprintf(&port, "RCLONE_CONFIG_STORAGE_PORT=%s", ys) == -1) {
		port = NULL;
		goto cleanup;
	}
	// user
	if (!(ys = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_USER))) ||
	    asprintf(&user, "RCLONE_CONFIG_STORAGE_USER=%s", ys) == -1) {
		user = NULL;
		goto cleanup;
	}
	// password
	if (!(ys = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_PWD))) ||
	    asprintf(&pass, "RCLONE_CONFIG_STORAGE_PASS=%s", ys) == -1) {
		pass = NULL;
	}
	// keyfile
	if (!(ys = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_KEYFILE))) ||
	    asprintf(&keyfile, "RCLONE_CONFIG_STORAGE_KEY_FILE=%s", ys) == -1) {
		pass = NULL;
	}
	// env creation
	ystatus_t st = yarray_push_multi(
		&env,
		4,
		type,
		host,
		port,
		user
	);
	if (st != YENOERR)
		goto cleanup;
	if (pass && yarray_push(&env, pass) != YENOERR)
		goto cleanup;
	if (keyfile && yarray_push(&env, keyfile) != YENOERR)
		goto cleanup;
	return (env);
cleanup:
	yarray_free(env);
	free0(type);
	free0(host);
	free0(port);
	free0(user);
	free0(pass);
	free0(keyfile);
	return (NULL);
}
/* Upload a backed up file or database using an SFTP connection. */
static ystatus_t upload_item_sftp(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	log_item_t *item = (log_item_t*)data;
	agent_t *agent = (agent_t*)user_data;
	yarray_t args = NULL;
	ystr_t root_path = NULL;
	ystr_t dest_path = NULL;

	ADEBUG("│ ├ " YANSI_FAINT "Upload file " YANSI_RESET "%s", item->archive_path);
	// root path
	root_path = yvar_get_string(ytable_get_key_data(agent->param.storage, A_PARAM_KEY_PATH));
	if (root_path) {
		// remove starting slashes
		while (!ys_empty(root_path) && root_path[0] == SLASH) {
			ys_lshift(root_path);
		}
		// remove trailing slashes
		while (!ys_empty(root_path) && root_path[ys_bytesize(root_path) - 1] == SLASH) {
			ys_rshift(root_path);
		}
		// check if the string is still not empty
		if (ys_empty(root_path)) {
			ys_free(root_path);
			root_path = NULL;
		}
	}
	// destination path
	dest_path = ys_printf(
		NULL,
		"storage:%s%s%s/%s/%s/%s/%s",
		root_path ? root_path : "",
		root_path ? "/" : "",
		agent->param.org_name,
		agent->conf.hostname,
		agent->datetime_chunk_path,
		(item->type == A_ITEM_TYPE_FILE) ? "files" : "databases",
		item->archive_name
	);
	if (!dest_path) {
		ADEBUG("│ ├ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		item->upload_status = status;
		item->success = false;
		goto cleanup;
	}
	// create argument list for backed up file
	if (!(args = yarray_create(2))) {
		ADEBUG("│ ├ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		item->upload_status = status;
		item->success = false;
		goto cleanup;
	}
	yarray_push_multi(
		&args,
		3,
		"copyto",
		item->archive_path,
		dest_path
	);
	// upload the backed up file
	status = yexec(A_EXE_RCLONE, args, agent->param.storage_env, NULL, NULL);
	if (status != YENOERR) {
		ADEBUG("│ └ " YANSI_RED "Failed" YANSI_RESET);
		item->upload_status = status;
		item->success = false;
		goto cleanup;
	}
	/* upload the checksum file */
	// destination path
	dest_path = ys_printf(
		&dest_path,
		"storage:%s%s%s/%s/%s/%s/%s",
		root_path ? root_path : "",
		root_path ? "/" : "",
		agent->param.org_name,
		agent->conf.hostname,
		agent->datetime_chunk_path,
		(item->type == A_ITEM_TYPE_FILE) ? "files" : "databases",
		item->checksum_name
	);
	// create argument list for checksum file
	args[1] = item->checksum_path;
	args[2] = dest_path;
	// upload the checksum file
	ADEBUG("│ ├ " YANSI_FAINT "Upload checksum file " YANSI_RESET "%s", item->checksum_path);
	status = yexec(A_EXE_RCLONE, args, agent->param.storage_env, NULL, NULL);
	if (status != YENOERR) {
		ADEBUG("│ └ " YANSI_RED "Failed" YANSI_RESET);
		item->upload_status = status;
		item->success = false;
		goto cleanup;
	}
	item->upload_status = YENOERR;
cleanup:
	ys_free(dest_path);
	return (status);
}

