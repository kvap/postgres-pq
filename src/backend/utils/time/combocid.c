/*-------------------------------------------------------------------------
 *
 * combocid.c
 *	  Combo command ID support routines
 *
 * Before version 8.3, HeapTupleHeaderData had separate fields for cmin
 * and cmax.  To reduce the header size, cmin and cmax are now overlayed
 * in the same field in the header.  That usually works because you rarely
 * insert and delete a tuple in the same transaction, and we don't need
 * either field to remain valid after the originating transaction exits.
 * To make it work when the inserting transaction does delete the tuple,
 * we create a "combo" command ID and store that in the tuple header
 * instead of cmin and cmax. The combo command ID can be mapped to the
 * real cmin and cmax using a backend-private array, which is managed by
 * this module.
 *
 * To allow reusing existing combo cids, we also keep a hash table that
 * maps cmin,cmax pairs to combo cids.	This keeps the data structure size
 * reasonable in most cases, since the number of unique pairs used by any
 * one transaction is likely to be small.
 *
 * With a 32-bit combo command id we can represent 2^32 distinct cmin,cmax
 * combinations. In the most perverse case where each command deletes a tuple
 * generated by every previous command, the number of combo command ids
 * required for N commands is N*(N+1)/2.  That means that in the worst case,
 * that's enough for 92682 commands.  In practice, you'll run out of memory
 * and/or disk space way before you reach that limit.
 *
 * The array and hash table are kept in TopTransactionContext, and are
 * destroyed at the end of each transaction.
 *
 *
 * Portions Copyright (c) 1996-2007, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/utils/time/combocid.c,v 1.2 2007/11/15 21:14:41 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup.h"
#include "access/xact.h"
#include "utils/combocid.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"


/* Hash table to lookup combo cids by cmin and cmax */
static HTAB *comboHash = NULL;

/* Key and entry structures for the hash table */
typedef struct
{
	CommandId	cmin;
	CommandId	cmax;
}	ComboCidKeyData;

typedef ComboCidKeyData *ComboCidKey;

typedef struct
{
	ComboCidKeyData key;
	CommandId	combocid;
}	ComboCidEntryData;

typedef ComboCidEntryData *ComboCidEntry;

/* Initial size of the hash table */
#define CCID_HASH_SIZE			100


/*
 * An array of cmin,cmax pairs, indexed by combo command id.
 * To convert a combo cid to cmin and cmax, you do a simple array lookup.
 */
static ComboCidKey comboCids = NULL;
static int	usedComboCids = 0;	/* number of elements in comboCids */
static int	sizeComboCids = 0;	/* allocated size of array */

/* Initial size of the array */
#define CCID_ARRAY_SIZE			100


/* prototypes for internal functions */
static CommandId GetComboCommandId(CommandId cmin, CommandId cmax);
static CommandId GetRealCmin(CommandId combocid);
static CommandId GetRealCmax(CommandId combocid);


/**** External API ****/

/*
 * GetCmin and GetCmax assert that they are only called in situations where
 * they make sense, that is, can deliver a useful answer.  If you have
 * reason to examine a tuple's t_cid field from a transaction other than
 * the originating one, use HeapTupleHeaderGetRawCommandId() directly.
 */

CommandId
HeapTupleHeaderGetCmin(HeapTupleHeader tup)
{
	CommandId	cid = HeapTupleHeaderGetRawCommandId(tup);

	Assert(!(tup->t_infomask & HEAP_MOVED));
	Assert(TransactionIdIsCurrentTransactionId(HeapTupleHeaderGetXmin(tup)));

	if (tup->t_infomask & HEAP_COMBOCID)
		return GetRealCmin(cid);
	else
		return cid;
}

CommandId
HeapTupleHeaderGetCmax(HeapTupleHeader tup)
{
	CommandId	cid = HeapTupleHeaderGetRawCommandId(tup);

	/* We do not store cmax when locking a tuple */
	Assert(!(tup->t_infomask & (HEAP_MOVED | HEAP_IS_LOCKED)));
	Assert(TransactionIdIsCurrentTransactionId(HeapTupleHeaderGetXmax(tup)));

	if (tup->t_infomask & HEAP_COMBOCID)
		return GetRealCmax(cid);
	else
		return cid;
}

/*
 * Given a tuple we are about to delete, determine the correct value to store
 * into its t_cid field.
 *
 * If we don't need a combo CID, *cmax is unchanged and *iscombo is set to
 * FALSE.  If we do need one, *cmax is replaced by a combo CID and *iscombo
 * is set to TRUE.
 *
 * The reason this is separate from the actual HeapTupleHeaderSetCmax()
 * operation is that this could fail due to out-of-memory conditions.  Hence
 * we need to do this before entering the critical section that actually
 * changes the tuple in shared buffers.
 */
void
HeapTupleHeaderAdjustCmax(HeapTupleHeader tup,
						  CommandId *cmax,
						  bool *iscombo)
{
	/*
	 * If we're marking a tuple deleted that was inserted by (any
	 * subtransaction of) our transaction, we need to use a combo command id.
	 * Test for HEAP_XMIN_COMMITTED first, because it's cheaper than a
	 * TransactionIdIsCurrentTransactionId call.
	 */
	if (!(tup->t_infomask & HEAP_XMIN_COMMITTED) &&
		TransactionIdIsCurrentTransactionId(HeapTupleHeaderGetXmin(tup)))
	{
		CommandId	cmin = HeapTupleHeaderGetRawCommandId(tup);

		*cmax = GetComboCommandId(cmin, *cmax);
		*iscombo = true;
	}
	else
	{
		*iscombo = false;
	}
}

/*
 * Combo command ids are only interesting to the inserting and deleting
 * transaction, so we can forget about them at the end of transaction.
 */
void
AtEOXact_ComboCid(void)
{
	/*
	 * Don't bother to pfree. These are allocated in TopTransactionContext, so
	 * they're going to go away at the end of transaction anyway.
	 */
	comboHash = NULL;

	comboCids = NULL;
	usedComboCids = 0;
	sizeComboCids = 0;
}


/**** Internal routines ****/

/*
 * Get a combo command id that maps to cmin and cmax.
 *
 * We try to reuse old combo command ids when possible.
 */
static CommandId
GetComboCommandId(CommandId cmin, CommandId cmax)
{
	CommandId	combocid;
	ComboCidKeyData key;
	ComboCidEntry entry;
	bool		found;

	/*
	 * Create the hash table and array the first time we need to use combo
	 * cids in the transaction.
	 */
	if (comboHash == NULL)
	{
		HASHCTL		hash_ctl;

		memset(&hash_ctl, 0, sizeof(hash_ctl));
		hash_ctl.keysize = sizeof(ComboCidKeyData);
		hash_ctl.entrysize = sizeof(ComboCidEntryData);
		hash_ctl.hash = tag_hash;
		hash_ctl.hcxt = TopTransactionContext;

		comboHash = hash_create("Combo CIDs",
								CCID_HASH_SIZE,
								&hash_ctl,
								HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);

		comboCids = (ComboCidKeyData *)
			MemoryContextAlloc(TopTransactionContext,
							   sizeof(ComboCidKeyData) * CCID_ARRAY_SIZE);
		sizeComboCids = CCID_ARRAY_SIZE;
		usedComboCids = 0;
	}

	/* Lookup or create a hash entry with the desired cmin/cmax */

	/* We assume there is no struct padding in ComboCidKeyData! */
	key.cmin = cmin;
	key.cmax = cmax;
	entry = (ComboCidEntry) hash_search(comboHash,
										(void *) &key,
										HASH_ENTER,
										&found);

	if (found)
	{
		/* Reuse an existing combo cid */
		return entry->combocid;
	}

	/*
	 * We have to create a new combo cid. Check that there's room for it in
	 * the array, and grow it if there isn't.
	 */
	if (usedComboCids >= sizeComboCids)
	{
		/* We need to grow the array */
		int			newsize = sizeComboCids * 2;

		comboCids = (ComboCidKeyData *)
			repalloc(comboCids, sizeof(ComboCidKeyData) * newsize);
		sizeComboCids = newsize;
	}

	combocid = usedComboCids;

	comboCids[combocid].cmin = cmin;
	comboCids[combocid].cmax = cmax;
	usedComboCids++;

	entry->combocid = combocid;

	return combocid;
}

static CommandId
GetRealCmin(CommandId combocid)
{
	Assert(combocid < usedComboCids);
	return comboCids[combocid].cmin;
}

static CommandId
GetRealCmax(CommandId combocid)
{
	Assert(combocid < usedComboCids);
	return comboCids[combocid].cmax;
}
