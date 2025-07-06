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

/**
 * Execution types, representing the command line execution parameter.
 * @constant	A_TYPE_USAGE	For 'usage' execution.
 * @constant	A_TYPE_VERSION	For 'version' execution.
 * @constant	A_TYPE_CONFIG	For 'config' execution.
 * @constant	A_TYPE_DECLARE	For 'declare' execution.
 * @constant	A_TYPE_BACKUP	For 'backup' execution.
 * @constant	A_TYPE_RESTORE	For 'restore' execution.
 */
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
		ADEBUG_RAW("agent_path           : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->agent_path);
		ADEBUG_RAW("conf_path            : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf_path);
		ADEBUG_RAW("execution timestamp  : \"" YANSI_FAINT "%.f" YANSI_RESET "\"", difftime(agent->exec_timestamp, (time_t)0));
		ADEBUG_RAW("conf.standalone      : " YANSI_FAINT "%s" YANSI_RESET, agent->conf.standalone ? "true" : "false");
		ADEBUG_RAW("conf.hostname        : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf.hostname);
		ADEBUG_RAW("conf.org_key         : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf.org_key);
		ADEBUG_RAW("conf.scripts_allowed : " YANSI_FAINT "%s" YANSI_RESET, agent->conf.scripts_allowed ? "true" : "false");
		ADEBUG_RAW("conf.archives_path   : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf.archives_path);
		ADEBUG_RAW("conf.logfile         : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf.logfile);
		ADEBUG_RAW("conf.use_syslog      : " YANSI_FAINT "%s" YANSI_RESET, agent->conf.use_syslog ? "true" : "false");
		ADEBUG_RAW("conf.use_stdout      : " YANSI_FAINT "%s" YANSI_RESET, agent->conf.use_stdout ? "true" : "false");
		ADEBUG_RAW("conf.use_ansi        : " YANSI_FAINT "%s" YANSI_RESET, agent->conf.use_ansi ? "true" : "false");
		ADEBUG_RAW("conf.crypt_pwd       : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf.crypt_pwd);
		ADEBUG_RAW("conf.param_url       : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf.param_url);
		ADEBUG_RAW("conf.api_base_url    : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf.api_base_url);
		ADEBUG_RAW("conf.param_file      : \"" YANSI_FAINT "%s" YANSI_RESET "\"", agent->conf.param_file);
		ADEBUG_RAW("\n");
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
		YANSI_BOLD YANSI_LIGHT_BLUE "  arkiv-agent " YANSI_RESET "is a program designed to back up a computer. It is best used\n"
		"  with a " YANSI_BOLD "centralized server" YANSI_RESET ", which provides a graphical interface to\n"
		"  configure and manage the program.\n"
		"  By default, it connects to " YANSI_LIGHT_BLUE YANSI_LINK_STATIC("https://www.arkiv.sh/", "Arkiv.sh") YANSI_RESET ", but any compatible server can be used.\n\n"
		"  It can also run in " YANSI_BOLD "standalone mode" YANSI_RESET ", without connecting to a server. In that\n"
		"  case, the graphical interface, backup history, and centralized management\n"
		"  features won’t be available.\n\n"
		"  The Arkiv agent should be run by the " YANSI_BOLD "root" YANSI_RESET " user. It is intended to be executed\n"
		"  automatically by the cron daemon, not manually by a user (except during the\n"
		"  installation process).\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Account creation " YANSI_RESET "\n\n"
		"  If you plan to use the " YANSI_LINK_STATIC("https://www.arkiv.sh/", "Arkiv.sh") " service, you must create an account on the\n"
		"  website before installing and configuring the Arkiv agent.\n"
		"  You will also need to retrieve the " YANSI_FAINT "organization key" YANSI_RESET " provided by the service.\n\n"
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
		"  declares the local machine to the Arkiv.sh service.\n\n"
		YANSI_YELLOW "  declare\n" YANSI_RESET
		"  Declares the local machine to the Arkiv.sh service.\n"
		"  Useful if the configuration is cloned and the " YANSI_FAINT "config" YANSI_RESET
		" mode was not used.\n\n"
		YANSI_YELLOW "  backup\n" YANSI_RESET
		"  Performs the backup configured on the Arkiv.sh service for this machine.\n"
		"  Should be triggered by the cron daemon only.\n\n"
		//YANSI_YELLOW "  restore latest|identifier\n" YANSI_RESET
		//"  Perform the restore of the lastest backup or the backup with the\n"
		//"  given identifier.\n\n"
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Environment variables " YANSI_RESET "\n\n"
		"  These environment variables override the parameters set in the configuration\n"
		"  file (see below).\n\n"

		YANSI_BOLD "  conf" YANSI_RESET "=/path/to/conf.json\n"
		YANSI_FAINT "  Specifies the path to the configuration file.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/opt/arkiv/etc/agent.json\n\n" YANSI_RESET

		YANSI_BOLD "  standalone" YANSI_RESET "=true\n"
		YANSI_FAINT "  Enables the standalone mode. The agent will use local configuration file\n" YANSI_RESET
		YANSI_FAINT "  (see " YANSI_RESET YANSI_YELLOW "local_paramter_file" YANSI_RESET YANSI_FAINT ") and will not connect to any external API.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_BOLD "  hostname" YANSI_RESET "=host_name\n"
		YANSI_FAINT "  Specifies the name of the local machine.\n"
		"  Default value: " YANSI_FAINT "the output of the " YANSI_RESET YANSI_YELLOW "hostname" YANSI_RESET " program\n\n" YANSI_RESET

		YANSI_BOLD "  org_key" YANSI_RESET "=...\n"
		YANSI_FAINT "  45 characters-long organization key, provided by Arkiv.sh service (or another\n" YANSI_RESET
		YANSI_FAINT "  compatible service).\n\n" YANSI_RESET

		YANSI_BOLD "  scripts" YANSI_RESET "=false\n"
		YANSI_FAINT "  Specifies whether pre- and post-execution scripts are allowed.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "true\n\n" YANSI_RESET

		YANSI_BOLD "  archives_path" YANSI_RESET "=/path/to/dir\n"
		YANSI_FAINT "  Specifies the directory where local archives will be created.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/var/archives\n\n" YANSI_RESET

		YANSI_BOLD "  logfile" YANSI_RESET "=/path/to/file.log\n"
		YANSI_FAINT "  Use " YANSI_RESET YANSI_YELLOW "/dev/null" YANSI_RESET YANSI_FAINT " to disable file-based logging.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/var/log/arkiv.log\n\n" YANSI_RESET

		YANSI_BOLD "  syslog" YANSI_RESET "=true\n"
		YANSI_FAINT "  Enables logging to syslog in addition to other logging mechanisms.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_BOLD "  stdout" YANSI_RESET "=true\n"
		YANSI_FAINT "  Enables logging to the program's standard output.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_BOLD "  ansi" YANSI_RESET "=false\n"
		YANSI_FAINT "  Enables or disables ANSI color codes and formatting in the log file.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "true\n\n" YANSI_RESET

		YANSI_BOLD "  crypt_pwd" YANSI_RESET "=...(40 characters-long password)...\n"
		YANSI_FAINT "  Overrides the encryption password defined in the configuration file.\n\n" YANSI_RESET

		YANSI_BOLD "  api_url" YANSI_RESET "=https://url\n"
		YANSI_FAINT "  Specifies the base URL of the server API, used to declare the host and to send\n" YANSI_RESET
		YANSI_FAINT "  backup reports.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "https://api.arkiv.sh/v1\n\n" YANSI_RESET

		YANSI_BOLD "  param_url" YANSI_RESET "=https://url\n"
		YANSI_FAINT "  Specifies the URL of the host parameter file. This URL may contain:\n" YANSI_RESET
		YANSI_FAINT "  - " YANSI_RESET YANSI_YELLOW "[ORG]" YANSI_RESET YANSI_FAINT " for the organization key\n" YANSI_RESET
		YANSI_FAINT "  - " YANSI_RESET YANSI_YELLOW "[HOST]" YANSI_RESET YANSI_FAINT " for the host name\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "https://conf.arkiv.sh/v1/[ORG]/[HOST]/backup.json\n\n" YANSI_RESET

		YANSI_BOLD "  param_file" YANSI_RESET "=/path/to/params.json\n"
		YANSI_FAINT "  Specifies the path to the file used to store the host parameters.\n" YANSI_RESET
		YANSI_FAINT "  It is used to store a copy of the fetched parameters (see " YANSI_RESET YANSI_YELLOW "param_url" YANSI_RESET YANSI_FAINT ") or to\n" YANSI_RESET
		YANSI_FAINT "  store the used set of parameters (see " YANSI_RESET YANSI_YELLOW "standalone" YANSI_RESET YANSI_FAINT ")\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/opt/arkiv/etc/backup.json\n\n" YANSI_RESET

		YANSI_BOLD "  debug" YANSI_RESET "=true\n"
		YANSI_FAINT "  Sets the log level to DEBUG, causing the program to write more log messages.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET
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
		"  Enables the debug mode, with a specific log file:\n"
		YANSI_FAINT "  debug=true logfile=/root/arkiv.log " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
		"  Write logs to syslog:\n"
		YANSI_FAINT "  syslog=true " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
		"  Standalone mode:\n"
		YANSI_FAINT "  standalone=true " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
		"  Use a server which is not arkiv.sh:\n"
		YANSI_FAINT "  param_url=https://mysite.com/config.json api_url=https://mysite.com/api " YANSI_RESET
		YANSI_GREEN "/opt/arkiv/bin/agent " YANSI_RESET
		YANSI_YELLOW "backup\n\n" YANSI_RESET
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Configuration file " YANSI_RESET "\n\n"
		"  The configuration file is created by the Arkiv agent when run in " YANSI_YELLOW "config" YANSI_RESET " mode.\n"
		"  In most cases, it should not be edited manually.\n"
		"  By default, this file is stored at " YANSI_FAINT "/opt/arkiv/etc/agent.json" YANSI_RESET " but another\n"
		"  location can be specified with the " YANSI_YELLOW "conf" YANSI_RESET " environment variable.\n\n"
		"  " YANSI_BG_BLUE YANSI_LIME "                                                                              " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "  {                                                                           " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"standalone\":    false,                                                 " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"hostname\":      \"host_name\",                                           " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"org_key\":       \"organization key\",                                    " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"scripts\":       true,                                                  " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"archives_path\": \"/var/archives\",                                       " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"logfile\":       \"/var/log/arkiv.log\",                                  " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"syslog\":        false,                                                 " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"stdout\":        false,                                                 " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"ansi\":          false,                                                 " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"crypt_pwd\":     \"encryption password\",                                 " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"api_url\":       \"https://api.arkiv.sh/v1\",                             " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"param_url\":     \"https://conf.arkiv.sh/v1/[ORG]/[HOST]/param.json\",    " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"param_file\":    \"/opt/arkiv/etc/param.json\",                           " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "      \"debug\":         false                                                  " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "  }                                                                           " YANSI_RESET "\n"
		"  " YANSI_BG_BLUE YANSI_LIME "                                                                              " YANSI_RESET "\n"
		"\n\n"
	);
	printf(
		YANSI_BOLD "  standalone " YANSI_RESET YANSI_GREEN "(optional)\n" YANSI_RESET
		YANSI_FAINT "  Specifies wether standalone mode (no connection to a centralized service) is\n" YANSI_RESET
		YANSI_FAINT "  enabled.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_BOLD "  hostname " YANSI_RESET YANSI_GREEN "(optional)" YANSI_RESET "\n"
		YANSI_FAINT "  Name of the local host.\n" YANSI_RESET
		"  Default value: " YANSI_FAINT "the output of the " YANSI_RESET YANSI_YELLOW "hostname" YANSI_RESET YANSI_FAINT " program\n\n" YANSI_RESET

		YANSI_BOLD "  org_key " YANSI_RESET YANSI_GREEN "(mandatory when not standalone)" YANSI_RESET "\n"
		YANSI_FAINT "  45 characters-long organization key, provided by Arkiv.sh service (or another\n" YANSI_RESET
		YANSI_FAINT "  compatible service).\n\n" YANSI_RESET

		YANSI_BOLD "  scripts " YANSI_RESET YANSI_GREEN "(optional)" YANSI_RESET "\n"
		YANSI_FAINT "  Specifies whether pre- and post-execution scripts are allowed.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "true\n\n" YANSI_RESET

		YANSI_BOLD "  archives_path " YANSI_RESET YANSI_GREEN "(optional)" YANSI_RESET "\n"
		YANSI_FAINT "  Specifies the directory where local archives will be created.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/var/archives\n\n" YANSI_RESET

		YANSI_BOLD "  logfile " YANSI_RESET YANSI_GREEN "(optional)" YANSI_RESET "\n"
		YANSI_FAINT "  Use " YANSI_RESET YANSI_YELLOW "/dev/null" YANSI_RESET YANSI_FAINT " to disable file-based logging.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/var/log/arkiv.log\n\n" YANSI_RESET

		YANSI_BOLD "  syslog " YANSI_RESET YANSI_GREEN "(optional)\n" YANSI_RESET
		YANSI_FAINT "  Enables logging to syslog in addition to other logging mechanisms.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_BOLD "  stdout " YANSI_RESET YANSI_GREEN "(optional)\n" YANSI_RESET
		YANSI_FAINT "  Enables logging to the program's standard output.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET

		YANSI_BOLD "  ansi " YANSI_RESET YANSI_GREEN "(optional)\n" YANSI_RESET
		YANSI_FAINT "  Enables or disables ANSI color codes and formatting in the log file.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "true\n\n" YANSI_RESET

		YANSI_BOLD "  crypt_pwd " YANSI_RESET YANSI_GREEN "(mandatory)\n" YANSI_RESET
		YANSI_FAINT "  Password used to encrypt files.\n\n" YANSI_RESET

		YANSI_BOLD "  api_url " YANSI_RESET YANSI_GREEN "(optional)\n" YANSI_RESET
		YANSI_FAINT "  Specifies the base URL of the server API, used to declare the host and to send\n" YANSI_FAINT
		YANSI_FAINT "  send backup reports.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "https://api.arkiv.sh/v1\n\n" YANSI_RESET

		YANSI_BOLD "  param_url " YANSI_RESET YANSI_GREEN "(optional)\n" YANSI_RESET
		YANSI_FAINT "  Specifies the URL of the host parameter file. This URL may contain:\n" YANSI_RESET
		YANSI_FAINT "  - " YANSI_RESET YANSI_YELLOW "[ORG]" YANSI_RESET YANSI_FAINT " for the organization key\n" YANSI_RESET
		YANSI_FAINT "  - " YANSI_RESET YANSI_YELLOW "[HOST]" YANSI_RESET YANSI_FAINT " for the host name\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "https://conf.arkiv.sh/v1/[ORG]/[HOST]/backup.json\n\n" YANSI_RESET

		YANSI_BOLD "  param_file " YANSI_RESET YANSI_GREEN "(optional)\n" YANSI_RESET
		YANSI_FAINT "  Specifies the path to the file used to store the host parameters.\n" YANSI_RESET
		YANSI_FAINT "  It is used to store a copy of the fetched parameters (see " YANSI_RESET YANSI_YELLOW "param_url" YANSI_RESET YANSI_FAINT ") or to\n" YANSI_RESET
		YANSI_FAINT "  store the used set of parameters (see " YANSI_RESET YANSI_YELLOW "standalone" YANSI_RESET YANSI_FAINT ")\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "/opt/arkiv/etc/backup.json\n\n" YANSI_RESET

		YANSI_BOLD "  debug " YANSI_RESET YANSI_GREEN "(optional)\n" YANSI_RESET
		YANSI_FAINT "  Sets the log level to DEBUG, causing the program to write more log messages.\n" YANSI_RESET
		"  Default value: " YANSI_CYAN "false\n\n" YANSI_RESET
	);
	printf(
		YANSI_BG_GRAY YANSI_WHITE " Copyright, licence and source code " YANSI_RESET "\n\n"
		"  The Arkiv agent is © " YANSI_LINK_STATIC("mailto:amaury@amaury.net", "Amaury Bouchard") ".\n\n"
		"  It is released under the terms of the "
		YANSI_LINK_STATIC("https://joinup.ec.europa.eu/collection/eupl", "European Union Public Licence")
		" (EUPL),\n"
		"  version 1.2 or later. This licence is officially available in 23 languages\n"
		"  and is compatible with other well-known licences, including GPL (v2, v3),\n"
		"  AGPL (v3), LGPL (v2.1, v3), CC BY-SA (v3), MPL (v2), EPL (v1), OSL (v2.1, v3),\n"
		"  LiLiQ-R and LiLiQ-R+, CeCILL (v2.0, v2.1) and many " YANSI_LINK_STATIC("https://joinup.ec.europa.eu/collection/eupl/matrix-eupl-compatible-open-source-licences", "other OSI-approved") "\n"
		"  " YANSI_LINK_STATIC("https://joinup.ec.europa.eu/collection/eupl/matrix-eupl-compatible-open-source-licences", "licenses") ".\n\n"
		"  The Arkiv agent's source code is available at: " YANSI_LINK_STATIC("https://developers.arkiv.sh", "https://developers.arkiv.sh") "\n\n"
	);
}

