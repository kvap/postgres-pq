#ifdef PAR_PLANMAIN_H
#error "par_planmain.h is already included elsewhere"
#else // PAR_PLANMAIN_H is undefined
#define PAR_PLANMAIN_H

// These methods are for internal use in make_exchange.
extern Split *make_split(Plan *lefttree, Plan *righttree);
extern Merge *make_merge(Plan *lefttree, Plan *righttree);
extern Scatter *make_scatter(Plan *sibling, int port, int fragattr);
extern Gather *make_gather(Plan *sibling, int port);

// Use these in Parallelizer.
extern Plan *make_exchange(Plan *plan, int port, int fragattr);

#endif
