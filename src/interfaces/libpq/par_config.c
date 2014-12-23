/*-----------------------------------------------------------------------------
 *
 * par_config.c
 * 	This file contains implementation for functions used by par_libpq-fe to
 * 	load its configuration.
 *
 * 2010, Constantin S. Pan
 *
 *-----------------------------------------------------------------------------
 */

#include "par_config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void par_config_resize(par_config *conf, const int new_cap)
{
	int i;
	for (i = new_cap; i < conf->capacity; i++)
	{
		free(conf->conninfo[i]);
	}
	conf->conninfo = realloc(conf->conninfo, sizeof(char*) * new_cap);
	for (i = conf->capacity; i < new_cap; i++)
	{
		conf->conninfo[i] = NULL;
		//conf->conninfo[i] = malloc(sizeof(char) * INIT_CONNINFO_LEN);
	}
	conf->capacity = new_cap;
}

par_config *par_config_load(const char *filename)
{
	par_config *conf = malloc(sizeof(par_config));
	conf->nodes_count = 0;
	conf->capacity = 0;
	conf->conninfo = NULL;
	par_config_resize(conf, INIT_CAPACITY);

	FILE *f = fopen(filename, "r");
	if (f == NULL)
	{
		puts("Cannot open PargreSQL config file");
	}
	else
	{
		char *line = NULL;
		size_t len;
		ssize_t charsread;
		while ((charsread = getline(&line, &len, f)) != -1)
		{
			char *c;
			if (c = strchr(line, '\n')) // remove trailing '\n'
			{
				*c = '\0';
			}
			if (c = strchr(line, '#')) // remove comments
			{
				*c = '\0';
			}

			if (line[0] != '\0')
			{
				conf->nodes_count++;
				if (conf->nodes_count == conf->capacity)
				{
					par_config_resize(conf, conf->capacity * 2);
				}
				conf->conninfo[conf->nodes_count - 1] = line;
				line = NULL;
			}
		}
		fclose(f);
	}
	return conf;
}

void par_config_unload(par_config *conf)
{
	par_config_resize(conf, 0); // FIXME: WTF this causes SEGFAULT!?
	free(conf);
}

#ifdef TEST
int main()
{
	par_config *c = par_config_load("par_libpq.conf");
	int i;
	for (i = 0; i < c->nodes_count; i++)
	{
		puts(c->conninfo[i]);
	}
	return 0;
}
#endif

