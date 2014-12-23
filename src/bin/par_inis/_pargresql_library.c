/*
 * _pargresql_library.c
 *
 * by Alexey Koltakov
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "par_inis/_pargresql_memory_manager.h"
#include "par_inis/_pargresql_library.h"

static int node;	/* current node id */
static int nodescount;	/* total number of nodes */


/*
 * This function opens the access to the shared memory.
 * No other functions of the library should be called
 * before this one.
 */
extern void _pargresql_InitLib()
{
	OpenSHMObject(SHMEMNAME, &node, &nodescount);
}

/*
 * This function puts a message of a given size into a free block
 * of the shared memory. Upon return it is guaranteed that the
 * message is in the shared memory, but not that the message was
 * actually sent.
 * 'dst' contains the id of the destination node.
 * 'port' allows to tell which exchange operation the message is
 * sent to.
 * 'request' represents the current message state.
 */
extern void _pargresql_ISend(int dst, uuid_t port, int size, void *buf, _pargresql_request_t *request)
{
	shmblock_t *block;
	int blockNumber;
	
	assert(size <= MAX_MESSAGE_SIZE);
	blockNumber = GetEmptyBlockNumber();
	block = GetBlock(blockNumber);

	block->node = dst;
	block->port = port;
	block->msgType = TO_SEND;
	block->msgSize = size;
	memcpy(block->msg, buf, size);

	SetUnprocBlockNumber(blockNumber);
	request->buf = NULL;
	request->blockNumber = blockNumber;
}

/*
 * This function returns checks the size of an incoming message.
 * If the message has arrived, its size is stored in 'size' and
 * 'flag' is set to 1. If there was no message, 'flag' is 0.
 * 'dst' contains the id of the destination node.
 * 'port' allows to tell which exchange operation the message is
 * sent to.
 */
extern void _pargresql_IProbe(int dst, uuid_t port, int *flag, int *size)
{
	shmblock_t *block;
	int blockNumber, res;

	blockNumber = GetEmptyBlockNumber();
	block = GetBlock(blockNumber);

	block->node = dst;
	block->port = port;
	block->msgType = TO_PROBE;
	block->msgSize = -1;

	SetUnprocBlockNumber(blockNumber);
	res = sem_wait(&block->state);
	assert(res == 0);

	if (block->msgSize > -1) {
		*flag = 1;
		*size = block->msgSize;
	} else {
		*flag = 0;
	}

	block->msgSize = 0;
	SetEmptyBlockNumber(blockNumber);
}

/*
 * This function puts a request for a message of a given size and buffer
 * into a free block of shared memory. Upon return it is guaranteed that
 * the request was stored in the shared memory, but not that any
 * message was actually received and stored in buf.
 * 'src' contains the id of the source node.
 * 'port' allows to tell which exchange operation the message is
 * sent to.
 * 'request' allows to see the current state of the request.
 */
extern void _pargresql_IRecv(int src, uuid_t port, int size, void *buf, _pargresql_request_t *request)
{
	shmblock_t *block;
	int blockNumber;

	assert(size <= MAX_MESSAGE_SIZE);	
	blockNumber = GetEmptyBlockNumber();
	block = GetBlock(blockNumber);
	
	block->node = src;
	block->port = port;
	block->msgType = TO_RECV;
	block->msgSize = size;

	SetUnprocBlockNumber(blockNumber);
	request->buf = buf;
	request->blockNumber = blockNumber;
}

/*
 * This function checks the message state by its request handle.
 * If the message has been processed, 'flag' is set to 1,
 * if not---it is set to 0.
 */
extern void _pargresql_Test(_pargresql_request_t *request, int *flag)
{
	shmblock_t *block;
	int state, res;

	assert(request->blockNumber >= 0 && request->blockNumber < BLOCKS_IN_SHMEM);
	block = GetBlock(request->blockNumber);
	res = sem_getvalue(&block->state, &state);
	assert(res == 0);
	
	if (state == PROCESSED) {
		if (block->msgType == TO_RECV) {
			assert(request->buf != NULL);
			memcpy(request->buf, block->msg, block->msgSize);
		} else
			assert(block->msgType == TO_SEND);

		memset(block->msg, 0, MAX_MESSAGE_SIZE);
		block->msgSize = 0;
		res = sem_wait(&block->state);
		assert(res == 0);
		SetEmptyBlockNumber(request->blockNumber);
		request->buf = NULL;
		request->blockNumber = -1;
		*flag = 1;
	} else {
		*flag = 0;
	}
}

/*
 * This function returns the current node id.
 */
extern int _pargresql_GetNode(void)
{
	return node;
}

/*
 * This function returns the total number of nodes.
 */
extern int _pargresql_GetNodesCount(void)
{
	return nodescount;
}

/*
 * This function finishes the work with the shared memory.
 * All the following calls to any library functions are forbidden.
 * Upon the call to this function by a process all the message
 * interactions of this process should be finished.
 */
extern void _pargresql_FinalizeLib(void)
{
	shmblock_t *block;
	int blockNumber;

	blockNumber = GetEmptyBlockNumber();
	block = GetBlock(blockNumber);
	block->msgType = TO_CLOSE;
	SetUnprocBlockNumber(blockNumber);
}
