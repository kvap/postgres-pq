/*-------------------------------------------------------------------------
 * nodeMerge.h
 *
 * Copyright (c) 2011, Constantin S. Pan
 *-------------------------------------------------------------------------
 */
#ifndef PAR_NODEMERGE_H
#define PAR_NODEMERGE_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsMerge(Merge *node);
extern MergeState *ExecInitMerge(Merge *node, EState *estate, int eflags);
extern TupleTableSlot *ExecMerge(MergeState *node);
extern void ExecEndMerge(MergeState *node);
extern void ExecReScanMerge(MergeState *node, ExprContext *exprCtxt);

#endif   /* PAR_NODEMERGE_H */
