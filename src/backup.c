#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "yansi.h"
#include "ytable.h"
#include "yvar.h"
#include "yfile.h"
#include "yexec.h"
#include "log.h"
#include "api.h"
#include "utils.h"
#include "upload.h"

#define __A_BACKUP_PRIVATE__
#include "backup.h"

/* List of days of the week. */
static const char *DAYS_OF_THE_WEEK[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat", "sun"};

/* Main backup function. */
void exec_backup(agent_t *agent) {
	ystatus_t st;

	ALOG_RAW(YANSI_NEGATIVE "------------------------- AGENT EXECUTION -------------------------" YANSI_RESET);
	/* local programs */
	// get find path
	if (!(agent->bin.find = get_program_path("find"))) {
		ALOG("Search local programs");
		ALOG("└ " YANSI_RED "Unable to find " YANSI_RESET "find" YANSI_RED " program" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}
	// get tar path
	if (!(agent->bin.tar = get_program_path("tar"))) {
		ALOG("Search local programs");
		ALOG("└ " YANSI_RED "Unable to find " YANSI_RESET "tar" YANSI_RED " program" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}
	// get sha512sum path
	if (!(agent->bin.checksum = get_program_path("sha512sum"))) {
		ALOG("Search local programs");
		ALOG("└ " YANSI_RED "Unable to find " YANSI_RESET "sha512sum" YANSI_RED " program" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}
	// get database dump programs path
	agent->bin.mysqldump = get_program_path("mysqldump");
	agent->bin.pg_dump = get_program_path("pg_dump");
	agent->bin.pg_dumpall = get_program_path("pg_dumpall");
	// check rclone
	if (!yfile_is_executable(A_EXE_RCLONE)) {
		ALOG("Search local programs");
		ALOG("└ " YANSI_RED "Unable to find " YANSI_RESET A_EXE_RCLONE YANSI_RED " program" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}
	ADEBUG("Search local programs");
	ADEBUG("└ " YANSI_GREEN "Done" YANSI_RESET);

	// fetch parameters file
	st = backup_fetch_params(agent);
	if (st != YENOERR && st != YEAGAIN) {
		ALOG(YANSI_BG_RED "Abort" YANSI_RESET);
		return;
	}
	ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);

	/* purge old local archives */
	if (backup_purge_local(agent) != YENOERR) {
		ALOG(YANSI_BG_RED "Abort" YANSI_RESET);
		return;
	}
	// quit if there is nothing to backup
	if (st == YEAGAIN) {
		ALOG(YANSI_GREEN "✓ End of processing" YANSI_RESET);
		return;
	}

	ALOG("Start backup");
	// create output directory
	if (backup_create_output_directory(agent) != YENOERR) {
		ALOG(YANSI_BG_RED "Abort" YANSI_RESET);
		return;
	}
	// change working directory
	if (chdir(agent->backup_path)) {
		ALOG("└ " YANSI_RED "Unable to change working directory to '" YANSI_RESET "%s" YANSI_RED "'" YANSI_RESET, agent->backup_path);
		ALOG(YANSI_BG_RED "Abort" YANSI_RESET);
		return;
	}
	// execute pre-scripts
	if (backup_exec_scripts(agent, A_SCRIPT_TYPE_PRE) == YENOERR) {
		// backup files
		backup_files(agent);
		// backup databases
		backup_databases(agent);
		// encrypt files
		backup_encrypt_files(agent);
		// compute checksums
		backup_compute_checksums(agent);
		// upload files
		upload_files(agent);
	}
	// send report
	ALOG("Send report to arkiv.sh");
	st = api_backup_report(agent);
	if (st == YENOERR)
		ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	else if (st == YENOMEM)
		ALOG("└ " YANSI_RED "Failed (memory allocation error)" YANSI_RESET);
	else if (st == YENOEXEC)
		ALOG("└ " YANSI_RED "Failed (can't find curl nor wget)" YANSI_RESET);
	else if (st == YEFAULT)
		ALOG("└ " YANSI_RED "Failed (communication error" YANSI_RESET);
	else
		ALOG("└ " YANSI_RED "Failed" YANSI_RESET);
	// execute post-scripts
	if (backup_exec_scripts(agent, A_SCRIPT_TYPE_POST) != YENOERR) {
		return;
	}
}

/* ********** PRIVATE FUNCTIONS ********** */
/* Purge local archive files. */
static ystatus_t backup_purge_local(agent_t *agent) {
	ystatus_t status = YENOERR;
	yarray_t args = NULL;
	ystr_t ys = NULL;

	ALOG("Purge local archives");
	if (!(args = yarray_create(6))) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return (YENOMEM);
	}
	// check if files may be purged
	if (!yfile_is_dir(agent->conf.archives_path)) {
		ADEBUG("├ " YANSI_FAINT "No directory " YANSI_RESET "%s", agent->conf.archives_path);
		ALOG("└ " YANSI_GREEN "Pass" YANSI_RESET);
		return (YENOERR);
	}
	// removes files older than 24 hours
	ys = ys_printf(NULL, "+%d", (agent->param.local_retention_hours * 60));
	ADEBUG("├ " YANSI_FAINT "Delete archives older than %d hours" YANSI_RESET, agent->param.local_retention_hours);
	yarray_push_multi(
		&args,
		6,
		agent->conf.archives_path,
		"-type",
		"f",
		"-mmin",
		ys,
		"-delete"
	);
	status = yexec(agent->bin.find, args, NULL, NULL, NULL);
	if (status == YENOERR) {
		ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
	} else {
		ALOG("└ " YANSI_RED "Error" YANSI_RESET);
		goto cleanup;
	}
	// removes empty directories
	ADEBUG("├ " YANSI_FAINT "Delete empty archive folders" YANSI_RESET);
	yarray_trunc(args, NULL, NULL);
	yarray_push_multi(
		&args,
		5,
		agent->conf.archives_path,
		"-type",
		"d",
		"-empty",
		"-delete"
	);
	status = yexec(agent->bin.find, args, NULL, NULL, NULL);
	if (status == YENOERR) {
		ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
	} else {
		ALOG("└ " YANSI_RED "Execution error" YANSI_RESET);
		goto cleanup;
	}
	ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
cleanup:
	ys_free(ys);
	yarray_free(args);
	return (status);
}
/* Fetch and process host backup parameter file. */
static ystatus_t backup_fetch_params(agent_t *agent) {
	ALOG("Fetch host parameters");
	// fetch params file
	yvar_t *params = api_get_params_file(agent);
	if (!params) {
		ALOG("└ " YANSI_RED "Failed (unable to download or deserialize the file)" YANSI_RESET);
		return (YEBADCONF);
	}
	/* extract parameters */
	yvar_t *var_ptr;
	// extract organization name
	var_ptr = yvar_get_from_path(params, A_PARAM_PATH_NAME);
	agent->param.org_name = yvar_get_string(var_ptr);
	if (!agent->param.org_name) {
		ALOG("└ " YANSI_RED "Failed (unable to find organization name)" YANSI_RESET);
		return (YEBADCONF);
	}
	// extract encryption algorithm
	ADEBUG("├ " YANSI_FAINT "Search for matching encryption method:" YANSI_RESET);
	var_ptr = yvar_get_from_path(params, A_PARAM_PATH_ENCRYPTION_STRING);
	ystr_t enc_string = yvar_get_string(var_ptr);
	if (!enc_string) {
		ALOG("└ " YANSI_RED "Failed (wrongly formatted file: no encryption)" YANSI_RESET);
		return (YEBADCONF);
	}
	agent->param.encryption = A_CRYPT_UNDEF;
	while (!ys_empty(enc_string)) {
		char c = ys_lshift(enc_string);
		if (c == A_CHAR_CRYPT_GPG &&
		    (agent->bin.crypt = get_program_path("gpg"))) {
			agent->param.encryption = A_CRYPT_GPG;
			ADEBUG("│ └ gpg");
			break;
		} else if (c == A_CHAR_CRYPT_SCRYPT &&
		           (agent->bin.crypt = get_program_path("scrypt"))) {
			agent->param.encryption = A_CRYPT_SCRYPT;
			ADEBUG("│ └ scrypt");
			break;
		} else if (c == A_CHAR_CRYPT_OPENSSL &&
		           (agent->bin.crypt = get_program_path("openssl"))) {
			agent->param.encryption = A_CRYPT_OPENSSL;
			ADEBUG("│ └ openssl");
			break;
		}
	}
	if (agent->param.encryption == A_CRYPT_UNDEF) {
		ALOG("└ " YANSI_RED "Failed (no encryption available)" YANSI_RESET);
		return (YEBADCONF);
	}
	// extract compression algorithm
	ADEBUG("├ " YANSI_FAINT "Search for matching compression method:" YANSI_RESET);
	var_ptr = yvar_get_from_path(params, A_PARAM_PATH_COMPRESSION_STRING);
	ystr_t z_string = yvar_get_string(var_ptr);
	if (!z_string) {
		ALOG("└ " YANSI_RED "Failed (wrongly formatted file: no compression)" YANSI_RESET);
		return (YEBADCONF);
	}
	agent->param.compression = A_COMP_NONE;
	while (!ys_empty(z_string)) {
		char c = ys_lshift(z_string);
		if (c == A_CHAR_COMP_NONE) {
			ADEBUG("│ └ none");
			break;
		} else if (c == A_CHAR_COMP_ZSTD && check_program_exists("unzstd") &&
		           (agent->bin.z = get_program_path("zstd"))) {
			agent->param.compression = A_COMP_ZSTD;
			ADEBUG("│ └ zstd");
			break;
		} else if (c == A_CHAR_COMP_XZ && check_program_exists("unxz") &&
		           (agent->bin.z = get_program_path("xz"))) {
			agent->param.compression = A_COMP_XZ;
			ADEBUG("│ └ xz");
			break;
		} else if (c == A_CHAR_COMP_BZIP2 && check_program_exists("bunzip2") &&
		           (agent->bin.z = get_program_path("bzip2"))) {
			agent->param.compression = A_COMP_BZIP2;
			ADEBUG("│ └ bzip2");
			break;
		} else if (c == A_CHAR_COMP_GZIP && check_program_exists("gunzip") &&
		           (agent->bin.z = get_program_path("gzip"))) {
			agent->param.compression = A_COMP_GZIP;
			ADEBUG("│ └ gzip");
			break;
		}
	}
	// extract local retention
	var_ptr = yvar_get_from_path(params, A_PARAM_PATH_RETENTION_HOURS);
	int64_t retention_int = yvar_get_int(var_ptr);
	if (!var_ptr || retention_int <= 0 || retention_int > UINT16_MAX) {
		ALOG("├ " YANSI_YELLOW "No local retention duration value. Use default value (%d hours)." YANSI_RESET, A_DEFAULT_LOCAL_RETENTION);
		agent->param.local_retention_hours = A_DEFAULT_LOCAL_RETENTION;
	} else
		agent->param.local_retention_hours = (uint16_t)retention_int;

	// extract schedules
	var_ptr = yvar_get_from_path(params, A_PARAM_PATH_SCHEDULES);
	ytable_t *schedules = yvar_get_table(var_ptr);
	if (!schedules) {
		ALOG("└ " YANSI_RED "Failed (wrongly formatted file: no schedule)" YANSI_RESET);
		return (YEBADCONF);
	}
	// get execution day and time
	struct tm *tm = localtime(&agent->exec_timestamp);
	if (tm->tm_wday < 0 || tm->tm_wday > 6) {
		ALOG("└ " YANSI_RED "Failed (unable to compute current day and time)" YANSI_RESET);
		return (YEBADCONF);
	}
	// search the current schedule
	ystr_t varpath = ys_printf(NULL, "/%s/%02d", DAYS_OF_THE_WEEK[tm->tm_wday], tm->tm_hour);
	if (!varpath) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return (YENOMEM);
	}
	ADEBUG("├ " YANSI_FAINT "Search schedule for current execution day and time (" YANSI_RESET "%s" YANSI_FAINT ")" YANSI_RESET, varpath);
	yvar_t *schedule = yvar_get_from_path(var_ptr, varpath);
	ys_free(varpath);
	if (!schedule) {
		ALOG("├ " YANSI_FAINT "No backup scheduled for this day/time" YANSI_RESET);
		return (YEAGAIN);
	}
	ADEBUG("│ └ " YANSI_FAINT "Schedule found" YANSI_RESET);
	// from the schedule, get the retention
	ADEBUG("├ " YANSI_FAINT "From the schedule, extract the retention type" YANSI_RESET);
	yvar_t *var_ptr2 = yvar_get_from_path(schedule, A_PARAM_PATH_RETENTION_TYPE);
	yvar_t *var_ptr3 = yvar_get_from_path(schedule, A_PARAM_PATH_RETENTION_DURATION);
	ystr_t ret_type;
	uint64_t ret_duration;
	if (var_ptr2 && yvar_is_string(var_ptr2) && (ret_type = yvar_get_string(var_ptr2)) &&
	    !ys_empty(ret_type) &&
	    var_ptr3 && yvar_is_int(var_ptr3) && (ret_duration = yvar_get_int(var_ptr3)) &&
	    ret_duration) {
		agent->param.retention_type = (ret_type[0] == 'd') ? A_RETENTION_DAYS :
		                              (ret_type[0] == 'w') ? A_RETENTION_WEEKS :
		                              (ret_type[0] == 'm') ? A_RETENTION_MONTHS :
		                              (ret_type[0] == 'y') ? A_RETENTION_YEARS :
		                              A_RETENTION_INFINITE;
		if (agent->param.retention_type != A_RETENTION_INFINITE)
			agent->param.retention_duration = ret_duration;
	}
	// from the schedule, get the savepack ID
	ADEBUG("├ " YANSI_FAINT "From the schedule, extract the savepack ID" YANSI_RESET);
	var_ptr2 = yvar_get_from_path(schedule, A_PARAM_PATH_SAVEPACKS);
	if (!var_ptr2 || !yvar_is_int(var_ptr2)) {
		ALOG("└ " YANSI_RED "Failed (wrongly formatted file: no schedule savepack)" YANSI_RESET);
		return (YEBADCONF);
	}
	int64_t savepack_id = yvar_get_int(var_ptr2);
	ADEBUG("│ └ " YANSI_FAINT "Savepack ID: " YANSI_RESET "%" PRId64, savepack_id);
	agent->param.savepack_id = savepack_id;
	// from the schedule, get the storage ID
	ADEBUG("├ " YANSI_FAINT "From the schedule, extract the storage ID" YANSI_RESET);
	var_ptr2 = yvar_get_from_path(schedule, A_PARAM_PATH_STORAGES);
	if (!var_ptr2 || !yvar_is_int(var_ptr2)) {
		ALOG("└ " YANSI_RED "Failed (wrongly formatted file: no schedule storage)" YANSI_RESET);
		return (YEBADCONF);
	}
	int64_t storage_id = yvar_get_int(var_ptr2);
	ADEBUG("│ └ " YANSI_FAINT "Storage ID: " YANSI_RESET "%" PRId64, storage_id);
	agent->param.storage_id = storage_id;

	// search the savepack
	ADEBUG("├ " YANSI_FAINT "Search for the savepack from its ID" YANSI_RESET);
	varpath = ys_printf(NULL, "%s/%" PRId64, A_PARAM_PATH_SAVEPACKS, savepack_id);
	if (!varpath) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return (YENOMEM);
	}
	yvar_t *savepack = yvar_get_from_path(params, varpath);
	ys_free(varpath);
	if (!savepack) {
		ALOG("└ " YANSI_RED "Unable to find savepack (ID %" PRId64 ")" YANSI_RESET, savepack_id);
		return (YEBADCONF);
	}
	ADEBUG("│ ├ " YANSI_FAINT "Savepack found" YANSI_RESET);
	// get pre-scripts
	var_ptr2 = yvar_get_from_path(savepack, A_PARAM_PATH_PRE);
	if (var_ptr2 && yvar_is_table(var_ptr2)) {
		agent->param.pre_scripts = yvar_get_table(var_ptr2);
		ADEBUG("│ ├ %d" YANSI_FAINT " pre-script(s)" YANSI_RESET, ytable_length(agent->param.pre_scripts));
	} else
		ADEBUG("│ ├ " YANSI_FAINT "No pre-script" YANSI_RESET);
	// get post-scripts
	var_ptr2 = yvar_get_from_path(savepack, A_PARAM_PATH_POST);
	if (var_ptr2 && yvar_is_table(var_ptr2)) {
		agent->param.post_scripts = yvar_get_table(var_ptr2);
		ADEBUG("│ ├ %d" YANSI_FAINT " post-script(s)" YANSI_RESET, ytable_length(agent->param.post_scripts));
	} else
		ADEBUG("│ ├ " YANSI_FAINT "No post-script" YANSI_RESET);
	// get MySQL databases
	var_ptr2 = yvar_get_from_path(savepack, A_PARAM_PATH_MYSQL);
	if (var_ptr2 && yvar_is_table(var_ptr2)) {
		agent->param.mysql = yvar_get_table(var_ptr2);
		ADEBUG("│ ├ %d" YANSI_FAINT " MySQL database(s)" YANSI_RESET, ytable_length(agent->param.mysql));
	} else
		ADEBUG("│ ├ " YANSI_FAINT "No MySQL database" YANSI_RESET);
	// get PostgreSQL databases
	var_ptr2 = yvar_get_from_path(savepack, A_PARAM_PATH_PGSQL);
	if (var_ptr2 && yvar_is_table(var_ptr2)) {
		agent->param.pgsql = yvar_get_table(var_ptr2);
		ADEBUG("│ ├ %d" YANSI_FAINT " PostgreSQL database(s)" YANSI_RESET, ytable_length(agent->param.pgsql));
	} else
		ADEBUG("│ ├ " YANSI_FAINT "No PostgreSQL database" YANSI_RESET);
	// get files
	var_ptr2 = yvar_get_from_path(savepack, A_PARAM_PATH_FILE);
	if (var_ptr2 && yvar_is_table(var_ptr2)) {
		agent->param.files = yvar_get_table(var_ptr2);
		ADEBUG("│ └ %d" YANSI_FAINT " file(s)" YANSI_RESET, ytable_length(agent->param.files));
	} else
		ADEBUG("│ └ " YANSI_FAINT "No file" YANSI_RESET);

	// search the storage
	ADEBUG("├ " YANSI_FAINT "Search for the storage from its ID" YANSI_RESET);
	varpath = ys_printf(NULL, "%s/%" PRId64, A_PARAM_PATH_STORAGES, storage_id);
	if (!varpath) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return (YENOMEM);
	}
	yvar_t *storage = yvar_get_from_path(params, varpath);
	ys_free(varpath);
	// store storage data
	if (!storage || !yvar_is_table(storage) || !(agent->param.storage = yvar_get_table(storage))) {
		ALOG("└ " YANSI_RED "Unable to find storage (ID %" PRId64 ")" YANSI_RESET, storage_id);
		return (YEBADCONF);
	}
	// get storage name
	var_ptr2 = yvar_get_from_path(storage, A_PARAM_PATH_NAME);
	if (!var_ptr2 || !yvar_is_string(var_ptr2) || !(agent->param.storage_name = yvar_get_string(var_ptr2))) {
		ALOG("└ " YANSI_RED "Unable to find storage name (ID %" PRId64 ")" YANSI_RESET, storage_id);
		return (YEBADCONF);
	}
	ADEBUG("│ └ " YANSI_FAINT "Storage found: " YANSI_RESET "%s", agent->param.storage_name);
	
	return (YENOERR);
}
/* Create output directory. */
static ystatus_t backup_create_output_directory(agent_t *agent) {
	struct tm tm = *gmtime(&agent->exec_timestamp);

	// generate path
	agent->datetime_chunk_path = ys_printf(NULL, "%04d-%02d-%02d/%02d:00", tm.tm_year + 1900,
	                                       tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
	agent->backup_path = ys_printf(NULL, "%s/%s", agent->conf.archives_path, agent->datetime_chunk_path);
	if (!agent->datetime_chunk_path || !agent->backup_path) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return (YENOMEM);
	}
	ALOG("├ " YANSI_FAINT "Create output directory " YANSI_RESET "%s", agent->backup_path);
	// create directory
	if (!yfile_mkpath(agent->backup_path, 0700)) {
		ALOG("└ " YANSI_RED "Failed" YANSI_RESET);
		return (YEIO);
	}
	ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	return (YENOERR);
}
/* Execute each script in the list of pre- or post-scripts. */
static ystatus_t backup_exec_scripts(agent_t *agent, script_type_t type) {
	ystatus_t st;
	ytable_t *scripts;
	ytable_function_t callback;

	// use the pre- or post-list
	if (type == A_SCRIPT_TYPE_PRE) {
		scripts = agent->param.pre_scripts;
		callback = backup_exec_pre_script;
	} else {
		scripts = agent->param.post_scripts;
		callback = backup_exec_post_script;
	}
	// check if there are some scripts to execute
	if (!scripts || !ytable_length(scripts))
		return (YENOERR);
	// log message
	if (type == A_SCRIPT_TYPE_PRE)
		ALOG("Execute pre-scripts");
	else
		ALOG("Execute post-scripts");
	// process all scripts
	st = ytable_foreach(scripts, callback, agent);
	if (st == YENOERR)
		ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	else
		ALOG("└ " YANSI_RED "Failed" YANSI_RESET);
	return (st);
}
/* Execute a pre-script. */
static ystatus_t backup_exec_pre_script(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	yvar_t *var_script = (yvar_t*)data;
	agent_t *agent = user_data;
	ystr_t command;
	log_script_t *log = NULL;

	// checks
	if (!yvar_is_string(var_script) || !(command = yvar_get_string(var_script))) {
		ALOG("└ " YANSI_RED "Failed (bad parameter)" YANSI_RESET "\n");
		status = YEBADCONF;
		goto cleanup;
	}
	ALOG("├ " YANSI_FAINT "Execution of " YANSI_RESET "%s", command);
	// creation of the log entry
	if (!(log = log_create_pre_script(agent, command))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		goto cleanup;
	}
	// script execution
	int ret_val = system(command);
	if (ret_val == -1 || !WIFEXITED(ret_val) || WEXITSTATUS(ret_val)) {
		ADEBUG("│ └ " YANSI_RED "Failed (returned value " YANSI_RESET "%d" YANSI_RED ")" YANSI_RESET, WEXITSTATUS(ret_val));
		status = YEFAULT;
		goto cleanup;
	}
	ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
cleanup:
	if (status != YENOERR)
		agent->exec_log.status_pre_scripts = false;
	if (log)
		log->success = (status == YENOERR) ? true : false;
	return (status);
}
/* Execute a post-script. */
static ystatus_t backup_exec_post_script(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	yvar_t *var_script = (yvar_t*)data;
	agent_t *agent = user_data;
	ystr_t command;
	log_script_t *log = NULL;

	// checks
	if (!yvar_is_string(var_script) || !(command = yvar_get_string(var_script))) {
		ALOG("└ " YANSI_RED "Failed (bad parameter)" YANSI_RESET "\n");
		status = YEBADCONF;
		goto cleanup;
	}
	ALOG("├ " YANSI_FAINT "Execution of " YANSI_RESET "%s", command);
	// creation of the log entry
	if (!(log = log_create_post_script(agent, command))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		goto cleanup;
	}
	// script execution
	int ret_val = system(command);
	if (ret_val) {
		ADEBUG("│ └ " YANSI_RED "Failed (returned value " YANSI_RESET "%d" YANSI_RED ")" YANSI_RESET, ret_val);
		status = YEFAULT;
		goto cleanup;
	}
	ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
cleanup:
	if (status != YENOERR)
		agent->exec_log.status_post_scripts = false;
	if (log)
		log->success = (status == YENOERR) ? true : false;
	return (status);
}
/* Backup all listed files. They are tar'ed and compressed. */
static void backup_files(agent_t *agent) {
	ystatus_t st;

	// check if there is something to backup
	if (!agent->param.files || !ytable_length(agent->param.files))
		return;
	// log message
	ALOG("Backup files");
	// create output subdirectory
	agent->backup_files_path = ys_printf(NULL, "%s/files", agent->backup_path);
	if (!agent->backup_files_path) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return;
	}
	ADEBUG("├ " YANSI_FAINT "Create directory " YANSI_RESET "%s", agent->backup_files_path);
	if (!yfile_mkpath(agent->backup_files_path, 0700)) {
		ALOG("└ " YANSI_RED "Failed" YANSI_RESET);
		return;
	}
	// tar and compress all files
	st = ytable_foreach(agent->param.files, backup_file, agent);
	if (st == YENOERR)
		ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	else
		ALOG("└ " YANSI_YELLOW "Error" YANSI_RESET);
}
/* Backup one file. */
static ystatus_t backup_file(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	yvar_t *var_file_path = (yvar_t*)data;
	agent_t *agent = (agent_t*)user_data;
	ystr_t file_path = NULL;
	log_item_t *log = NULL;
	ystr_t filename = NULL;
	yarray_t args = NULL;
	char *tmp_file = NULL;

	// checks
	if (!yvar_is_string(var_file_path) || !(file_path = yvar_get_string(var_file_path))) {
		ALOG("└ " YANSI_RED "Failed (bad parameter)" YANSI_RESET);
		status = YEBADCONF;
		goto cleanup;
	}
	// log message
	ALOG("├ " YANSI_FAINT "Backup path " YANSI_RESET "%s", file_path);
	// creation of the log entry
	if (!(log = log_create_file(agent, file_path))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		goto cleanup;
	}
	// remove '/' at the beginning of the path to save, and call tar with "-C /" options
	// to avoid the warning "Removing leading `/' from member names"
	char *path = file_path;
	while (*path == '/')
		path++;
	// create tar command
	if (!(filename = ys_filenamize_path(path, ",")) ||
	    !(log->archive_name = ys_printf(NULL, "%s.tar", filename)) ||
	    !(log->archive_path = ys_printf(NULL, "%s/%s", agent->backup_files_path, log->archive_name)) ||
	    !(args = yarray_create(9))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = log->dump_status = YENOMEM;
		goto cleanup;
	}
	if (!(tmp_file = yfile_tmp(log->archive_path))) {
		ALOG("│ └ " YANSI_RED "Unable to create temporary file" YANSI_RESET);
		status = log->dump_status = YEIO;
		goto cleanup;
	}
	yarray_push_multi(
		&args,
		9,
		"cf",
		tmp_file,
		"--exclude-caches",
		"--exclude-tag=.arkiv-exclude",
		"--exclude-ignore=.arkiv-ignore",
		"--exclude-ignore-recursive=.arkiv-ignore-recursive",
		"-C",
		"/",
		path
	);
	// execution
	ADEBUG("│ ├ " YANSI_FAINT "Tar " YANSI_RESET "%s" YANSI_FAINT " to " YANSI_RESET "%s", file_path, log->archive_path);
	status = yexec(agent->bin.tar, args, NULL, NULL, NULL);
	if (status != YENOERR) {
		ALOG("│ └ " YANSI_RED "Tar error" YANSI_RESET);
		log->dump_status = status;
		goto cleanup;
	}
	// set log status
	log->dump_status = YENOERR;
	// move the file to its destination
	if (rename(tmp_file, log->archive_path)) {
		ALOG("│ └ " YANSI_RED "Unable to move file " YANSI_RESET "%s" YANSI_RED " to " YANSI_RED "%s" YANSI_RESET, tmp_file, log->archive_path);
		status = log->dump_status = YEIO;
		unlink(tmp_file);
		goto cleanup;
	}
	// compression of the file
	status = backup_compress_file(agent, log);
	// get archive file's size
	log->archive_size = yfile_get_size(log->archive_path);
cleanup:
	if (status != YENOERR)
		agent->exec_log.status_files = false;
	if (log)
		log->success = (status == YENOERR) ? true : false;
	ys_free(filename);
	yarray_free(args);
	free0(tmp_file);
	return (status);
}
/* Backup all listed databases. They are tar'ed and compressed. */
static void backup_databases(agent_t *agent) {
	ystatus_t st_mysql = YENOERR, st_pgsql = YENOERR;

	/* MySQL */
	if (ytable_length(agent->param.mysql)) {
		// log message
		ALOG("Backup MySQL databases");
		// check if mysqldump is available
		if (!agent->bin.mysqldump) {
			ALOG("└ " YANSI_YELLOW "Error" YANSI_RESET " mysqldump " YANSI_YELLOW "is not installed" YANSI_RESET);
			st_mysql = YENOEXEC;
		} else {
			// create output subdirectory
			agent->backup_mysql_path = ys_printf(NULL, "%s/mysql", agent->backup_path);
			if (!agent->backup_mysql_path) {
				ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
				return;
			}
			ADEBUG("├ " YANSI_FAINT "Create directory " YANSI_RESET "%s", agent->backup_mysql_path);
			if (!yfile_mkpath(agent->backup_mysql_path, 0700)) {
				ALOG("└ " YANSI_RED "Failed" YANSI_RESET);
				return;
			}
			// dump databases
			st_mysql = ytable_foreach(agent->param.mysql, backup_mysql, agent);
			if (st_mysql == YENOERR)
				ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
			else if (st_mysql != YEBADCONF)
				ALOG("└ " YANSI_YELLOW "Error" YANSI_RESET);
		}
	}
	/* PostgreSQL */
	if (ytable_length(agent->param.pgsql)) {
		// log message
		ALOG("Backup PostgreSQL databases");
		// check if pg_dump is available
		if (!agent->bin.mysqldump) {
			ALOG("└ " YANSI_YELLOW "Error" YANSI_RESET " mysqldump " YANSI_YELLOW "is not installed" YANSI_RESET);
			st_pgsql = YENOEXEC;
		} else {
			// create output subdirectory
			agent->backup_pgsql_path = ys_printf(NULL, "%s/postgresql", agent->backup_path);
			if (!agent->backup_pgsql_path) {
				ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
				return;
			}
			ADEBUG("├ " YANSI_FAINT "Create directory " YANSI_RESET "%s", agent->backup_pgsql_path);
			if (!yfile_mkpath(agent->backup_pgsql_path, 0700)) {
				ALOG("└ " YANSI_RED "Failed" YANSI_RESET);
				return;
			}
			// dump databases
			st_pgsql = ytable_foreach(agent->param.pgsql, backup_pgsql, agent);
			if (st_pgsql == YENOERR)
				ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
			else if (st_pgsql != YEBADCONF)
				ALOG("└ " YANSI_YELLOW "Error" YANSI_RESET);
		}
	}
	// database status
	agent->exec_log.status_databases = (st_mysql == YENOERR && st_pgsql == YENOERR) ? true : false;
}
/* Backup a MySQL database. */
static ystatus_t backup_mysql(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	yvar_t *var_db_data = (yvar_t*)data;
	agent_t *agent = (agent_t*)user_data;
	ytable_t *db_data = NULL;
	log_item_t *log = NULL;
	bool all_databases = false;
	ystr_t dbname = NULL;
	ystr_t dbuser = NULL;
	ystr_t dbpwd = NULL;
	ystr_t dbhost = NULL;
	int64_t dbport = 0;
	ystr_t dbport_str = NULL;
	ystr_t filename = NULL;
	ystr_t password_env = NULL;
	yarray_t args = NULL;
	yarray_t env = NULL;
	char *tmp_file = NULL;

	// check mysqldump
	if (!agent->bin.mysqldump) {
		ALOG("└ " YANSI_RED "Failed (mysqldump not installed)" YANSI_RESET);
		status = YENOEXEC;
		goto cleanup;
	}
	// extract parameters and check them
	if (!yvar_is_table(var_db_data) || !(db_data = yvar_get_table(var_db_data)) ||
	    !(dbname = yvar_get_string(ytable_get_key_data(db_data, A_PARAM_KEY_DB))) ||
	    !(dbuser = yvar_get_string(ytable_get_key_data(db_data, A_PARAM_KEY_USER))) ||
	    !(dbpwd = yvar_get_string(ytable_get_key_data(db_data, A_PARAM_KEY_PWD))) ||
	    !(dbhost = yvar_get_string(ytable_get_key_data(db_data, A_PARAM_KEY_HOST))) ||
	    !(dbport = yvar_get_int(ytable_get_key_data(db_data, A_PARAM_KEY_PORT)))) {
		ALOG("└ " YANSI_RED "Failed (bad parameter)" YANSI_RESET);
		status = YEBADCONF;
		goto cleanup;
	}
	if (!strcmp(dbname, A_DB_ALL_DATABASES_DEFINITION))
		all_databases = true;
	// log message
	ALOG("├ " YANSI_FAINT "Database " YANSI_RESET "%s", dbname);
	// creation of the log entry
	if (!(log = log_create_mysql(agent, dbname))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		goto cleanup;
	}
	log->success = true;
	// create mysqldump command
	if (all_databases)
		filename = ys_new(A_DB_ALL_DATABASES_FILENAME);
	else
		filename = ys_filenamize(dbname);
	if (!filename ||
	    !(log->archive_name = ys_printf(NULL, "%s.sql", filename)) ||
	    !(log->archive_path = ys_printf(NULL, "%s/%s", agent->backup_mysql_path, log->archive_name)) ||
	    !(dbport_str = ys_printf(NULL, "%d", (int)dbport)) ||
	    !(password_env = ys_printf(NULL, "MYSQL_PWD=%s", dbpwd)) ||
	    !(args = yarray_create(11)) ||
	    !(env = yarray_create(1))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = log->dump_status = YENOMEM;
		goto cleanup;
	}
	if (!(tmp_file = yfile_tmp(log->archive_path))) {
		ALOG("│ └ " YANSI_RED "Unable to create temporary file" YANSI_RESET);
		status = log->dump_status = YEIO;
		goto cleanup;
	}
	yarray_push(&env, password_env);
	yarray_push_multi(
		&args,
		11,
		"-u",
		dbuser,
		"--single-transaction",
		"--no-tablespaces",
		"--skip-lock-tables",
		"--routines",
		"-h",
		dbhost,
		"-P",
		dbport_str,
		(all_databases ? "-A" : dbname)
	);
	// execution
	ADEBUG("│ ├ " YANSI_FAINT "Execute " YANSI_RESET "mysqldump" YANSI_FAINT " to " YANSI_RESET "%s", log->archive_path);
	status = yexec(agent->bin.mysqldump, args, env, NULL, tmp_file);
	if (status != YENOERR) {
		ALOG("│ └ " YANSI_RED "Mysqldump error" YANSI_RESET);
		log->dump_status = status;
		unlink(tmp_file);
		goto cleanup;
	}
	// set log status
	log->dump_status = YENOERR;
	// move the file to its destination
	if (rename(tmp_file, log->archive_path)) {
		ALOG("│ └ " YANSI_RED "Unable to move file " YANSI_RESET "%s" YANSI_RED " to " YANSI_RED "%s" YANSI_RESET, tmp_file, log->archive_path);
		status = log->dump_status = YEIO;
		unlink(tmp_file);
		goto cleanup;
	}
	// compression of the file
	status = backup_compress_file(agent, log);
	// get archive file's size
	log->archive_size = yfile_get_size(log->archive_path);
cleanup:
	if (status != YENOERR)
		agent->exec_log.status_databases = false;
	if (log)
		log->success = (status == YENOERR) ? true : false;
	ys_free(filename);
	ys_free(password_env);
	ys_free(dbport_str);
	yarray_free(args);
	yarray_free(env);
	free0(tmp_file);
	return (status);
}
/* Backup a PostgreSQL database. */
static ystatus_t backup_pgsql(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	yvar_t *var_db_data = (yvar_t*)data;
	agent_t *agent = (agent_t*)user_data;
	ytable_t *db_data = NULL;
	log_item_t *log = NULL;
	bool all_databases = false;
	ystr_t dbname = NULL;
	ystr_t dbuser = NULL;
	ystr_t dbpwd = NULL;
	ystr_t dbhost = NULL;
	int64_t dbport = 0;
	ystr_t dbport_str = NULL;
	ystr_t filename = NULL;
	ystr_t password_env = NULL;
	yarray_t args = NULL;
	yarray_t env = NULL;
	char *tmp_file = NULL;

	// check mysqldump
	if (!agent->bin.mysqldump) {
		ALOG("└ " YANSI_RED "Failed (pg_dump/pg_dumpall not installed)" YANSI_RESET);
		status = YENOEXEC;
		goto cleanup;
	}
	// extract parameters and check them
	if (!yvar_is_array(var_db_data) || !(db_data = yvar_get_table(var_db_data)) ||
	    !(dbname = yvar_get_string(ytable_get_key_data(db_data, A_PARAM_KEY_DB))) ||
	    !(dbuser = yvar_get_string(ytable_get_key_data(db_data, A_PARAM_KEY_USER))) ||
	    !(dbpwd = yvar_get_string(ytable_get_key_data(db_data, A_PARAM_KEY_USER))) ||
	    !(dbhost = yvar_get_string(ytable_get_key_data(db_data, A_PARAM_KEY_HOST))) ||
	    !(dbport = yvar_get_int(ytable_get_key_data(db_data, A_PARAM_KEY_PORT)))) {
		ALOG("└ " YANSI_RED "Failed (bad parameter)" YANSI_RESET);
		status = YEBADCONF;
		goto cleanup;
	}
	if (!strcmp(dbname, A_DB_ALL_DATABASES_DEFINITION))
		all_databases = true;
	// log message
	ALOG("├ " YANSI_FAINT "Database " YANSI_RESET "%s", dbname);
	// creation of the log entry
	if (!(log = log_create_pgsql(agent, dbname))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		goto cleanup;
	}
	log->success = true;
	// create pg_dump command
	if (all_databases)
		filename = ys_new(A_DB_ALL_DATABASES_FILENAME);
	else
		filename = ys_filenamize(dbname);
	if (!filename ||
	    !(log->archive_name = ys_printf(NULL, "%s.sql", filename)) ||
	    !(log->archive_path = ys_printf(NULL, "%s/%s", agent->backup_mysql_path, log->archive_name)) ||
	    !(dbport_str = ys_printf(NULL, "%d", (int)dbport)) ||
	    !(password_env = ys_printf(NULL, "PGPASSWORD=\"%s\"", dbpwd)) ||
	    !(args = yarray_create(11)) ||
	    !(env = yarray_create(1))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = log->dump_status = YENOMEM;
		goto cleanup;
	}
	if (!(tmp_file = yfile_tmp(log->archive_path))) {
		ALOG("│ └ " YANSI_RED "Unable to create temporary file" YANSI_RESET);
		status = log->dump_status = YEIO;
		goto cleanup;
	}
	yarray_push(&env, password_env);
	yarray_push_multi(
		&args,
		11,
		"-U",
		dbuser,
		"-h",
		dbhost,
		"-p",
		dbport,
		"-f",
		tmp_file
	);
	if (!all_databases)
		yarray_push(&args, dbname);
	// execution
	char *bin_path = all_databases ? agent->bin.pg_dumpall : agent->bin.pg_dump;
	char *bin_name = all_databases ? "pg_dumpall" : "pg_dump";
	ADEBUG("│ ├ " YANSI_FAINT "Execute " YANSI_RESET "%s" YANSI_FAINT " to " YANSI_RESET "%s", bin_name, log->archive_path);
	status = yexec(bin_path, args, env, NULL, NULL);
	if (status != YENOERR) {
		ALOG("│ └ " YANSI_RED "%s error" YANSI_RESET, bin_name);
		log->dump_status = status;
		unlink(tmp_file);
		goto cleanup;
	}
	// set log status
	log->dump_status = YENOERR;
	// move the file to its destination
	if (rename(tmp_file, log->archive_path)) {
		ALOG("│ └ " YANSI_RED "Unable to move file " YANSI_RESET "%s" YANSI_RED " to " YANSI_RED "%s" YANSI_RESET, tmp_file, log->archive_path);
		status = log->dump_status = YEIO;
		unlink(tmp_file);
		goto cleanup;
	}
	// compression of the file
	status = backup_compress_file(agent, log);
	// get archive file's size
	log->archive_size = yfile_get_size(log->archive_path);
cleanup:
	if (status != YENOERR)
		agent->exec_log.status_databases = false;
	if (log)
		log->success = (status == YENOERR) ? true : false;
	ys_free(filename);
	ys_free(password_env);
	ys_free(dbport_str);
	yarray_free(args);
	yarray_free(env);
	free0(tmp_file);
	return (status);
}
/* Encrypt each backed up file. */
static void backup_encrypt_files(agent_t *agent) {
	ystatus_t st_files = YENOERR;
	ystatus_t st_mysql = YENOERR;
	ystatus_t st_pgsql = YENOERR;

	// check if there is something to encrypt
	if ((!agent->exec_log.backup_files || ytable_empty(agent->exec_log.backup_files)) &&
	    (!agent->exec_log.backup_mysql || ytable_empty(agent->exec_log.backup_mysql)) &&
	    (!agent->exec_log.backup_pgsql || ytable_empty(agent->exec_log.backup_pgsql)))
		return;
	// log message
	ALOG(
		"Encrypt files using " YANSI_FAINT "%s" YANSI_RESET,
		(agent->param.encryption == A_CRYPT_GPG) ? "gpg" :
		(agent->param.encryption == A_CRYPT_SCRYPT) ? "scrypt" :
		(agent->param.encryption == A_CRYPT_OPENSSL) ? "openssl" : "undef"
	);
	// encrypt backed up files
	if (agent->exec_log.backup_files && !ytable_empty(agent->exec_log.backup_files)) {
		ADEBUG("├ " YANSI_FAINT "Encrypt backed up files" YANSI_RESET);
		st_files = ytable_foreach(agent->exec_log.backup_files, backup_encrypt_item, agent);
	}
	// encrypt backed up MySQL databases
	if (agent->exec_log.backup_mysql && !ytable_empty(agent->exec_log.backup_mysql)) {
		ADEBUG("├ " YANSI_FAINT "Encrypt backed up MySQL databases" YANSI_RESET);
		st_mysql = ytable_foreach(agent->exec_log.backup_mysql, backup_encrypt_item, agent);
	}
	// encrypt backed up PostgreSQL databases
	if (agent->exec_log.backup_pgsql && !ytable_empty(agent->exec_log.backup_pgsql)) {
		ADEBUG("├ " YANSI_FAINT "Encrypt backed up PostgreSQL databases" YANSI_RESET);
		st_pgsql = ytable_foreach(agent->exec_log.backup_pgsql, backup_encrypt_item, agent);
	}
	// return status
	if (st_files == YENOERR && st_mysql == YENOERR && st_pgsql == YENOERR)
		ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	else if (st_files != YENOERR && st_mysql != YENOERR && st_pgsql != YENOERR)
		ALOG("└ " YANSI_RED "Error" YANSI_RESET);
	else
		ALOG("└ " YANSI_YELLOW "Partial error" YANSI_RESET);
}
/* Encrypt one backed up item. */
static ystatus_t backup_encrypt_item(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	log_item_t *item = (log_item_t*)data;
	agent_t *agent = (agent_t*)user_data;
	yarray_t args = NULL;
	char *pass_path = NULL;
	ystr_t output_name = NULL;
	ystr_t output_path = NULL;
	ystr_t param = NULL;

	if (!item->success)
		return (YENOERR);
	if (!(args = yarray_create(10)) ||
	    !(pass_path = yfile_tmp("/tmp/arkiv"))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		item->encrypt_status = status;
		item->success = false;
		goto cleanup;
	}
	yfile_put_string(pass_path, agent->conf.crypt_pwd);
	ADEBUG("│ ├ " YANSI_FAINT "Encrypting " YANSI_RESET "%s", item->archive_path);
	// prepare command
	if (agent->param.encryption == A_CRYPT_GPG) {
		// GPG
		if (!(output_name = ys_printf(NULL, "%s.gpg", item->archive_name)) ||
		    !(output_path = ys_printf(NULL, "%s.gpg", item->archive_path))) {
			status = YENOMEM;
			item->encrypt_status = status;
			item->success = false;
			goto cleanup;
		}
		// gpg --batch --passphrase-file /chemin/vers/votre/fichier_passphrase.txt --symmetric --output fichier_crypte.gpg fichier_a_chiffrer.txt
		yarray_push_multi(
			&args,
			8,
			"--batch",
			"--yes",
			"--passphrase-file",
			pass_path,
			"--symmetric",
			"--output",
			output_path,
			item->archive_path
		);
		// decrypt: gpg --batch --passphrase-file /chemin/vers/votre/fichier_passphrase.txt --decrypt -o fichier_decrypte.txt fichier_crypte.gpg
	} else if (agent->param.encryption == A_CRYPT_SCRYPT) {
		// SCRYPT
		if (!(output_name = ys_printf(NULL, "%s.scrypt", item->archive_name)) ||
		    !(output_path = ys_printf(NULL, "%s.scrypt", item->archive_path)) ||
		    !(param = ys_printf(NULL, "file:%s", pass_path))) {
			status = YENOMEM;
			item->encrypt_status = status;
			item->success = false;
			goto cleanup;
		}
		// scrypt enc --passphrase file:/chemin/vers/fichier_mot_de_passe.txt fichier_a_crypter fichier_crypté
		yarray_push_multi(
			&args,
			5,
			"enc",
			"--passphrase",
			param,
			item->archive_path,
			output_path
		);
	} else if (agent->param.encryption == A_CRYPT_OPENSSL) {
		// OPENSSL
		if (!(output_name = ys_printf(NULL, "%s.openssl", item->archive_name)) ||
		    !(output_path = ys_printf(NULL, "%s.openssl", item->archive_path)) ||
		    !(param = ys_printf(NULL, "file:%s", pass_path))) {
			status = YENOMEM;
			item->encrypt_status = status;
			item->success = false;
			goto cleanup;
		}
		// openssl enc -aes-256-cbc -e -salt -in file/to/encrypt -out file/encrypted -pass file:path/to/pass
		yarray_push_multi(
			&args,
			10,
			"enc",
			"-aes-256-cbc",
			"-e",
			"-salt",
			"-in",
			item->archive_path,
			"-out",
			output_path,
			"-pass",
			param
		);
	}
	// execution
	status = yexec(agent->bin.crypt, args, NULL, NULL, NULL);
	if (status != YENOERR) {
		ADEBUG("│ └ " YANSI_RED "Failed" YANSI_RESET);
		status = YENOMEM;
		item->encrypt_status = status;
		item->success = false;
		goto cleanup;
	}
	ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
	// remove unencrypted file
	unlink(item->archive_path);
	// set log status
	item->encrypt_status = YENOERR;
	// definitive paths
	ys_free(item->archive_name);
	item->archive_name = output_name;
	output_name = NULL;
	ys_free(item->archive_path);
	item->archive_path = output_path;
	output_path = NULL;
	// get archive file's size
	item->archive_size = yfile_get_size(item->archive_path);
cleanup:
	ys_free(param);
	if (pass_path) {
		unlink(pass_path);
		free0(pass_path);
	}
	ys_free(output_name);
	ys_free(output_path);
	yarray_free(args);
	return (status);
}
/* Compress a backed up file. */
static ystatus_t backup_compress_file(agent_t *agent, log_item_t *log) {
	ystatus_t status = YENOERR;
	yarray_t args = NULL;
	ystr_t z_name = NULL, z_path = NULL;

	// chec if a compression is needed
	if (agent->param.compression == A_COMP_NONE ||
	    !log->success)
		return (YENOERR);
	// create compression command
	if (!(args = yarray_create(4))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		log->compress_status = status;
		log->success = false;
		return (status);
	}
	if (agent->param.compression == A_COMP_ZSTD)
		yarray_push(&args, "--rm");
	yarray_push_multi(&args, 3, "--quiet", "--force", log->archive_path);
	// execution
	ADEBUG("│ ├ " YANSI_FAINT "Compress file " YANSI_RESET "%s", log->archive_path);
	status = yexec(agent->bin.z, args, NULL, NULL, NULL);
	if (status != YENOERR) {
		ALOG("│ └ " YANSI_RED "Compression error" YANSI_RESET);
		log->compress_status = status;
		log->success = false;
		goto cleanup;
	}
	ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
	// set log status
	log->compress_status = YENOERR;
	// definitive paths
	char *ext = NULL;
	if (agent->param.compression == A_COMP_GZIP)
		ext = "gz";
	else if (agent->param.compression == A_COMP_BZIP2)
		ext = "bz2";
	else if (agent->param.compression == A_COMP_XZ)
		ext = "xz";
	else if (agent->param.compression == A_COMP_ZSTD)
		ext = "zst";
	z_name = ys_printf(NULL, "%s.%s", log->archive_name, ext);
	z_path = ys_printf(NULL, "%s.%s", log->archive_path, ext);
	if (!z_name || !z_path) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		log->compress_status = status;
		log->success = false;
		goto cleanup;
	}
	ys_free(log->archive_name);
	ys_free(log->archive_path);
	log->archive_name = z_name;
	log->archive_path = z_path;
	z_name = z_path = NULL;
cleanup:
	yarray_free(args);
	ys_free(z_name);
	ys_free(z_path);
	return (status);
}
/* Compute the checksum of each backed up file. */
static void backup_compute_checksums(agent_t *agent) {
	ystatus_t st_files = YENOERR, st_mysql = YENOERR, st_pgsql = YENOERR;
	ystatus_t st_databases = YENOERR;

	// check if there is something to compute
	if (ytable_empty(agent->exec_log.backup_files) &&
	    ytable_empty(agent->exec_log.backup_mysql) &&
	    ytable_empty(agent->exec_log.backup_pgsql))
		return;
	// log message
	ALOG("Compute checksums");
	// compute checksums of backed up files
	if (!ytable_empty(agent->exec_log.backup_files)) {
		// change working directory
		if (chdir(agent->backup_files_path)) {
			ALOG("└ " YANSI_RED "Unable to change working directory to '" YANSI_RESET "%s" YANSI_RED "'" YANSI_RESET, agent->backup_files_path);
			return;
		}
		// compute
		ADEBUG("├ " YANSI_FAINT "Compute checkums of backed up files" YANSI_RESET);
		st_files = ytable_foreach(agent->exec_log.backup_files, backup_compute_checksum_item, agent);
		if (st_files == YENOERR)
			ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
		else
			ADEBUG("│ └ " YANSI_RED "Error" YANSI_RESET);
	}
	// compute checksums of backed up MySQL databases
	if (!ytable_empty(agent->exec_log.backup_mysql)) {
		// change working directory
		if (chdir(agent->backup_mysql_path)) {
			ALOG("└ " YANSI_RED "Unable to change working directory to '" YANSI_RESET "%s" YANSI_RED "'" YANSI_RESET, agent->backup_mysql_path);
			return;
		}
		// compute
		ADEBUG("├ " YANSI_FAINT "Compute checksums of backed up MySQL databases" YANSI_RESET);
		st_mysql = ytable_foreach(agent->exec_log.backup_mysql, backup_compute_checksum_item, agent);
		if (st_mysql == YENOERR)
			ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
		else
			ADEBUG("│ └ " YANSI_RED "Error" YANSI_RESET);
	}
	// compute checksums of backed up PostgreSQL databases
	if (!ytable_empty(agent->exec_log.backup_pgsql)) {
		// change working directory
		if (chdir(agent->backup_pgsql_path)) {
			ALOG("└ " YANSI_RED "Unable to change working directory to '" YANSI_RESET "%s" YANSI_RED "'" YANSI_RESET, agent->backup_pgsql_path);
			return;
		}
		// compute
		ADEBUG("├ " YANSI_FAINT "Compute checksums of backed up PostgreSQL databases" YANSI_RESET);
		st_pgsql = ytable_foreach(agent->exec_log.backup_pgsql, backup_compute_checksum_item, agent);
		if (st_pgsql == YENOERR)
			ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
		else
			ADEBUG("│ └ " YANSI_RED "Error" YANSI_RESET);
	}
	if (st_mysql != YENOERR)
		st_databases = st_mysql;
	else if (st_pgsql != YENOERR)
		st_databases = st_pgsql;
	if (st_files == YENOERR && st_databases == YENOERR)
		ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	else if (st_files != YENOERR && st_databases != YENOERR)
		ALOG("└ " YANSI_RED "Error" YANSI_RESET);
	else
		ALOG("└ " YANSI_YELLOW "Partial error" YANSI_RESET);
}
/* Compute the checksum of a backed up item. */
static ystatus_t backup_compute_checksum_item(uint64_t hash, char *key, void *data, void *user_data) {
	ystatus_t status = YENOERR;
	log_item_t *item = data;
	agent_t *agent = user_data;
	yarray_t args = NULL;
	ybin_t bin = {0};

	if (!item->success)
		return (YENOERR);
	ADEBUG("├ " YANSI_FAINT "Compute checksum of " YANSI_RESET "%s", item->archive_path);
	// create argument list
	if (!(args = yarray_create(1))) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		item->success = false;
		goto end;
	}
	yarray_push(&args, item->archive_name);
	// execution
	status = yexec(agent->bin.checksum, args, NULL, &bin, NULL);
	if (status != YENOERR || !bin.bytesize) {
		ALOG("└ " YANSI_RED "Checksum error" YANSI_RESET);
		goto end;
	}
	// write result
	if (!(item->checksum_name = ys_printf(NULL, "%s.sha512", item->archive_name)) ||
	    !(item->checksum_path = ys_printf(NULL, "%s.sha512", item->archive_path))) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		item->success = false;
		goto end;
	}
	if (!yfile_put_contents(item->checksum_path, &bin)) {
		ALOG("└ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		item->success = false;
		goto end;
	}
end:
	item->checksum_status = status;
	yarray_free(args);
	ybin_delete_data(&bin);
	return (status);
}

