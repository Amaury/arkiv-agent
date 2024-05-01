/**
 * Arkiv agent.
 * Command line:
 * ./agent
 * debug_mode=true ./agent
 * logfile=/var/log/arkiv/arkiv.log ./agent
 * loglevel=WARN ./agent
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2019-2024, Amaury Bouchard
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "yansi.h"
#include "agent.h"
#include "configuration.h"
#include "declare.h"
#include "backup.h"

/* *** declaration of private functions *** */
void _agent_usage(const char *progname);
//agent_t *_agent_new(void);
//void _agent_free(agent_t *agent);

typedef enum {
	A_TYPE_USAGE = 0,
	A_TYPE_CONFIG,
	A_TYPE_DECLARE,
	A_TYPE_BACKUP,
	A_TYPE_RESTORE
} exec_type_t;

/**
 * Main function of the program.
 */
int main(int argc, char *argv[]) {
	// agent structure allocation and initialization
	agent_t *agent = agent_new(argv[0]);

	// check command-line arguments
	exec_type_t exec_type = (
		(argc == 2 && !strcmp(argv[1], A_OPT_CONFIG)) ? A_TYPE_CONFIG :
		(argc == 2 && !strcmp(argv[1], A_OPT_DECLARE)) ? A_TYPE_DECLARE :
		(argc == 2 && !strcmp(argv[1], A_OPT_BACKUP)) ? A_TYPE_BACKUP :
		(argc == 3 && !strcmp(argv[1], A_OPT_RESTORE)) ? A_TYPE_RESTORE :
		A_TYPE_USAGE
	);
	// execution
	if (exec_type == A_TYPE_USAGE) {
		// usage
		_agent_usage(agent->agent_path);
	} else if (exec_type == A_TYPE_CONFIG) {
		// configuration
		exec_configuration(agent);
	} else {
		// load configuration file
		agent_load_configuration(agent);
		if (agent->debug_mode) {
			printf("agent_path           : '%s'\n", agent->agent_path);
			printf("conf_path            : '%s'\n", agent->conf_path);
			printf("debug_mode           : '%s'\n", agent->debug_mode ? "true" : "false");
			printf("conf.logfile         : '%s'\n", agent->conf.logfile);
			printf("conf.archives_path   : '%s'\n", agent->conf.archives_path);
			printf("conf.org_key         : '%s'\n", agent->conf.org_key);
			printf("conf.hostname        : '%s'\n", agent->conf.hostname);
			printf("conf.crypt_pwd       : '%s'\n", agent->conf.crypt_pwd);
			printf("conf.use_syslog      : '%s'\n", agent->conf.use_syslog ? "true" : "false");
		}
		// execution
		if (exec_type == A_TYPE_DECLARE) {
			// server declaration
			exec_declare(agent);
		} else if (exec_type == A_TYPE_BACKUP) {
			// backup
			exec_backup(agent);
		} else if (exec_type == A_TYPE_RESTORE) {
			// restore
			printf("agent_restore(argv[2]);\n");
		}
	}
	agent_free(agent);
	return (0);
}

#if 0
	ystatus_t status = YENOERR;
	int exit_value = 0;
	agent_t *agent = NULL;

	// display usage and quit if needed
	if (argc == 1 || argc > 2 ||
	    (strcasecmp(argv[1], "config") && strcasecmp(argv[1], "exec"))) {
		_agent_usage(argv[0]);
		exit(1);
	}
	ytry {
		// agent creation
		agent = _agent_new();
		// check if debug option was activated
		init_check_debug(agent, argc, argv);
		// check the current user
		if ((status = check_root_user()) != YENOERR)
			goto error;
		// get execution mode
		if (argc > 1) {
			if (!strcmp(argv[1], "install")) {
				// install mode
				return (install(argv[0]));
			} else if (strcmp(argv[1], "backup")) {
				// bad parameter
				fprintf(stderr, "Bad parameter.");
				exit(2);
			}
		}
		// initialize log
		init_log(agent);
		// *** startup
		ALOG_RAW(YANSI_NEGATIVE "---------------------------" YANSI_RESET);
		{
			ALOG(YANSI_BOLD "Start" YANSI_RESET);
			// read the local configuration
			if (init_read_conf(agent, argc, argv) != YENOERR)
				goto error;
			// get distant parameters for this machine
			if (init_fetch_params(agent) != YENOERR)
				goto error;
			// check if encryption is mandatory but impossible to do
			if (init_check_mandatory_encryption(agent) != YENOERR)
				goto error;
			// create output directory
			if (init_create_output_directory(agent) != YENOERR)
				goto error;
			ALOG("└ " YANSI_GREEN "Done" YANSI_RESET);
		}
		// ** check if there is something to backup
		if ((!agent->path || !yarray_len(agent->path)) &&
		    !agent->mysqldump.all_db &&
		    (!agent->mysqldump.db || !yarray_len(agent->mysqldump.db))) {
			ALOG(YANSI_BOLD "Nothing to backup" YANSI_RESET);
			goto cleanup;
		}
		// *** files backup
		if ((status = backup_files(agent)) != YENOERR)
			agent->log.status = AERROR_OVERRIDE(agent->log.status, status);
		// *** MySQL dump backup
		if ((status = backup_database_mysqldump(agent)) != YENOERR)
			goto error;
		// *** Xtrabackup
		if ((status = backup_database_xtrabackup(agent)) != YENOERR)
			goto error;
		// *** PostgreSQL dump backup
		if ((status = backup_database_pgdump(agent)) != YENOERR)
			goto error;
		// compute sha256sum
		if ((status = sha256sum_create(agent)) != YENOERR)
			goto error;
		// upload files onto Amazon S3
		upload_aws_s3(agent);
		// send log to Arkiv.sh server
	} ycatch("YEXCEPT_NOMEM") {
		YLOG_ADD(YLOG_ERR, "Memory allocation error.");
		goto error;
	} yfinalize;
	goto cleanup;
error:
	exit_value = 3;
cleanup:
	YLOG_ADD(YLOG_NOTE, "End of processing.");
	_agent_free(agent);
	YLOG_END();
	return (exit_value);
}
#endif

/* ********** PRIVATE FUNCTIONS ********** */
/**
 * Display documentation.
 * @param	progname	Name of the executed program.
 */
void _agent_usage(const char *progname) {
	printf("\n");
	printf(YANSI_BG_BLUE "%80c" YANSI_RESET "\n", ' ');
	printf(YANSI_BG_BLUE YANSI_WHITE "%31c%s%30c" YANSI_RESET "\n", ' ', "Arkiv.sh agent help", ' ');
	printf(YANSI_BG_BLUE "%80c" YANSI_RESET "\n", ' ');
	printf("\n");
	printf(
		YANSI_BOLD "  arkiv-agent " YANSI_RESET
		"is a program used to backup a computer, using the parameters\n"
		"  declared on the " YANSI_FAINT "Arkiv.sh" YANSI_RESET
		" service. It shoud be called automatically (by the\n"
		"  cron daemon) and not by a user, appart from during its installation process.\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Account creation " YANSI_RESET "\n\n"
		"  Before you install and configure the Arkiv agent, you need to create your\n"
		"  account on the " YANSI_LINK_STATIC("https://www.arkiv.sh/", "Arkiv.sh") " website. "
		"Free accounts allow to manage one server.\n"
		"  You will need to retrieve the " YANSI_FAINT "organization key" YANSI_RESET
		" provided by the service.\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " User rights " YANSI_RESET "\n\n"
		"  The Arkiv agent should be run by the " YANSI_FAINT "root" YANSI_RESET
		" user, or any other user with\n  sufficient access rights.\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Usage " YANSI_RESET "\n\n"
		YANSI_FAINT "  [envvars] " YANSI_RESET
		YANSI_GREEN "%s" YANSI_RESET
		YANSI_YELLOW " [mode]\n" YANSI_RESET
		"\n",
		progname
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Execution mode " YANSI_RESET "\n\n"
		YANSI_YELLOW "  help\n" YANSI_RESET
		"  Display this help. Same effect if the mode is not specified.\n\n"
		YANSI_YELLOW "  config\n" YANSI_RESET
		"  Ask questions in order to create the Arkiv configuration file, then declare\n"
		"  the local machine up to the Arkiv.sh service.\n\n"
		YANSI_YELLOW "  declare\n" YANSI_RESET
		"  Declare the local server to the Arkiv.sh service.\n"
		"  Useful when the configuration is cloned and the " YANSI_YELLOW "config" YANSI_RESET
		" command was not used.\n\n"
		YANSI_YELLOW "  backup\n" YANSI_RESET
		"  Perform the backup configured on Arkiv.sh service for this machine.\n"
		"  SHould be called by the cron daemon only.\n\n"
		YANSI_YELLOW "  restore latest|identifier\n" YANSI_RESET
		"  Perform the restore of the lastest backup or the backup with the\n"
		"  given identifier.\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Environment variables " YANSI_RESET "\n\n"
		YANSI_FAINT "  conf=/path/to/conf.ini\n" YANSI_RESET
		YANSI_RED "  Path to the configuration file\n" YANSI_RESET
		"  Default value: /opt/arkiv/etc/agent.ini\n\n"
		YANSI_FAINT "  logfile=/path/to/file.log\n" YANSI_RESET
		YANSI_RED "  Path to the log file\n" YANSI_RESET
		"  Default value: /var/log/arkiv-agent.log\n\n"
		YANSI_FAINT "  syslog=USER|LOCAL0|LOCAL1|LOCAL2|LOCAL3|LOCAL4|LOCAL5|LOCAL6|LOCAL7\n" YANSI_RESET
		YANSI_RED "  Enabling log on syslog\n" YANSI_RESET
		"  Sets the log to syslog, in addition to other logging mechanisms. The given\n"
		"  parameter value is used as the output facility (see "
		YANSI_LINK_STATIC("https://en.wikipedia.org/wiki/Syslog#Facility", "Wikipedia") ").\n\n"
		YANSI_FAINT "  debug_mode=true|yes|on|1\n" YANSI_RESET
		YANSI_RED "  Activation of the debug mode\n" YANSI_RESET
		"  Set log level to 'DEBUG' and display message on stderr.\n"
		"  Any value other than 'true', 'yes', 'on', '1' would be "
		"interpreted as false.\n"
		"  Default value: false\n\n"
		YANSI_FAINT "  archives_path=/path/to/dir\n" YANSI_RESET
		YANSI_RED "  Path to the directory where local archives will be created\n" YANSI_RESET
		"  Default value: /var/archives\n\n"
		YANSI_FAINT "  crypt_pwd=...45 characters-long password...\n" YANSI_RESET
		YANSI_RED "  Encryption password\n" YANSI_RESET
		"  Used to override the encryption password defined in the configuration file.\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Examples " YANSI_RESET "\n\n"
		"  Start configuration:\n"
		YANSI_GREEN "  /opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "config\n\n" YANSI_RESET
		"  Launch backup with default parameters:\n"
		YANSI_GREEN "  /opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
		"  Use a specific log file:\n"
		YANSI_FAINT "  logfile=/root/arkiv.log " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
		"  Use an alternative configuration file:\n"
		YANSI_FAINT "  conf=/root/arkiv.ini " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
		"  Activate the debug mode, with a specific log file:\n"
		YANSI_FAINT "  debug_mode=true logfile=/root/arkiv.log " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
		"  Write log on syslog:\n"
		YANSI_FAINT "  syslog=USER " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Copyright, licence and source code " YANSI_RESET "\n\n"
		"  The Arkiv agent's source code is copyright © " YANSI_LINK_STATIC("mailto:amaury@amaury.net", "Amaury Bouchard") ".\n\n"
		"  It is released under the terms of the "
		YANSI_LINK_STATIC("https://joinup.ec.europa.eu/collection/eupl", "European Union Public Licence")
		" (EUPL),\n"
		"  version 1.2 or later. This licence is officially available in 23 languages,\n"
		"  and is compatible with other well-known licences: GPL (v2, v3), AGPL (v3),\n"
		"  LGPL (v2.1, v3), CC BY-SA (v3), MPL (v2), EPL (v1), OSL (v2.1, v3), LiLiQ-R\n"
		"  and LiLiQ-R+, CeCILL (v2.0, v2.1) and many " YANSI_LINK_STATIC("https://joinup.ec.europa.eu/collection/eupl/matrix-eupl-compatible-open-source-licences", "other OSI-approved licences") ".\n\n"
		"  Arkiv agent's source code could be find here: " YANSI_LINK_STATIC("https://developers.arkiv.sh", "https://developers.arkiv.sh") "\n\n"
	);
}

#if 0
/** Create a new agent structure. */
agent_t *_agent_new() {
	agent_t *agent = malloc0(sizeof(agent_t));
	agent->log.backup_files = yarray_new();
	agent->log.backup_databases = yarray_new();
	agent->log.upload_s3 = yarray_new();
	return (agent);
}
/**
 * Free a previously created agent structure.
 * @param       agent   Pointer to the allocated structure.
 */
void _agent_free(agent_t *agent) {
	if (!agent)
		return;
	ys_del(&agent->backup_path);
	free0(agent->conf.hostname);
	free0(agent->conf.public_key);
	free0(agent->conf.private_key);
	free0(agent->bin_path.mysqldump);
	free0(agent->bin_path.xtrabackup);
	free0(agent->bin_path.awscli);
	free0(agent->mysqldump.user);
	free0(agent->mysqldump.pwd);
	free0(agent->mysqldump.host);
	yarray_del(&agent->mysqldump.db, callback_free_raw, NULL);
	free0(agent->aws.public_key);
	free0(agent->aws.private_key);
	free0(agent->aws.bucket);
	free0(agent->aws.region);
	yarray_del(&agent->log.backup_files, callback_free_log_item, NULL);
	yarray_del(&agent->log.backup_databases, callback_free_log_item, NULL);
	yarray_del(&agent->log.upload_s3, callback_free_log_item, NULL);
	free0(agent);
}
#endif

