/*-------------------------------------------------------------------------
 * nodeGather.h
 *
 * Copyright (c) 2011, Constantin S. Pan
 *-------------------------------------------------------------------------
 */
#ifndef PAR_NODEGATHER_H
#define PAR_NODEGATHER_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsGather(Gather *node);
extern GatherState *ExecInitGather(Gather *node, EState *estate, int eflags);
extern TupleTableSlot *ExecGather(GatherState *node);
extern void ExecEndGather(GatherState *node);
extern void ExecReScanGather(GatherState *node, ExprContext *exprCtxt);

#endif   /* PAR_NODEGATHER_H */
