/*-----------------------------------------------------------------------------
 *
 * par_Compat.h
 * 	This file contains macros for compatibility with PostgreSQL applications.
 *
 * 2010, Constantin S. Pan
 *
 *-----------------------------------------------------------------------------
 */

#ifndef PAR_COMPAT_H
#define PAR_COMPAT_H

#ifndef PAR_NO_COMPAT // if compatibility is being needed
	#define PGconn par_PGconn
	#define PQconnectdb(X) par_PQconnectdb()
	#define PQfinish(X) par_PQfinish(X)
	#define PQstatus(X) par_PQstatus(X)
	#define PQexec(X,Y) par_PQexec(X,Y)
#endif

#endif
