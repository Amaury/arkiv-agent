#include <time.h>
#include <stdarg.h>
#include "yansi.h"
#include "ymemory.h"
#include "yarray.h"
#include "log.h"

/* Write a message to the log file. */
void alog(agent_t *agent, bool debug, bool show_time, const char *str, ...) {
	va_list plist, plist2, plist3;
	char buffer[4096];

	if (!agent || (!agent->log_fd && !agent->conf.use_stdout && !agent->conf.use_syslog) ||
	    (debug && !agent->debug_mode)) {
		return;
	}
	va_start(plist, str);
	va_copy(plist2, plist);
	va_copy(plist3, plist);
	buffer[0] = '\0';
	if (show_time) {
		time_t current_time = time(NULL);
		struct tm *tm = localtime(&current_time);
		int tz_hours = abs((int)(tm->tm_gmtoff / 3600));
		int tz_minutes = abs((int)((tm->tm_gmtoff % 3600) / 60));
		char tz_sign = (tm->tm_gmtoff >= 0) ? '+' : '-';
		snprintf(
			buffer,
			sizeof(buffer),
			"%s%04d-%02d-%02d %02d:%02d:%02d%c%02d:%02d%s ",
			YANSI_FAINT,
			tm->tm_year + 1900,
			tm->tm_mon + 1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec,
			tz_sign,
			tz_hours,
			tz_minutes,
			YANSI_RESET
		);
	}
	size_t len = strlen(buffer);
	char *pt = buffer + len;
	snprintf(pt, sizeof(buffer) - len, "%s\n", str);
	if (agent->log_fd) {
		vfprintf(agent->log_fd, buffer, plist);
		fflush(agent->log_fd);
	}
	if (agent->conf.use_stdout) {
		vfprintf(stdout, buffer, plist2);
		fflush(stdout);
	}
	if (agent->conf.use_syslog) {
		vsyslog(LOG_NOTICE, buffer, plist3);
	}
	va_end(plist);
	va_end(plist2);
	va_end(plist3);
}
/* Creates a log entry for a pre-script execution. */
log_script_t *log_create_pre_script(agent_t *agent, ystr_t command) {
	log_script_t *log = malloc0(sizeof(log_script_t));
	if (!log)
		return (NULL);
	log->command = command;
	log->success = true;
	ytable_add(agent->exec_log.pre_scripts, log);
	return (log);
}
/* Creates a log entry for a post-script execution. */
log_script_t *log_create_post_script(agent_t *agent, ystr_t command) {
	log_script_t *log = malloc0(sizeof(log_script_t));
	if (!log)
		return (NULL);
	log->command = command;
	log->success = true;
	ytable_add(agent->exec_log.post_scripts, log);
	return (log);
}
/* Creates a log entry for a file. */
log_item_t *log_create_file(agent_t *agent, ystr_t path) {
	log_item_t *log = malloc0(sizeof(log_item_t));
	if (!log)
		return (NULL);
	log->type = A_ITEM_TYPE_FILE;
	log->item = path;
	log->success = true;
	log->dump_status = YEUNDEF;
	log->compress_status = YEUNDEF;
	log->encrypt_status = YEUNDEF;
	log->checksum_status = YEUNDEF;
	log->upload_status = YEUNDEF;
	ytable_add(agent->exec_log.backup_files, log);
	return (log);
}

#if 0
/* Add a log item entry for a backed up file. */
void log_file_backup(agent_t *agent, char *filename, char *tarname, ystatus_t status,
                     ystatus_t compress_status, ystatus_t encrypt_status) {
	log_item_t *item = malloc0(sizeof(log_item_t));
	if (!item)
		return;
	item->orig_path = strdup(filename);
	item->result_path = strdup(tarname);
	item->status = status;
	yarray_add(&agent->exec_log.backup_files, item);
}
/* Add a log item entry for a backed up database. */
void log_database_backup(agent_t *agent, char *db, char *tarname, ystatus_t status,
                         ystatus_t compress_status, ystatus_t encrypt_status) {
	log_item_t *item = malloc0(sizeof(log_item_t));
	if (!item)
		return;
	item->orig_path = strdup(db);
	item->result_path = strdup(tarname);
	item->status = status;
	yarray_add(&agent->exec_log.backup_databases, item);
}
/* Add a log item entry for a S3 upload. */
void log_s3_upload(agent_t *agent, char *local, char *distant, ystatus_t status) {
	log_item_t *item = malloc0(sizeof(log_item_t));
	if (!item)
		return;
	item->orig_path = strdup(local);
	item->result_path = strdup(distant);
	item->status = status;
	yarray_add(&agent->exec_log.upload_s3, item);
}
#endif

