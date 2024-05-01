/**
 * @header	declare.h
 *
 * Definitions used for the declare of the local computer to the Arkiv.sh service.
 *
 * @author	Amaury Bouchard <amaury@amaury.net>
 * @copyright	© 2024, Amaury Bouchard
 */
#pragma once

#include "agent.h"

/**
 * @function	exec_declare
 * @abstract	Declares the server to arkiv.sh API.
 * @param	agent	Pointer to the agent structure.
 */
void exec_declare(agent_t *agent);

