/**
 * Arkiv agent.
 * Command line:
 * ./agent
 * debug=true ./agent
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
#include "log.h"

/* *** declaration of private functions *** */
void _agent_usage(const char *progname);
//agent_t *_agent_new(void);
//void _agent_free(agent_t *agent);

typedef enum {
	A_TYPE_USAGE = 0,
	A_TYPE_VERSION,
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
		(argc == 2 && !strcmp(argv[1], A_OPT_VERSION)) ? A_TYPE_VERSION :
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
	} else if (exec_type == A_TYPE_VERSION) {
		// version
		printf("%.01f\n", A_AGENT_VERSION);
	} else if (exec_type == A_TYPE_CONFIG) {
		// load configuration file
		agent_load_configuration(agent, true);
		// configuration
		exec_configuration(agent);
	} else {
		// load configuration file
		agent_load_configuration(agent, false);
		// show debug data
		ADEBUG_RAW(YANSI_NEGATIVE "------------------------- DEBUG VARIABLES -------------------------" YANSI_RESET);
		ADEBUG_RAW("agent_path           : '" YANSI_FAINT "%s" YANSI_RESET "'", agent->agent_path);
		ADEBUG_RAW("conf_path            : '" YANSI_FAINT "%s" YANSI_RESET "'", agent->conf_path);
		ADEBUG_RAW("execution timestamp  : '" YANSI_FAINT "%.f" YANSI_RESET "'", difftime(agent->exec_timestamp, (time_t)0));
		ADEBUG_RAW("conf.logfile         : '" YANSI_FAINT "%s" YANSI_RESET "'", agent->conf.logfile);
		ADEBUG_RAW("conf.archives_path   : '" YANSI_FAINT "%s" YANSI_RESET "'", agent->conf.archives_path);
		ADEBUG_RAW("conf.org_key         : '" YANSI_FAINT "%s" YANSI_RESET "'", agent->conf.org_key);
		ADEBUG_RAW("conf.hostname        : '" YANSI_FAINT "%s" YANSI_RESET "'", agent->conf.hostname);
		ADEBUG_RAW("conf.crypt_pwd       : '" YANSI_FAINT "%s" YANSI_RESET "'", agent->conf.crypt_pwd);
		ADEBUG_RAW("conf.use_syslog      : '" YANSI_FAINT "%s" YANSI_RESET "'", agent->conf.use_syslog ? "true" : "false");
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
		"is a program designed to back up a computer, using the parameters\n"
		"  defined on the " YANSI_FAINT "Arkiv.sh" YANSI_RESET
		" service. It shoud be executed automatically by the\n"
		"  cron daemon and not manually by a user, except during its installation\n"
		"  process.\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Account creation " YANSI_RESET "\n\n"
		"  Before installing and configuring the Arkiv agent, you need to create an\n"
		"  account on the " YANSI_LINK_STATIC("https://www.arkiv.sh/", "Arkiv.sh") " website. "
		"Free accounts can manage one host, while paid\n  accounts can manage multiple hosts.\n"
		"  You will also need to retrieve the " YANSI_FAINT "organization key" YANSI_RESET
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
		"  Displays this help. This is also the default if no mode is specified.\n\n"
		YANSI_YELLOW "  version\n" YANSI_RESET
		"  Displays the version number of the installed agent.\n\n"
		YANSI_YELLOW "  config\n" YANSI_RESET
		"  Prompts for information to create the Arkiv configuration file and then\n"
		"  delcares the local machine to the Arkiv.sh service.\n\n"
		YANSI_YELLOW "  declare\n" YANSI_RESET
		"  Declares the local machine to the Arkiv.sh service.\n"
		"  Useful if the configuration is cloned and the " YANSI_FAINT "config" YANSI_RESET
		" mode was not used.\n\n"
		YANSI_YELLOW "  backup\n" YANSI_RESET
		"  Performs the backup configured on the Arkiv.sh service for this machine.\n"
		"  SHould be called by the cron daemon only.\n\n"
		//YANSI_YELLOW "  restore latest|identifier\n" YANSI_RESET
		//"  Perform the restore of the lastest backup or the backup with the\n"
		//"  given identifier.\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Environment variables " YANSI_RESET "\n\n"
		YANSI_RED "  Path to the configuration file\n" YANSI_RESET
		YANSI_BOLD "  conf" YANSI_RESET "=/path/to/conf.json\n"
		YANSI_FAINT "  Defines the path to the configuration file to use in place of the default one.\n" YANSI_RESET
		YANSI_FAINT "  Default value: " YANSI_CYAN "/opt/arkiv/etc/agent.json\n\n" YANSI_RESET

		YANSI_RED "  Path to the log file\n" YANSI_RESET
		YANSI_BOLD "  logfile" YANSI_RESET "=/path/to/file.log\n"
		YANSI_FAINT "  Use " YANSI_RESET YANSI_YELLOW "/dev/null" YANSI_RESET YANSI_FAINT " to disable file-based logging.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/var/log/arkiv-agent.log\n\n" YANSI_RESET

		YANSI_RED "  Enabling log on syslog\n" YANSI_RESET
		YANSI_BOLD "  syslog" YANSI_RESET "=true\n"
		YANSI_FAINT "  Enables logging to syslog in addition to other logging mechanisms.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_RED "  Activation of the debug mode\n" YANSI_RESET
		YANSI_BOLD "  debug" YANSI_RESET "=true\n"
		YANSI_FAINT "  Sets the log level to DEBUG, causing the program to write more log messages.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_RED "  Log on STDOUT\n" YANSI_RESET
		YANSI_BOLD "  stdout" YANSI_RESET "=true\n"
		YANSI_FAINT "  Activates logging to the program's standard output.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_RED "  Path to the archives directory\n" YANSI_RESET
		YANSI_BOLD "  archives_path" YANSI_RESET "=/path/to/dir\n"
		YANSI_FAINT "  Specifies the directory where local archives will be created.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/var/archives\n\n" YANSI_RESET

		YANSI_RED "  Encryption password\n" YANSI_RESET
		YANSI_BOLD "  crypt_pwd" YANSI_RESET "=...(40 characters-long password)...\n"
		YANSI_FAINT "  Overrides the encryption password defined in the configuration file.\n\n" YANSI_RESET

		YANSI_RED "  Disable ANSI sequences in log file\n" YANSI_RESET
		YANSI_BOLD "  ansi" YANSI_RESET "=false\n"
		YANSI_FAINT "  Set to " YANSI_RESET "false" YANSI_FAINT " to disable ANSI control sequences (color, etc.) in the log file.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "true\n\n" YANSI_RESET
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
		YANSI_FAINT "  debug=true logfile=/root/arkiv.log " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
		"  Write logs to syslog:\n"
		YANSI_FAINT "  syslog=true " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Copyright, licence and source code " YANSI_RESET "\n\n"
		"  The Arkiv agent's source code is © " YANSI_LINK_STATIC("mailto:amaury@amaury.net", "Amaury Bouchard") ".\n\n"
		"  It is released under the terms of the "
		YANSI_LINK_STATIC("https://joinup.ec.europa.eu/collection/eupl", "European Union Public Licence")
		" (EUPL),\n"
		"  version 1.2 or later. This licence is officially available in 23 languages\n"
		"  and is compatible with other well-known licences, including GPL (v2, v3),\n"
		"  AGPL (v3), LGPL (v2.1, v3), CC BY-SA (v3), MPL (v2), EPL (v1), OSL (v2.1, v3),\n"
		"  LiLiQ-R and LiLiQ-R+, CeCILL (v2.0, v2.1) and many " YANSI_LINK_STATIC("https://joinup.ec.europa.eu/collection/eupl/matrix-eupl-compatible-open-source-licences", "other OSI-approved licences") ".\n\n"
		"  The Arkiv agent's source code can be found here: " YANSI_LINK_STATIC("https://developers.arkiv.sh", "https://developers.arkiv.sh") "\n\n"
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

