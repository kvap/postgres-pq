/*-------------------------------------------------------------------------
 * nodeSplit.c
 *	  One of the four suboperations of the Exchange
 *
 * Copyright (c) 2011, Constantin S. Pan
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/par_tupack.h"
#include "executor/executor.h"
#include "executor/par_nodeSplit.h"
#include "executor/par_nodeScatter.h"

extern void printhex(char *name, char *buf, int len);

/* ----------------------------------------------------------------
 *		ExecSplit
 * ----------------------------------------------------------------
 */
TupleTableSlot *				/* return: a tuple or NULL */
ExecSplit(SplitState *node)
{
	//ScanDirection direction;
	TupleTableSlot *slot;
	PlanState *left;
	ScatterState *right;
	ItemPointer ctid;

	left = ((PlanState*)node)->lefttree; // any kind of plan
	right = (ScatterState*)((PlanState*)node)->righttree; // only a ScatterState is possible here

	int port = ((Scatter*)((PlanState*)right)->plan)->port;
        elog(DEBUG5, "split.next(port=%d)", port);

	if (node->sent_nulls)
	{
		elog(DEBUG5, "split: all the nulls sent, returning NULL");
		// The left son has returned a NULL,
		// and the right son has scattered the NULLs,
		// so we do nothing and return the NULL.
		node->status = PAR_OK;
		return NULL;
	}

	if (right->isSending)
	{
		elog(DEBUG5, "split: right was sending the last time, checking if it finished");
		// There will be no room in the right son's buffer,
		// in case we need to put there an alien tuple taken
		// from the left son.
		ExecProcNode((PlanState*)right);
		if (right->status == PAR_WAIT)
		{
			elog(DEBUG5, "split: right is still sending, returning WAIT");
			// The right son (i.e. Scatter) is still busy,
			// so we wait for him.
			node->status = PAR_WAIT;
			return slot;
		}
	}

	// The buffer of the right son is empty.
	// Advance the left son.
	slot = ExecProcNode(left);
	if (TupIsNull(slot))
	{
		// Got an EOF tuple from below - scatter it!
		elog(DEBUG5, "split: got an EOF from below, scattering it");
		right->upstreamTuple = NULL;
		ExecProcNode((PlanState*)right);
		node->sent_nulls = 1;
		return NULL;
	}
	else
	{
		int dst, rank, size, fragattr;

		// Get the fragattr value from Scatter
		fragattr = node->ps.plan->righttree->fragattr;

		rank = _pargresql_GetNode(); // FIXME: INIS naming
		size = _pargresql_GetNodesCount(); // FIXME: INIS naming
		// Got a normal tuple
		elog(DEBUG5, "split: calling fragfunc");
		dst = fragfunc(size, slot, fragattr);
		elog(DEBUG5, "split: fragfunc finished");
		if (dst == rank)
		{ // Native tuple, keep it
			StringInfoData sid;
			node->status = PAR_OK;
			elog(DEBUG5, "split: a native tuple (fragfunc == %d, data below)", dst);
			sid = par_tupack(slot);
			pfree(sid.data);
			printhex("the data", sid.data, sid.len);
			return slot;
		}
		else
		{ // Alien tuple, expel it!
			ItemPointerData ipdata;

			if (AttributeNumberIsValid(node->ctidatno)) {
				elog(DEBUG5, "split: mark as DELETE and expel");
				//ExecMaterializeSlot(slot); // FIXME: Somehow this breaks the modifications made to the tuple. Why?

				ctid = (ItemPointer)DatumGetPointer(slot->tts_values[node->ctidatno - 1]);
				ItemPointerCopy(ctid, &ipdata); // Save the old value of ctid
				ItemPointerSetInvalid(ctid); // Invalidate the ctid (mark as "INSERT ME")
				elog(DEBUG5, "split: INVALIDATED: Ctid.blkid is %u, Ctid.offset is %u", ItemPointerGetBlockNumber(ctid), ItemPointerGetOffsetNumber(ctid));
				slot->tts_values[node->ctidatno - 1] = PointerGetDatum(ctid);
				ctid = (ItemPointer)DatumGetPointer(slot->tts_values[node->ctidatno - 1]);
				elog(DEBUG5, "split: NEW INVALIDATED: Ctid.blkid is %u, Ctid.offset is %u", ItemPointerGetBlockNumber(ctid), ItemPointerGetOffsetNumber(ctid));

				// Scatter the modified tuple
				right->upstreamTuple = slot;
				ExecProcNode((PlanState*)right);

				ItemPointerCopy(&ipdata, ctid); // Return to the old value of ctid
				elog(DEBUG5, "split: Correct ctid.blkid was %u", ItemPointerGetBlockNumber(ctid));
				ItemPointerSetDeleteMe(ctid); // Set the "DELETE ME" flag
				elog(DEBUG5, "split: DeleteMe ctid.blkid became %u", ItemPointerGetBlockNumber(ctid));
				slot->tts_values[node->ctidatno - 1] = PointerGetDatum(ctid);

				node->status = PAR_OK; // We return a tuple flagged as "DELETE ME"
				return slot;
			} else {
				StringInfoData sid;
				elog(DEBUG5, "split: an alien tuple (fragfunc == %d, data below), expelling", dst);
				sid = par_tupack(slot);
				printhex("the data", sid.data, sid.len);
				pfree(sid.data);
				// Scatter the tuple
				right->upstreamTuple = slot;
				ExecProcNode((PlanState*)right);
				node->status = PAR_WAIT;
				return NULL;
			}
		}
	}
}

/* ----------------------------------------------------------------
 *		ExecInitSplit
 *
 *		This initializes the split node state structures and
 *		the node's subplan.
 * ----------------------------------------------------------------
 */
SplitState *
ExecInitSplit (Split *node, EState *estate, int eflags)
{
	SplitState *splitstate;
	Plan	   *childPlan;

	/* check for unsupported flags */
	Assert(!(eflags)); // FIXME: no flags supported

	/*
	 * create state structure
	 */
	splitstate = makeNode(SplitState);
	splitstate->ps.plan = (Plan *) node;
	splitstate->ps.state = estate;

	splitstate->status = PAR_OK;
	splitstate->sent_nulls = 0;

	/*
	 * initialize child expressions
	 */
	//splitstate->ps.targetlist = (List*)ExecInitExpr((Expr*) node->plan.targetlist, (PlanState*)splitstate);
	//splitstate->ps.qual = (List*)ExecInitExpr((Expr*) node->plan.qual, (PlanState*)splitstate);

	/*
	 * Tuple table initialization (XXX not actually used...)
	 */
	ExecInitResultTupleSlot(estate, &splitstate->ps);

	/*
	 * then initialize child plans
	 */
	outerPlanState(splitstate) = ExecInitNode(outerPlan(node), estate, eflags);
	innerPlanState(splitstate) = ExecInitNode(innerPlan(node), estate, eflags);

	/*
	 * split nodes do no projections, so initialize projection info for this
	 * node appropriately
	 */
	ExecAssignResultTypeFromTL(&splitstate->ps);
	splitstate->ps.ps_ProjInfo = NULL;

	/*
	 * Split behaves differently if there is a 'ctid' junk attribute.
	 */
	splitstate->ctidatno = ExecFindJunkAttributeInTlist(node->plan.targetlist, "ctid");
	if (AttributeNumberIsValid(splitstate->ctidatno)) {
		elog(DEBUG5, "split.init: junk 'ctid' attrno == %d", splitstate->ctidatno);
	} else {
		elog(DEBUG5, "split.init: no 'ctid' junk attribute detected");
	}

	return splitstate;
}

int
ExecCountSlotsSplit(Split *node)
{
	return ExecCountSlotsNode(outerPlan(node)) +
		ExecCountSlotsNode(innerPlan(node)) +
		1 /* our slot */;
}

/* ----------------------------------------------------------------
 *		ExecEndSplit
 *
 *		This shuts down the subplan and frees resources allocated
 *		to this node.
 * ----------------------------------------------------------------
 */
void
ExecEndSplit(SplitState *node)
{
	ExecEndNode(outerPlanState(node));
	ExecEndNode(innerPlanState(node));
}


void
ExecReScanSplit(SplitState *node, ExprContext *exprCtxt)
{
	ExecReScan(outerPlanState(node), exprCtxt);
	ExecReScan(innerPlanState(node), exprCtxt);
	node->sent_nulls = 0;
}
