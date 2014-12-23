/*-------------------------------------------------------------------------
 * nodeScatter.h
 *
 * Copyright (c) 2011, Constantin S. Pan
 *-------------------------------------------------------------------------
 */
#ifndef PAR_NODESCATTER_H
#define PAR_NODESCATTER_H

#include "nodes/execnodes.h"

extern int fragfunc(int size, TupleTableSlot *slot, int fragattr);

extern int	ExecCountSlotsScatter(Scatter *node);
extern ScatterState *ExecInitScatter(Scatter *node, EState *estate, int eflags);
extern TupleTableSlot *ExecScatter(ScatterState *node);
extern void ExecEndScatter(ScatterState *node);
extern void ExecReScanScatter(ScatterState *node, ExprContext *exprCtxt);

#endif   /* PAR_NODESCATTER_H */
