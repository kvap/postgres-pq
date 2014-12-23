/*
 * _pargresql_memory_manager.c
 *
 * by Alexey Koltakov
 */

#include <assert.h>
#include <stdio.h>
#include "par_inis/_pargresql_memory_manager.h"

typedef struct stack {
	int top;
	int data[BLOCKS_IN_SHMEM];
} stack_t;

typedef struct queue {
	int head;
	int tail;
	int data[BLOCKS_IN_SHMEM];
} queue_t;

typedef struct {
	sem_t semaphore;
	int node;
	int nodescount;
	
	sem_t nempty;
	sem_t emptySem;
	stack_t emptyblocks;
	
	sem_t nunproc;
	sem_t unprocSem;
	queue_t unprocblocks;

	shmblock_t blocks[BLOCKS_IN_SHMEM];
} shmem_t;

#define init_stack(s)											\
	for((s).top = BLOCKS_IN_SHMEM-1; (s).top >= 0; (s).top--)	\
		((s).data[(s).top]) = -1

#define push(s, e)												\
	(s).top++;													\
	((s).data[(s).top]) = (e)

#define pop(s, e)												\
	(e) = ((s).data[(s).top]);									\
	((s).data[(s).top]) = -1;									\
	(s).top--

#define init_queue(q)											\
	for((q).head = 0; (q).head < BLOCKS_IN_SHMEM; (q).head++)	\
		((q).data[(q).head]) = -1;								\
	(q).head = 0;												\
	(q).tail = 0

#define enqueue(q, e)											\
	((q).data[(q).tail]) = (e);									\
	((q).tail) = ((q).tail + 1) % BLOCKS_IN_SHMEM

#define dequeue(q, e)											\
	(e) = (q).data[(q).head];									\
	(q).data[(q).head] = -1;									\
	(q).head = ((q).head + 1) % BLOCKS_IN_SHMEM

#define queue_count(q)											\
	( (q).data[(q).tail] == -1 ) ?								\
		(( (q).head <= (q).tail ) ?								\
				( (q).tail - (q).head )							\
				: ( (q).tail - (q).head + BLOCKS_IN_SHMEM ))	\
		: BLOCKS_IN_SHMEM

static shmem_t *memptr; 		/* указатель на разделяемую память */
static queue_t curblocks;


/*
 * This function creates and opens a new shared memory object.
 * 'name' contains the name of the shared memory object.
 * 'node' contains the current node id.
 * 'nodescount' contains the total number of nodes.
 */
extern void CreateSHMObject(const char *shmName, int node, int nodescount)
{
	int fd, res, i;

	shm_unlink(shmName);
	fd = shm_open(shmName, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	assert(fd > -1);
	res = ftruncate(fd, sizeof(shmem_t));
	assert(res == 0);
	memptr = mmap(NULL, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	res = sem_init(&memptr->semaphore, 1, 0);
	assert(res == 0);
	res = sem_init(&memptr->nunproc, 1, 0);
	assert(res == 0);
	res = sem_init(&memptr->unprocSem, 1, 1);
	assert(res == 0);
	res = sem_init(&memptr->nempty, 1, BLOCKS_IN_SHMEM);
	assert(res == 0);
	res = sem_init(&memptr->emptySem, 1, 1);
	assert(res == 0);

	memptr->node = node;
	memptr->nodescount = nodescount;
	init_stack(memptr->emptyblocks);
	init_queue(memptr->unprocblocks);

	for (i = 0; i < BLOCKS_IN_SHMEM; i++) {
		res = sem_init(&memptr->blocks[i].state, 1 , UNPROCESSED);
		assert(res == 0);
		push(memptr->emptyblocks, i);
	}
	
	res = sem_post(&memptr->semaphore);
	assert(res == 0);
	init_queue(curblocks);
}

/*
 * This function opens an existing shared memory object.
 * 'name' contains the shared memory object name.
 * 'node' will contain the current node id upon return.
 * 'nodescount' will contin the total number of nodes upon return.
 */
extern void OpenSHMObject(const char *shmName, int *node, int *nodescount)
{
	int fd, res;

	fd = shm_open(shmName, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	assert(fd > -1);
	memptr = mmap(NULL, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	res = sem_wait(&memptr->semaphore);
	assert(res == 0);
	*node = memptr->node;
	*nodescount = memptr->nodescount;
	res = sem_post(&memptr->semaphore);
	assert(res == 0);
	init_queue(curblocks);
}

/*
 * This function deletes an existing shared memory object.
 * 'name' contains the name of the object.
 */
extern void RemoveSHMObject(const char *name)
{
	int res;

	res = shm_unlink(name);
	assert(res == 0);
}

/*
 * This function returns a pointer to a shared memory block by its id.
 */
extern shmblock_t *GetBlock(int blockNumber)
{
	shmblock_t *block;

	assert(blockNumber >= 0 && blockNumber < BLOCKS_IN_SHMEM);
	block = &memptr->blocks[blockNumber];

	return block;
}

/*
 * This function returns the id of a free block of shared memory.
 */
extern int GetEmptyBlockNumber()
{
	int blockNumber, res;
	
	res = sem_wait(&memptr->nempty);
	assert(res == 0);
	res = sem_wait(&memptr->emptySem);
	assert(res == 0);
	
	pop(memptr->emptyblocks, blockNumber);
	assert(blockNumber >= 0 && blockNumber < BLOCKS_IN_SHMEM);
	
	res = sem_post(&memptr->emptySem);
	assert(res == 0);
	
	return blockNumber;
}

/*
 * This function marks a shared memory block with given id as free.
 */
extern void SetEmptyBlockNumber(int blockNumber)
{
	int i, res;
	
	assert(blockNumber >= 0 && blockNumber < BLOCKS_IN_SHMEM);
	res = sem_wait(&memptr->emptySem);
	assert(res == 0);
	
	push(memptr->emptyblocks, blockNumber);
	
	res = sem_post(&memptr->emptySem);
	assert(res == 0);
	res = sem_post(&memptr->nempty);
	assert(res == 0);
}

/*
 * This function returns the number of unprocessed shared memory blocks.
 */
extern int UnprocBlocksCount()
{
	int count, res;

	res = sem_getvalue(&memptr->nunproc, &count);
	assert(res == 0);

	return count;
}

/*
 * This function returns the id of an unprocessed shared memory block.
 */
extern int GetUnprocBlockNumber()
{
	int blockNumber, res;
	
	res = sem_wait(&memptr->nunproc);
	assert(res == 0);
	res = sem_wait(&memptr->unprocSem);
	assert(res == 0);
	
	dequeue(memptr->unprocblocks, blockNumber);
	assert(blockNumber >= 0 && blockNumber < BLOCKS_IN_SHMEM);
	
	res = sem_post(&memptr->unprocSem);
	assert(res == 0);

	return blockNumber;
}

/*
 * This function marks a shared memory block with a given id as unprocessed.
 */
extern void SetUnprocBlockNumber(int blockNumber)
{
	int res;
	
	assert(blockNumber >= 0 && blockNumber < BLOCKS_IN_SHMEM);
	res = sem_wait(&memptr->unprocSem);
	assert(res == 0);
	
	enqueue(memptr->unprocblocks, blockNumber);
	
	res = sem_post(&memptr->unprocSem);
	assert(res == 0);
	res = sem_post(&memptr->nunproc);
	assert(res == 0);
}

/*
 * This function returns the total number of current blocks of the shared memory.
 */
extern int CurrentBlocksCount()
{
	return queue_count(curblocks);
}

/*
 * This function returns the id of the current block of the shared memory.
 */
extern int GetCurrentBlockNumber()
{
	int blockNumber;

	dequeue(curblocks, blockNumber);
	assert(blockNumber >= 0 && blockNumber < BLOCKS_IN_SHMEM);

	return blockNumber;
}

/*
 * This function marks a shared memory block with a given id as current.
 */
extern void SetCurrentBlockNumber(int blockNumber)
{
	assert(blockNumber >= 0 && blockNumber < BLOCKS_IN_SHMEM);
	enqueue(curblocks, blockNumber);
}
