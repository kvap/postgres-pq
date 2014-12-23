/*-------------------------------------------------------------------------
 *
 * par_parallelizer.h
 *	  The query parallelizer subsystem of PargreSQL.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

Plan *par_Parallelize(Plan *plan, Query *query);
