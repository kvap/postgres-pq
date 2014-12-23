#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for executor
#
# IDENTIFICATION
#    $PostgreSQL: pgsql/src/backend/executor/Makefile,v 1.29 2008/12/28 18:53:55 tgl Exp $
#
#-------------------------------------------------------------------------

subdir = src/backend/executor
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

INIS_OBJS = $(top_builddir)/src/bin/par_inis/_pargresql_library.o \
       $(top_builddir)/src/bin/par_inis/_pargresql_memory_manager.o

OBJS = execAmi.o execCurrent.o execGrouping.o execMain.o \
       execProcnode.o execQual.o execScan.o execTuples.o \
       execUtils.o functions.o instrument.o nodeAppend.o nodeAgg.o \
       nodeBitmapAnd.o nodeBitmapOr.o \
       nodeBitmapHeapscan.o nodeBitmapIndexscan.o nodeHash.o \
       nodeHashjoin.o nodeIndexscan.o nodeMaterial.o nodeMergejoin.o \
       nodeNestloop.o nodeFunctionscan.o nodeRecursiveunion.o nodeResult.o \
       nodeSeqscan.o nodeSetOp.o nodeSort.o nodeUnique.o \
       nodeValuesscan.o nodeCtescan.o nodeWorktablescan.o \
       nodeLimit.o nodeGroup.o nodeSubplan.o nodeSubqueryscan.o nodeTidscan.o execJunk.o \
       par_nodeSplit.o par_nodeMerge.o par_nodeScatter.o par_nodeGather.o \
       nodeWindowAgg.o tstoreReceiver.o spi.o $(INIS_OBJS)

include $(top_srcdir)/src/backend/common.mk
