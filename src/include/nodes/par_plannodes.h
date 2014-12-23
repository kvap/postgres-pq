#ifdef PAR_PLANNODES_H
#error "par_plannodes.h is already included elsewhere"
#else // PAR_PLANNODES_H is undefined
#define PAR_PLANNODES_H

/* ----------------
 *		PargreSQL split node
 * ----------------
 */
typedef struct Split
{
	Plan		plan;
} Split;

/* ----------------
 *		PargreSQL merge node
 * ----------------
 */
typedef struct Merge
{
	Plan		plan;
} Merge;

/* ----------------
 *		PargreSQL scatter node
 * ----------------
 */
typedef struct Scatter
{
	Plan		plan;
	int		port; 
} Scatter;

/* ----------------
 *		PargreSQL gather node
 * ----------------
 */
typedef struct Gather
{
	Plan		plan;
	int		port; 
} Gather;

#endif
