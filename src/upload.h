/**
 * @header	upload.h
 * @abstract	Arkiv upload management.
 * @author	Amaury Bouchard <amaury@amaury.net>
 */
#pragma once

#include "ystatus.h"
#include "yvar.h"
#include "agent.h"

/**
 * @function	upload_files
 * @abstract	Upload backed up files to cloud storage.
 * @param	agent	Pointer to the agent structure.
 * @return	YENOERR if the declaration went well.
 */
void upload_files(agent_t *agent);

/* ********** PRIVATE DECLARATIONS ********** */
#ifdef __A_UPLOAD_PRIVATE__
	/**
	 * @function	upload_item_aws_s3
	 * @abstract	Upload a backed up file or database to AWS S3.
	 * @param	hash		Index of the item in its list.
	 * @param	key		Always null.
	 * @param	data		Pointer to the item.
	 * @param	user_data	Pointer to the agent structure.
	 * @return	YENOERR if the item has been upload successfully.
	 */
	static ystatus_t upload_item_aws_s3(uint64_t hash, char *key, void *data, void *user_data);
	/**
	 * @function	upload_create_env_aws_s3
	 * @abstract	Generates the list of environment variables for AWS S3 upload.
	 * @param	agent	Pointer to the agent structure.
	 * @return	The array of environment variables.
	 */
	static yarray_t upload_create_env_aws_s3(agent_t *agent);
#endif // __A_API_PRIVATE__

