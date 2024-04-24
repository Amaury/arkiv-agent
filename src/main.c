/**
 * Arkiv agent.
 * Command line:
 * ./agent
 * debug_mode=true ./agent
 * logfile=/var/log/arkiv/arkiv.log ./agent
 * loglevel=WARN ./agent
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2019, Amaury Bouchard
 */

//#include "agent.h"
//#include "http.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "yansi.h"
#include "configuration.h"
#include "agent.h"

/* *** declaration of private functions *** */
void _agent_usage(char *progname);
//agent_t *_agent_new(void);
//void _agent_free(agent_t *agent);

/** Constant: CLI option for configuration. */
#define	OPT_CONFIG	"config"
/** Constant: CLI option for declaration. */
#define OPT_DECLARE	"declare"
/** Constant: CLI option for backup. */
#define	OPT_BACKUP	"backup"
/** Constant: CLI option for restore. */
#define	OPT_RESTORE	"restore"

/**
 * Main function of the program.
 */
int main(int argc, char *argv[]) {
	printf("'%s'\n", A_API_URL_SERVER_PARAMS);
	exit(0);

	// management of execution option
	if (argc == 2 && !strcmp(argv[1], OPT_CONFIG)) {
		exec_configuration();
	} else if (argc == 2 && !strcmp(argv[1], OPT_BACKUP)) {
		printf("agent declare\n");
	/*} else if (argc == 2 && !strcmp(argv[1], OPT_BACKUP)) {
		printf("agent_backup();\n");
	} else (argc == 3 && !strcmp(argv[1], OPT_RESTORE)) {
		printf("agent_restore(argv[2]);\n");*/
	} else {
		// bad option: display usage and quit
		_agent_usage(argv[0]);
		exit(1);
	}
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
void _agent_usage(char *progname) {
	printf("\n");
	printf(YANSI_BG_BLUE "%80c" YANSI_RESET "\n", ' ');
	printf(YANSI_BG_BLUE YANSI_WHITE "%31c%s%30c" YANSI_RESET "\n", ' ', "Arkiv.sh agent help", ' ');
	printf(YANSI_BG_BLUE "%80c" YANSI_RESET "\n", ' ');
	printf("\n");
	printf(YANSI_BLUE "  arkiv-agent " YANSI_RESET
	       "is a program used to backup a computer, using the parameters\n"
	       "  declared on the "
	       YANSI_UNDERLINE "Arkiv.sh" YANSI_RESET
	       " service. It shoud be called automatically (by the\n  cron daemon) "
	       "and not by a user, appart from during its installation process.\n\n"

	       YANSI_BG_GRAY YANSI_WHITE " Usage " YANSI_RESET "\n\n"
	       YANSI_FAINT "  [envvars] " YANSI_RESET
	       YANSI_BLUE "/usr/local/bin/arkiv-agent " YANSI_RESET
	       YANSI_YELLOW "[mode]\n" YANSI_RESET
	       "\n"

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

	       YANSI_BG_GRAY YANSI_WHITE " Environment variables " YANSI_RESET "\n\n"
	       YANSI_FAINT "  conf=/path/to/conf.ini\n" YANSI_RESET
	       YANSI_RED "  Path to the configuration file\n" YANSI_RESET
	       "  Default value: /opt/arkiv/etc/agent.ini\n\n"
	       YANSI_FAINT "  logfile=/path/to/file.log\n" YANSI_RESET
	       YANSI_RED "  Path to the log file\n" YANSI_RESET
	       "  Default value: /opt/arkiv/log/agent.log\n\n"
	       YANSI_FAINT "  debug_mode=true|yes|on|1\n" YANSI_RESET
	       YANSI_RED "  Activation of the debug mode\n" YANSI_RESET
	       "  Set log level to 'DEBUG' and display message on stderr.\n"
	       "  Any value other than 'true', 'yes', 'on' or '1' would be "
	       "interpreted as false.\n"
	       "  Default value: false\n"
	       "\n"

	       YANSI_BG_GRAY YANSI_WHITE " Examples " YANSI_RESET "\n\n"
	       "  Default:\n"
	       YANSI_BLUE "  /opt/arkiv/bin/agent " YANSI_RESET
	       YANSI_YELLOW "backup\n\n" YANSI_RESET
	       "  Set log level and log file:\n"
	       YANSI_FAINT "  loglevel=WARN logfile=/root/arkiv.log " YANSI_RESET
	       YANSI_BLUE "/opt/arkiv/bin/agent " YANSI_RESET
	       YANSI_YELLOW "backup\n\n" YANSI_RESET
	       "  Use an alternate configuration:\n"
	       YANSI_FAINT "  conf=/root/arkiv.ini " YANSI_RESET
	       YANSI_BLUE "/opt/arkiv/bin/agent " YANSI_RESET
	       YANSI_YELLOW "backup\n\n" YANSI_RESET
	       "  Activate the debug mode, with a specific log file:\n"
	       YANSI_FAINT "  debug_mode=true logfile=/root/arkiv.log " YANSI_RESET
	       YANSI_BLUE "/opt/arkiv/bin/agent " YANSI_RESET
	       YANSI_YELLOW "backup\n\n" YANSI_RESET
	       "  Install the client (should not be executed directly):\n"
	       YANSI_BLUE "  /opt/arkiv/bin/agent " YANSI_RESET
	       YANSI_YELLOW "install\n" YANSI_RESET
	       "\n");
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

