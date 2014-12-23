/*-----------------------------------------------------------------------------
 *
 * par_libpq-fe.h
 * 	This file contains the implementation of functions used by frontend
 * 	pargresql applications.
 *
 * 2010, Constantin S. Pan
 *
 *-----------------------------------------------------------------------------
 */

#define PAR_NO_COMPAT
#define PAR_CONFIG_FILENAME "par_libpq.conf"

#include "par_libpq-fe.h"
#include "libpq-int.h"
#include "par_config.h"
#include <stdlib.h>
#include <time.h>

par_PGconn *par_PQconnectdb(void)
{
	int i;
	par_config *conf = par_config_load(PAR_CONFIG_FILENAME);

	par_PGconn *conn = malloc(sizeof(par_PGconn));
	conn->len = conf->nodes_count;
	printf("using %d nodes\n", conn->len);
	conn->conns = malloc(conn->len * sizeof(PGconn*));
	for (i = 0; i < conn->len; i++)
	{
		conn->conns[i] = PQconnectdb(conf->conninfo[i]);
	}

	par_config_unload(conf);

	return conn;
}

void par_PQfinish(par_PGconn *conn)
{
	int i;
	for (i = 0; i < conn->len; i++) {
		PQfinish(conn->conns[i]);
	}
}

ConnStatusType par_PQstatus(const par_PGconn *conn)
{
	int i;
	for (i = 0; i < conn->len; i++) {
		if (PQstatus(conn->conns[i]) != CONNECTION_OK)
		{
			printf("connection %d (max is %d) (%s) is not OK, returned %d (%s)\n",
					i,
					conn->len,
					conn->conns[i]->pghost,
					PQstatus(conn->conns[i]),
					PQerrorMessage(conn->conns[i])
			);
			return CONNECTION_BAD;
		}
	}
	return CONNECTION_OK;
}

PGresult *par_PQexec(par_PGconn *conn, const char *query)
{
	int i;
	PGresult *r;
	for (i = 1; i < conn->len; i++) {
		//PGresult *ignore = PQexec(conn->conns[i], query);
		//PQclear(ignore);
		PQsendQuery(conn->conns[i], query); // asynchronous command
	}
	r = PQexec(conn->conns[0], query); // synchronous command
	for (i = 1; i < conn->len; i++) {
		PGresult *ignore;
		while ((ignore = PQgetResult(conn->conns[i])) != NULL) {
			PQclear(ignore);
		}
	}
	return r;
}

// Returns current monotonic time in seconds
float now_s() {
	struct timespec t;
	if (clock_gettime(CLOCK_MONOTONIC, &t) == 0) {
		return t.tv_sec + t.tv_nsec * 1e-9;
	} else {
		return 0;
	}
}

PGresult *par_PQexec_time(par_PGconn *conn, const char *query, float *dt)
{
	int i;
	PGresult *r;
	for (i = 1; i < conn->len; i++) {
		//PGresult *ignore = PQexec(conn->conns[i], query);
		//PQclear(ignore);
		PQsendQuery(conn->conns[i], query); // asynchronous command
	}
	float t = now_s();
	r = PQexec(conn->conns[0], query); // synchronous command
	*dt = now_s() - t;
	for (i = 1; i < conn->len; i++) {
		PGresult *ignore;
		while ((ignore = PQgetResult(conn->conns[i])) != NULL) {
			PQclear(ignore);
		}
	}
	return r;
}

#undef PAR_NO_COMPAT
