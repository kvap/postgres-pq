/*-------------------------------------------------------------------------
 * nodeSplit.h
 *
 * Copyright (c) 2011, Constantin S. Pan
 *-------------------------------------------------------------------------
 */
#ifndef PAR_NODESPLIT_H
#define PAR_NODESPLIT_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsSplit(Split *node);
extern SplitState *ExecInitSplit(Split *node, EState *estate, int eflags);
extern TupleTableSlot *ExecSplit(SplitState *node);
extern void ExecEndSplit(SplitState *node);
extern void ExecReScanSplit(SplitState *node, ExprContext *exprCtxt);

#endif   /* PAR_NODESPLIT_H */
