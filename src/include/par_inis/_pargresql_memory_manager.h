/*
 * _pargresql_memory_manager.h
 *
 * by Alexey Koltakov
 */

#ifndef _PARGRESQL_MEMORY_MANAGER_H_
#define _PARGRESQL_MEMORY_MANAGER_H_

#include <fcntl.h>		/* for nonblocking */
#include <semaphore.h>	/* Posix semaphores */
#include <string.h>
#include <sys/mman.h>	/* Posix shared memory */
#include <sys/stat.h>	/* for S_xxx file mode constants */
#include <unistd.h>

#define SHMEMNAME			"/mem0"
#define BLOCKS_IN_SHMEM		2000
#define MAX_MESSAGE_SIZE	16777
#define UNPROCESSED			0
#define PROCESSED			1

typedef int uuid_t;
typedef int node_t;

typedef enum {
	TO_SEND = 0,
	TO_RECV,
	TO_PROBE,
	TO_CLOSE
} messagetype_t;

typedef char message_t[MAX_MESSAGE_SIZE];

typedef struct {
	sem_t state;
	uuid_t port;
	node_t node;
	messagetype_t msgType;
	message_t msg;
	int msgSize;
} shmblock_t;

/*
 * This function creates and opens a new shared memory object.
 * 'name' contains the name of the shared memory object.
 * 'node' contains the current node id.
 * 'nodescount' contains the total number of nodes.
 */
extern void CreateSHMObject(const char *name, int node, int nodescount);

/*
 * This function opens an existing shared memory object.
 * 'name' contains the shared memory object name.
 * 'node' will contain the current node id upon return.
 * 'nodescount' will contin the total number of nodes upon return.
 */
extern void OpenSHMObject(const char *name, int *node, int *nodescount);

/*
 * This function deletes an existing shared memory object.
 * 'name' contains the name of the object.
 */
extern void RemoveSHMObject(const char *name);

/*
 * This function returns a pointer to a shared memory block by its id.
 */
extern shmblock_t *GetBlock(int blockNumber);

/*
 * This function returns the id of a free block of shared memory.
 */
extern int GetEmptyBlockNumber();

/*
 * This function marks a shared memory block with given id as free.
 */
extern void SetEmptyBlockNumber(int blockNumber);

/*
 * This function returns the number of unprocessed shared memory blocks.
 */
extern int UnprocBlocksCount();

/*
 * This function returns the id of an unprocessed shared memory block.
 */
extern int GetUnprocBlockNumber();

/*
 * This function marks a shared memory block with a given id as unprocessed.
 */
extern void SetUnprocBlockNumber(int blockNumber);

/*
 * This function returns the total number of current blocks of the shared memory.
 */
extern int CurrentBlocksCount();

/*
 * This function returns the id of the current block of the shared memory.
 */
extern int GetCurrentBlockNumber();

/*
 * This function marks a shared memory block with a given id as current.
 */
extern void SetCurrentBlockNumber(int blockNumber);

#endif /* _PARGRESQL_MEMORY_MANAGER_H_ */
