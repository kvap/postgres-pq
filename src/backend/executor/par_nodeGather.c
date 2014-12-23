/*-------------------------------------------------------------------------
 * nodeGather.c
 *	  One of the four suboperations of the Exchange
 *
 * Copyright (c) 2011, Constantin S. Pan
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include <stdio.h>

#include "access/par_tupack.h"
#include "executor/executor.h"
#include "executor/par_nodeGather.h"
#include "par_inis/_pargresql_library.h" // FIXME: INIS naming

/* ----------------------------------------------------------------
 *		ExecGather
 * ----------------------------------------------------------------
 */
TupleTableSlot *				/* return: a tuple or NULL */
ExecGather(GatherState *node)
{
	//ScanDirection direction;
	TupleTableSlot *slot;
	TupleDesc tupledesc;
	int i;

	int rank = _pargresql_GetNode(); // FIXME: INIS naming
	int size = _pargresql_GetNodesCount(); // FIXME: INIS naming
	int port = ((Gather*)((PlanState*)node)->plan)->port;
	elog(DEBUG5, "gather(port=%d): %d nulls of %d", port, node->nullcnt, size - 1);
	if (node->nullcnt == size - 1)
	{
		// All EOFs have been gathered, return EOF
		node->status = PAR_OK;
		return NULL;
	}

	for (i = 0; i < size; i++)
	{
		if (i == rank) {
			continue; // don't get from yourself
		}
		if (node->bufs[i] == NULL)
		{
			// This node has alreary sent an EOF - skip the node
			continue;
		}
		int flag;
		_pargresql_Test(&node->requests[i], &flag);
		if (flag)
		{
			// Yahoo! Something recved from 'i'!
			char *buf = (char*)node->bufs[i];
			elog(DEBUG5, "gather(port=%d): got a message from node %d", port, i);
			if ((buf[0] == '\0') && (buf[1] == '\0'))
			{
				// EOF recved
				elog(DEBUG5, "gather(port=%d): that was EOF", port);
				pfree(buf);
				node->bufs[i] = NULL;
				node->nullcnt++;
				node->status = PAR_WAIT;
				return NULL;
			}
			else
			{
				// A tuple recved
				elog(DEBUG5, "gather(port=%d): that was a tuple", port);
				StringInfoData sid;
				//tupledesc = ExecTypeFromTL(((PlanState*)node)->plan->targetlist, false);
				//slot = MakeSingleTupleTableSlot(tupledesc);
				slot = ((PlanState*)node)->ps_ResultTupleSlot;
				sid.len = GATHER_BUFLEN;
				sid.data = buf;
				par_tunpack(sid, slot);

				_pargresql_IRecv(i, port, GATHER_BUFLEN, node->bufs[i], &node->requests[i]);
				node->status = PAR_OK;
				return slot;
			}
		}
	}

	node->status = PAR_WAIT;
	return NULL;
}

/* ----------------------------------------------------------------
 *		ExecInitGather
 *
 *		This initializes the gather node state structures and
 *		the node's subplan.
 * ----------------------------------------------------------------
 */
GatherState *
ExecInitGather (Gather *node, EState *estate, int eflags)
{
	GatherState *gatherstate;
	int i, port, size, rank;

	/* check for unsupported flags */
	Assert(!(eflags)); // FIXME: no flags supported

	/*
	 * create state structure
	 */
	gatherstate = makeNode(GatherState);
	gatherstate->ps.plan = (Plan *) node;
	gatherstate->ps.state = estate;

	port = ((Gather*)((PlanState*)gatherstate)->plan)->port;
	rank = _pargresql_GetNode(); // FIXME: INIS naming
	size = _pargresql_GetNodesCount(); // FIXME: INIS naming
	gatherstate->status = PAR_OK;
	gatherstate->nullcnt = 0;
	gatherstate->requests = palloc(size * sizeof(_pargresql_request_t));
	gatherstate->bufs = palloc(size * sizeof(void*));
	for (i = 0; i < size; i++) {
		if (i != rank) {
			gatherstate->bufs[i] = palloc0(GATHER_BUFLEN);
			_pargresql_IRecv(i, port, GATHER_BUFLEN, gatherstate->bufs[i], &gatherstate->requests[i]);
		}
	}

	/*
	 * Tuple table initialization
	 */
	ExecInitResultTupleSlot(estate, &gatherstate->ps);

	/*
	 * gather nodes do no projections, so initialize projection info for this
	 * node appropriately
	 */
	ExecAssignResultTypeFromTL(&gatherstate->ps);
	gatherstate->ps.ps_ProjInfo = NULL;

	return gatherstate;
}

int
ExecCountSlotsGather(Gather *node)
{
	return 1;
}

/* ----------------------------------------------------------------
 *		ExecEndGather
 *
 *		This shuts down the subplan and frees resources allocated
 *		to this node.
 * ----------------------------------------------------------------
 */
void
ExecEndGather(GatherState *node)
{
	//int size = _pargresql_GetNodesCount(); // FIXME: INIS naming
	//int i;
	pfree(node->bufs);
	pfree(node->requests);
}


void
ExecReScanGather(GatherState *node, ExprContext *exprCtxt)
{
	int size = _pargresql_GetNodesCount(); // FIXME: INIS naming
	int i;
	int port = ((Gather*)((PlanState*)node)->plan)->port;
	for (i = 0; i < size; i++) {
		node->bufs[i] = palloc(GATHER_BUFLEN);
//		printhex2("newbuf", (char*)node->bufs[i], 20);
		_pargresql_IRecv(i, port, GATHER_BUFLEN, node->bufs[i], &node->requests[i]);
	}
	node->status = PAR_OK;
	node->nullcnt = 0;
}
