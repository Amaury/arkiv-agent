#include <time.h>
#include "yansi.h"
#include "ymemory.h"
#include "yarray.h"
#include "log.h"

/* Write a message to the log file. */
void alog(agent_t *agent, bool debug, bool show_time, const char *str, ...) {
	va_list plist;
	char buffer[4096];

	if (!agent || (!agent->log_fd && !agent->conf.use_syslog) ||
	    (debug && agent->debug_mode)) {
		return;
	}
	va_start(plist, str);
	buffer[0] = '\0';
	if (show_time) {
		time_t current_time = time(NULL);
		struct tm *tm = localtime(&current_time);
		int tz_hours = (int)(timezone / 3600);
		int tz_minutes = (int)(timezone % 3600);
		char tz_sign = '+';
		if (timezone < 0) {
			tz_hours = -tz_hours;
			tz_minutes = -tz_minutes;
			tz_sign = '-';
		}
		snprintf(
			buffer,
			sizeof(buffer),
			"%s[%04d-%02d-%02d %02d:%02d:%02d%c%02d:%02d]%s ",
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
	if (agent->conf.use_syslog) {
		vsyslog(LOG_NOTICE, buffer, plist);
	}
}
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

