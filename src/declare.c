#include <stdio.h>
#include "yansi.h"
#include "api.h"
#include "declare.h"

/* Declares the server to arkiv.sh API. */
void exec_declare(agent_t *agent) {
	printf("‣ Declare the host '" YANSI_PURPLE "%s" YANSI_RESET "' to "
	       YANSI_FAINT "arkiv.sh" YANSI_RESET "... ", agent->conf.hostname);
	if (api_server_declare(agent->conf.hostname, agent->conf.org_key) != YENOERR) {
		printf(
			YANSI_RED "failed\n\n" YANSI_RESET
			YANSI_FAINT "  Check the organization key and try again.\n\n" YANSI_RESET
			YANSI_RED "Abort.\n" YANSI_RESET
		);
		exit(2);
	}
	printf(YANSI_GREEN "done\n" YANSI_RESET);
}

