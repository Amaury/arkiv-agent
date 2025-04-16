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
	ystr_t ansi_free = NULL;

	if (!agent || (!agent->log_fd && !agent->conf.use_stdout && !agent->conf.use_syslog) ||
	    (debug && !agent->debug_mode)) {
		return;
	}
	// creation of the log string
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
	// check if an ANSI-free string is needed
	if (!agent->conf.use_ansi || agent->conf.use_syslog)
		ansi_free = ys_clean_ansi(buffer);
	// write to file
	if (agent->log_fd) {
		vfprintf(agent->log_fd, (agent->conf.use_ansi ? buffer : ansi_free), plist);
		fflush(agent->log_fd);
	}
	// write to stdout
	if (agent->conf.use_stdout) {
		vfprintf(stdout, (agent->conf.use_ansi ? buffer : ansi_free), plist2);
		fflush(stdout);
	}
	// write to syslog
	if (agent->conf.use_syslog) {
		// if len is not zero, there is date and time at the beginning of the string,
		// so we take 26 bytes after the beginning of the ansi_free string; we can't use
		// the len variable because it counts ANSI characters wich are not in ansi_free
		pt = ansi_free + (len ? 26 : 0);
		vsyslog(LOG_NOTICE, pt, plist3);
	}
	va_end(plist);
	va_end(plist2);
	va_end(plist3);
	ys_free(ansi_free);
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
/* Creates a log entry for a MySQL database backup. */
log_item_t *log_create_mysql(agent_t *agent, ystr_t dbname) {
	log_item_t *log = malloc0(sizeof(log_item_t));
	if (!log)
		return (NULL);
	log->type = A_ITEM_TYPE_DB_MYSQL;
	log->item = dbname;
	log->success = true;
	log->dump_status = YEUNDEF;
	log->compress_status = YEUNDEF;
	log->encrypt_status = YEUNDEF;
	log->checksum_status = YEUNDEF;
	log->upload_status = YEUNDEF;
	ytable_add(agent->exec_log.backup_databases, log);
	return (log);
}
/* Creates a log entry for a PostgreSQL database backup. */
log_item_t *log_create_pgsql(agent_t *agent, ystr_t dbname) {
	log_item_t *log = malloc0(sizeof(log_item_t));
	if (!log)
		return (NULL);
	log->type = A_ITEM_TYPE_DB_PGSQL;
	log->item = dbname;
	log->success = true;
	log->dump_status = YEUNDEF;
	log->compress_status = YEUNDEF;
	log->encrypt_status = YEUNDEF;
	log->checksum_status = YEUNDEF;
	log->upload_status = YEUNDEF;
	ytable_add(agent->exec_log.backup_databases, log);
	return (log);
}

