#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
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

	ALOG_RAW(YANSI_NEGATIVE "---------------------------" YANSI_RESET);
	ALOG("Start backup");
	// get tar path
	if (!(agent->bin.tar = get_program_path("tar"))) {
		ALOG("└ " YANSI_RED "Unable to find " YANSI_RESET "tar" YANSI_RED " program" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}
	// get sh512sum path
	if (!(agent->bin.checksum = get_program_path("sha512sum"))) {
		ALOG("└ " YANSI_RED "Unable to find " YANSI_RESET "sha512sum" YANSI_RED " program" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}
	// check rclone
	if (!yfile_is_executable(A_EXE_RCLONE)) {
		ALOG("└ " YANSI_RED "Unable to find " YANSI_RESET A_EXE_RCLONE YANSI_RED " program" YANSI_RESET);
		ALOG(YANSI_RED "Abort" YANSI_RESET);
		return;
	}
	// fetch parameters file
	st = backup_fetch_params(agent);
	if (st == YEAGAIN) {
		ALOG(YANSI_GREEN "✓ End of processing" YANSI_RESET);
		return;
	} else if (st != YENOERR) {
		ALOG(YANSI_BG_RED "Abort" YANSI_RESET);
		return;
	}
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
		//backup_databases(agent);
		// encrypt files
		backup_encrypt_files(agent);
		// compute checksums
		backup_compute_checksums(agent);
		// upload files
		upload_files(agent);
	}
	// send report
	ALOG("Send report to arkiv.sh");
	if (api_backup_report(agent) == YENOERR)
		ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	else
		ALOG("└ " YANSI_RED "Failed" YANSI_RESET);
	// execute post-scripts
	if (backup_exec_scripts(agent, A_SCRIPT_TYPE_POST) != YENOERR) {
		return;
	}
}

/* ********** PRIVATE FUNCTIONS ********** */
/* Fetch and process host backup parameter file. */
static ystatus_t backup_fetch_params(agent_t *agent) {
	ALOG("├ " YANSI_FAINT "Fetch host parameters" YANSI_RESET);
	// fetch params file
	yvar_t *params = api_get_params_file(agent);
	if (!params) {
		ALOG("└ " YANSI_RED "Failed (unable to download or deserialize the file)" YANSI_RESET);
		return (YEBADCONF);
	}
	/* extract parameters */
	yvar_t *var_ptr;
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

	// get schedule name
	var_ptr = yvar_get_from_path(params, A_PARAM_PATH_SCHEDULE_NAME);
	if (!var_ptr || !yvar_is_string(var_ptr) || !(agent->param.schedule_name = yvar_get_string(var_ptr))) {
		ALOG("└ " YANSI_RED "Unable to find schedule name" YANSI_RESET);
		return (YEBADCONF);
	}
	ADEBUG("├ " YANSI_FAINT "Schedule name: " YANSI_RESET "%s", agent->param.schedule_name);
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
		ALOG("└ " YANSI_FAINT "No backup scheduled for this day/time" YANSI_RESET);
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
	// from the schedule, get the storage ID
	ADEBUG("├ " YANSI_FAINT "From the schedule, extract the storage ID" YANSI_RESET);
	var_ptr2 = yvar_get_from_path(schedule, A_PARAM_PATH_STORAGES);
	if (!var_ptr2 || !yvar_is_int(var_ptr2)) {
		ALOG("└ " YANSI_RED "Failed (wrongly formatted file: no schedule storage)" YANSI_RESET);
		return (YEBADCONF);
	}
	int64_t storage_id = yvar_get_int(var_ptr2);
	ADEBUG("│ └ " YANSI_FAINT "Storage ID: " YANSI_RESET "%" PRId64, storage_id);

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
	// get savepack name
	var_ptr2 = yvar_get_from_path(savepack, A_PARAM_PATH_NAME);
	if (!var_ptr2 || !yvar_is_string(var_ptr2) || !(agent->param.savepack_name = yvar_get_string(var_ptr2))) {
		ALOG("└ " YANSI_RED "Unable to find savepack name (ID %" PRId64 ")" YANSI_RESET, savepack_id);
		return (YEBADCONF);
	}
	ADEBUG("│ ├ " YANSI_FAINT "Savepack name: " YANSI_RESET "%s", agent->param.savepack_name);
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
	// get files
	var_ptr2 = yvar_get_from_path(savepack, A_PARAM_PATH_FILE);
	if (var_ptr2 && yvar_is_table(var_ptr2)) {
		agent->param.files = yvar_get_table(var_ptr2);
		ADEBUG("│ ├ %d" YANSI_FAINT " file(s)" YANSI_RESET, ytable_length(agent->param.files));
	} else
		ADEBUG("│ ├ " YANSI_FAINT "No file" YANSI_RESET);
	// get databases
	var_ptr2 = yvar_get_from_path(savepack, A_PARAM_PATH_DB);
	if (var_ptr2 && yvar_is_table(var_ptr2)) {
		ADEBUG("│ └ %d" YANSI_FAINT " database(s)" YANSI_RESET, ytable_length(agent->param.databases));
	} else
		ADEBUG("│ └ " YANSI_FAINT "No database" YANSI_RESET);

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
	ADEBUG("│ ├ " YANSI_FAINT "Storage found" YANSI_RESET);
	// get storage name
	var_ptr2 = yvar_get_from_path(storage, A_PARAM_PATH_NAME);
	if (!var_ptr2 || !yvar_is_string(var_ptr2) || !(agent->param.storage_name = yvar_get_string(var_ptr2))) {
		ALOG("└ " YANSI_RED "Unable to find storage name (ID %" PRId64 ")" YANSI_RESET, storage_id);
		return (YEBADCONF);
	}
	ADEBUG("│ └ " YANSI_FAINT "Storage name: " YANSI_RESET "%s", agent->param.storage_name);
	
	return (YENOERR);
}
/* Create output directory. */
static ystatus_t backup_create_output_directory(agent_t *agent) {
	time_t current_time = time(NULL);
	struct tm *tm = localtime(&current_time);

	// generate path
	agent->datetime_chunk_path = ys_printf(NULL, "%04d-%02d-%02d/%02d:00", tm->tm_year + 1900,
	                                       tm->tm_mon + 1, tm->tm_mday, tm->tm_hour);
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
	yvar_t *var_script = (yvar_t*)data;
	agent_t *agent = user_data;
	ystr_t command;
	log_script_t *log;

	// checks
	if (!yvar_is_string(var_script) || !(command = yvar_get_string(var_script))) {
		ALOG("└ " YANSI_RED "Failed (bad parameter)" YANSI_RESET "\n");
		return (YEBADCONF);
	}
	ALOG("├ " YANSI_FAINT "Execution of " YANSI_RESET "%s", command);
	// creation of the log entry
	if (!(log = log_create_pre_script(agent, command))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return (YENOMEM);
	}
	// script execution
	int ret_val = system(command);
	if (ret_val == -1 || !WIFEXITED(ret_val) || WEXITSTATUS(ret_val)) {
		ADEBUG("│ └ " YANSI_RED "Failed (returned value " YANSI_RESET "%d" YANSI_RED ")" YANSI_RESET, WEXITSTATUS(ret_val));
		log->success = false;
		return (YEFAULT);
	}
	ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
	return (YENOERR);
}
/* Execute a post-script. */
static ystatus_t backup_exec_post_script(uint64_t hash, char *key, void *data, void *user_data) {
	yvar_t *var_script = (yvar_t*)data;
	agent_t *agent = user_data;
	ystr_t command;
	log_script_t *log;

	// checks
	if (!yvar_is_string(var_script) || !(command = yvar_get_string(var_script))) {
		ALOG("└ " YANSI_RED "Failed (bad parameter)" YANSI_RESET "\n");
		return (YEBADCONF);
	}
	ALOG("├ " YANSI_FAINT "Execution of " YANSI_RESET "%s", command);
	// creation of the log entry
	if (!(log = log_create_post_script(agent, command))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return (YENOMEM);
	}
	// script execution
	int ret_val = system(command);
	if (ret_val) {
		ADEBUG("│ └ " YANSI_RED "Failed (returned value " YANSI_RESET "%d" YANSI_RED ")" YANSI_RESET, ret_val);
		log->success = false;
		return (YEFAULT);
	}
	ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
	return (YENOERR);
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

	// checks
	if (!yvar_is_string(var_file_path) || !(file_path = yvar_get_string(var_file_path))) {
		ALOG("└ " YANSI_RED "Failed (bad parameter)" YANSI_RESET);
		return (YEBADCONF);
	}
	// log message
	ALOG("├ " YANSI_FAINT "Backup path " YANSI_RESET "%s", file_path);
	// creation of the log entry
	if (!(log = log_create_file(agent, file_path))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		return (YENOMEM);
	}
	// remove '/' at the beginning of the path to save, and call tar with "-C /" options
	// to avoid the warning "Removing leading `/' from member names"
	char *path = file_path;
	while (*path == '/')
		path++;
	// create tar command
	if (!(filename = ys_filenamize(path)) ||
	    !(log->archive_name = ys_printf(NULL, "%s.tar", filename)) ||
	    !(log->archive_path = ys_printf(NULL, "%s/%s", agent->backup_files_path, log->archive_name)) ||
	    !(args = yarray_create(9))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		log->dump_status = status;
		log->success = false;
		goto cleanup;
	}
	yarray_push_multi(
		&args,
		9,
		"cf",
		log->archive_path,
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
	status = yexec(agent->bin.tar, args, NULL, NULL, "/tmp/tar_result_arkiv");
	if (status != YENOERR) {
		ALOG("│ └ " YANSI_RED "Tar error" YANSI_RESET);
		log->dump_status = status;
		log->success = false;
		goto cleanup;
	}
	// set log status
	log->dump_status = YENOERR;
	// check if a compression is needed
	if (agent->param.compression == A_COMP_NONE) {
		ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
		goto cleanup;
	}
	// create compression command
	yarray_free(args);
	if (!(args = yarray_create(4))) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		log->compress_status = status;
		log->success = false;
		goto cleanup;
	}
	if (agent->param.compression == A_COMP_ZSTD)
		yarray_push(&args, "--rm");
	yarray_push_multi(&args, 3, "--quiet", "--force", log->archive_path);
	// execution
	ADEBUG("│ ├ " YANSI_FAINT "Compress file " YANSI_RESET "%s/%s.tar", agent->backup_path, filename);
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
	ystr_t z_name = ys_printf(NULL, "%s.%s", log->archive_name, ext);
	if (!z_name) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		log->compress_status = status;
		log->success = false;
		goto cleanup;
	}
	ys_free(log->archive_name);
	log->archive_name = z_name;
	ystr_t z_path = ys_printf(NULL, "%s.%s", log->archive_path, ext);
	if (!z_path) {
		ALOG("│ └ " YANSI_RED "Memory allocation error" YANSI_RESET);
		status = YENOMEM;
		log->compress_status = status;
		log->success = false;
		goto cleanup;
	}
	ys_free(log->archive_path);
	log->archive_path = z_path;
cleanup:
	ys_free(filename);
	yarray_free(args);
	return (status);
}
/* Encrypt each backed up file. */
static void backup_encrypt_files(agent_t *agent) {
	ystatus_t st_files = YENOERR;
	ystatus_t st_databases = YENOERR;

	// check if there is something to encrypt
	if ((!agent->exec_log.backup_files || ytable_empty(agent->exec_log.backup_files)) &&
	    (!agent->exec_log.backup_databases || ytable_empty(agent->exec_log.backup_databases)))
		return;
	// log message
	ALOG(
		"Encrypt files using %s",
		(agent->param.encryption == A_CRYPT_GPG) ? "gpg" :
		(agent->param.encryption == A_CRYPT_SCRYPT) ? "scrypt" :
		(agent->param.encryption == A_CRYPT_OPENSSL) ? "openssl" : "undef"
	);
	// encrypt backed up files
	if (agent->exec_log.backup_files && !ytable_empty(agent->exec_log.backup_files)) {
		ADEBUG("├ " YANSI_FAINT "Encrypt backed up files" YANSI_RESET);
		st_files = ytable_foreach(agent->exec_log.backup_files, backup_encrypt_item, agent);
	}
	// encrypt backed up databases
	if (agent->exec_log.backup_databases && !ytable_empty(agent->exec_log.backup_databases)) {
		ADEBUG("├ " YANSI_FAINT "Encrypt backed up databases" YANSI_RESET);
		st_files = ytable_foreach(agent->exec_log.backup_databases, backup_encrypt_item, agent);
	}
	if (st_files == YENOERR && st_databases == YENOERR)
		ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
	else if (st_files != YENOERR && st_databases != YENOERR)
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
/* Compute the checksum of each backed up file. */
static void backup_compute_checksums(agent_t *agent) {
	ystatus_t st_files = YENOERR;
	ystatus_t st_databases = YENOERR;

	// check if there is something to compute
	if ((!agent->exec_log.backup_files || ytable_empty(agent->exec_log.backup_files)) &&
	    (!agent->exec_log.backup_databases || ytable_empty(agent->exec_log.backup_databases)))
		return;
	// log message
	ALOG("Compute checksums");
	// compute checksums of backed up files
	if (agent->exec_log.backup_files && !ytable_empty(agent->exec_log.backup_files)) {
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
	// compute checksums of backed up databases
	if (agent->exec_log.backup_databases && !ytable_empty(agent->exec_log.backup_databases)) {
		// change working directory
		if (chdir(agent->backup_databases_path)) {
			ALOG("└ " YANSI_RED "Unable to change working directory to '" YANSI_RESET "%s" YANSI_RED "'" YANSI_RESET, agent->backup_databases_path);
			return;
		}
		// compute
		ADEBUG("├ " YANSI_FAINT "Compute checksums of backed up databases" YANSI_RESET);
		st_files = ytable_foreach(agent->exec_log.backup_databases, backup_compute_checksum_item, agent);
		if (st_databases == YENOERR)
			ADEBUG("│ └ " YANSI_GREEN "Done" YANSI_RESET);
		else
			ADEBUG("│ └ " YANSI_RED "Error" YANSI_RESET);
	}
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

