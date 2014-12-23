/*-------------------------------------------------------------------------
 * nodeMerge.c
 *	  One of the four suboperations of the Exchange
 *
 * Copyright (c) 2011, Constantin S. Pan
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "executor/executor.h"
#include "executor/par_nodeMerge.h"

/* ----------------------------------------------------------------
 *		ExecMerge
 * ----------------------------------------------------------------
 */
TupleTableSlot *				/* return: a tuple or NULL */
ExecMerge(MergeState *node)
{
	//ScanDirection direction;
	TupleTableSlot *slot;
	GatherState *left;
	SplitState *right;

	left = (GatherState*)((PlanState*)node)->lefttree;
	right = (SplitState*)((PlanState*)node)->righttree;

	int port = ((Gather*)((PlanState*)left)->plan)->port;
	elog(DEBUG5, "merge(port=%d)", port);

	while (1)
	{
		node->even = !node->even;
		if (node->even)
		{ // right, then left
			elog(DEBUG5, "merge: even (split-gather)");
			// advance the right son
			slot = ExecProcNode((PlanState*)right);
			if (right->status == PAR_WAIT)
			{
				// the right son is busy, next iteration
				continue;
			}
			// the right son has advanced
			if (!TupIsNull(slot))
			{
				// the right son has a tuple, returning
				return slot;
			}

			// advance the left son
			slot = ExecProcNode((PlanState*)left);
			if (left->status == PAR_WAIT)
			{
				// the left son is busy, next iteration
				continue;
			}
			// the left son has advanced
			if (!TupIsNull(slot))
			{
				// the left son has a tuple, returning
				return slot;
			}
		}
		else
		{ // left, then right
			elog(DEBUG5, "merge: odd (gather-split)");
			// advance the left son
			slot = ExecProcNode((PlanState*)left);
			if (left->status == PAR_WAIT)
			{
				// the left son is busy, next iteration
				continue;
			}
			// the left son has advanced
			if (!TupIsNull(slot))
			{
				// the left son has a tuple, returning
				return slot;
			}

			// advance the right son
			slot = ExecProcNode((PlanState*)right);
			if (right->status == PAR_WAIT)
			{
				// the right son is busy, next iteration
				continue;
			}
			// the right son has advanced
			if (!TupIsNull(slot))
			{
				// the right son has a tuple, returning
				return slot;
			}
		}
		// EOF from both sons - return EOF
		return NULL;
	}
}

/* ----------------------------------------------------------------
 *		ExecInitMerge
 *
 *		This initializes the merge node state structures and
 *		the node's subplan.
 * ----------------------------------------------------------------
 */
MergeState *
ExecInitMerge (Merge *node, EState *estate, int eflags)
{
	MergeState *mergestate;
	//Plan	   *childPlan;

	/* check for unsupported flags */
	Assert(!(eflags)); // FIXME: no flags supported

	/*
	 * create state structure
	 */
	mergestate = makeNode(MergeState);
	mergestate->ps.plan = (Plan *) node;
	mergestate->ps.state = estate;

	mergestate->even = 0;

	/*
	 * Tuple table initialization (XXX not actually used...)
	 */
	ExecInitResultTupleSlot(estate, &mergestate->ps);

	/*
	 * then initialize child plans
	 */
	outerPlanState(mergestate) = ExecInitNode(outerPlan(node), estate, eflags);
	innerPlanState(mergestate) = ExecInitNode(innerPlan(node), estate, eflags);

	/*
	 * merge nodes do no projections, so initialize projection info for this
	 * node appropriately
	 */
	ExecAssignResultTypeFromTL(&mergestate->ps);
	mergestate->ps.ps_ProjInfo = NULL;

	return mergestate;
}

int
ExecCountSlotsMerge(Merge *node)
{
	return ExecCountSlotsNode(outerPlan(node)) +
		ExecCountSlotsNode(innerPlan(node)) +
		1 /* our slot */;
}

/* ----------------------------------------------------------------
 *		ExecEndMerge
 *
 *		This shuts down the subplan and frees resources allocated
 *		to this node.
 * ----------------------------------------------------------------
 */
void
ExecEndMerge(MergeState *node)
{
	ExecEndNode(outerPlanState(node));
	ExecEndNode(innerPlanState(node));
}


void
ExecReScanMerge(MergeState *node, ExprContext *exprCtxt)
{
	ExecReScan(outerPlanState(node), exprCtxt);
	ExecReScan(innerPlanState(node), exprCtxt);
	node->even = 0;
}
