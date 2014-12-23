/*-----------------------------------------------------------------------------
 *
 * par_libpq-fe.h
 * 	This file contains definitions for structures and declarations for
 * 	functions used by frontend pargres applications.
 *
 * 2010, Constantin S. Pan
 *
 *-----------------------------------------------------------------------------
 */

#ifndef PAR_LIBPQ_FE_H
#define PAR_LIBPQ_FE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "libpq-fe.h"
#include "par_Compat.h"

typedef struct par_PGconn
{
	int len; // number of connections
	struct pg_conn **conns; // the connections
} par_PGconn;

/* make new client connections to the backends */
extern par_PGconn *par_PQconnectdb(void);

/* close current connections and free the par_PGconn data structure */
extern void par_PQfinish(par_PGconn *conn);

extern ConnStatusType par_PQstatus(const par_PGconn *conn);

extern PGresult *par_PQexec(par_PGconn *conn, const char *query);

extern PGresult *par_PQexec_time(par_PGconn *conn, const char *query, float *dt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
