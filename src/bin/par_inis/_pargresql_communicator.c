/*
 * _pargresql_communicator.c
 *
 * by Alexey Koltakov
 */

#include <mpi.h>
#include <assert.h>
#include <stdio.h>
#include "par_inis/_pargresql_memory_manager.h"

static int node, nodescount;

/*
 * This function processes the shared memory blocks.
 */
void Start();

int main(int argc, char *argv[])
{
	int res;
	
	res = MPI_Init(&argc, &argv);
	assert(res == MPI_SUCCESS);

	res = MPI_Comm_rank(MPI_COMM_WORLD, &node);
	assert(res == MPI_SUCCESS);

	res = MPI_Comm_size(MPI_COMM_WORLD, &nodescount);
	assert(res == MPI_SUCCESS);

	CreateSHMObject(SHMEMNAME, node, nodescount);

	printf(" node %d/%d started!", node, nodescount);
	Start();
	RemoveSHMObject(SHMEMNAME);
	printf(" node %d/%d finished!", node, nodescount);

	MPI_Finalize();
	return 0;
}

void Start()
{
	shmblock_t *block;
	MPI_Request rqsts[BLOCKS_IN_SHMEM];
	MPI_Status status;
	int blockNumber, i, flag, res;
	int processing_count, unprocessed_count;

	while (1) {
		blockNumber = GetUnprocBlockNumber();
		block = GetBlock(blockNumber);

		if (block->msgType == TO_SEND) {
//			printf(" n%db%ds%d->n%d", node, blockNumber, block->msgSize, block->node);fflush(stdout);
			res = MPI_Isend(block->msg, block->msgSize, MPI_BYTE, block->node, block->port, MPI_COMM_WORLD, &rqsts[blockNumber]);
			assert(res == MPI_SUCCESS);
		} else if (block->msgType == TO_RECV) {
//			printf(" n%db%d,%d<-n%d", node, blockNumber, block->msgSize, block->node);fflush(stdout);
			res = MPI_Irecv(block->msg, block->msgSize, MPI_BYTE, block->node, block->port, MPI_COMM_WORLD, &rqsts[blockNumber]);
			assert(res == MPI_SUCCESS);
		} else if (block->msgType == TO_PROBE) {
			res = MPI_Iprobe(block->node, block->port, MPI_COMM_WORLD, &flag, &status);
			assert(res == MPI_SUCCESS);

			if (flag == 1) {
			 res = MPI_Get_count(&status, MPI_BYTE, &block->msgSize);
			 assert(res == MPI_SUCCESS);
			}

			res = sem_post(&block->state);
			assert(res == 0);
			continue;
		} else if (block->msgType == TO_CLOSE) {
			printf(" nnode %d TO_CLOSE", node);
			//unprocessed_count = UnprocBlocksCount();
			//assert(unprocessed_count == 0);
			//break;
			continue;
		} else
			assert(0);

		SetCurrentBlockNumber(blockNumber);
		processing_count = CurrentBlocksCount();

		do {
			for (i = 0; i < processing_count; i++) {
				blockNumber = GetCurrentBlockNumber();
				block = GetBlock(blockNumber);
				
				res = MPI_Test(&rqsts[blockNumber], &flag, &status);
				assert(res == MPI_SUCCESS);

				if (flag == 1) {
//					printf(" n%db%d", node, blockNumber);fflush(stdout);
					res = sem_post(&block->state);
					assert(res == 0);
				} else {
					//printf("\t#Узел %d сообщение блока %d еще не обработано\n", node, blockNumber);fflush(stdout);
					SetCurrentBlockNumber(blockNumber);
				}
			}

			processing_count = CurrentBlocksCount();
			unprocessed_count = UnprocBlocksCount();
		} while (unprocessed_count == 0 && processing_count > 0);
	}
}
