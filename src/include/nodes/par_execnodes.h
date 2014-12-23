#ifdef PAR_EXECNODES_H
#error "par_execnodes.h is already included elsewhere"
#else // PAR_EXECNODES_H is undefined
#define PAR_EXECNODES_H

typedef enum {
	PAR_OK,
	PAR_WAIT
} ExchangeStatus;

typedef struct SplitState
{
	PlanState	ps;
	ExchangeStatus	status;
	int		ctidatno; // 'ctid' attribute index in target list (1-based), or InvalidAttrNumber if no 'ctid'
	int		sent_nulls; // true if we've encountered a null and scattered it
} SplitState;

typedef struct MergeState
{
	PlanState	ps;
	int		even; 
} MergeState;

typedef struct ScatterState
{
	PlanState	ps;
	TupleTableSlot	*upstreamTuple;
	ExchangeStatus	status;
	int		isSending; 
	_pargresql_request_t	request;
	void		*buf;
} ScatterState;

#define GATHER_BUFLEN 8192
typedef struct GatherState
{
	PlanState	ps;
	ExchangeStatus	status;
	int		nullcnt;
	_pargresql_request_t	*requests;
	void		**bufs;
} GatherState;

#endif
