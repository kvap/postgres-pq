/*-----------------------------------------------------------------------------
 *
 * par_config.h
 * 	This file contains definitions for structures and declarations for
 * 	functions used by par_libpq-fe to load its configuration.
 *
 * 2010, Constantin S. Pan
 *
 *-----------------------------------------------------------------------------
 */

#ifndef PAR_CONFIG
#define PAR_CONFIG

#define INIT_CAPACITY 1
#define INIT_CONNINFO_LEN 1024

typedef struct par_config
{
	int nodes_count;
	int capacity;
	char **conninfo;
} par_config;

par_config *par_config_load(const char *filename);
void par_config_unload(par_config *conf);

#endif
