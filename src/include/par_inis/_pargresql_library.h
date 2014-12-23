/*
 * _pargresql_library.h
 *
 * Создал А.В. Колтаков
 */

#ifndef _PARGRESQL_LIBRARY_H_
#define _PARGRESQL_LIBRARY_H_

#include "_pargresql_memory_manager.h"

typedef struct {
	int blockNumber;
	void *buf;
} _pargresql_request_t;

/*
 * This function opens the access to the shared memory.
 * No other functions of the library should be called
 * before this one.
 */
extern void _pargresql_InitLib();

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
extern void _pargresql_ISend(int dst, uuid_t port, int size, void *buf, _pargresql_request_t *request);

/*
 * This function returns checks the size of an incoming message.
 * If the message has arrived, its size is stored in 'size' and
 * 'flag' is set to 1. If there was no message, 'flag' is 0.
 * 'dst' contains the id of the destination node.
 * 'port' allows to tell which exchange operation the message is
 * sent to.
 */
extern void _pargresql_IProbe(int dst, uuid_t port, int *flag, int *size);

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
extern void _pargresql_IRecv(int src, uuid_t port, int size, void *buf, _pargresql_request_t *request);

/*
 * This function checks the message state by its request handle.
 * If the message has been processed, 'flag' is set to 1,
 * if not---it is set to 0.
 */
extern void _pargresql_Test(_pargresql_request_t *request, int *flag);


/*
 * This function returns the current node id.
 */
extern int _pargresql_GetNode(void);

/*
 * This function returns the total number of nodes.
 */
extern int _pargresql_GetNodesCount(void);

/*
 * This function finishes the work with the shared memory.
 * All the following calls to any library functions are forbidden.
 * Upon the call to this function by a process all the message
 * interactions of this process should be finished.
 */
extern void _pargresql_FinalizeLib(void);

#endif /* _PARGRESQL_LIBRARY_H_ */
