/*
 * Routines to pack and unpack tuples for their
 * transmission between PargreSQL backends.
 *
 * Constantin S. Pan     2011     kvapen@gmail.com
 */

#include "postgres.h"
#include <stdio.h>

#include "access/par_tupack.h"
#include "libpq/pqformat.h"
#include "utils/lsyscache.h"

//void dump_stringinfo(StringInfoData sid) {
//	int i;
////	printf("sid: <");
//	for (i = 0; i < sid.len; i++) {
//		printf(" %02X", ((unsigned char*)sid.data)[i]);
//	}
////	printf(">\n");
//}

struct varlena * shove_bytes(unsigned char *stuff, int len) {
	struct varlena *result;

	result = (struct varlena *) palloc(VARHDRSZ + len);
	SET_VARSIZE(result, VARHDRSZ + len);
	memmove(VARDATA(result), stuff, len); 
	return(result);
}

//void printbytes(char *bytes, int len) {
//	int i;
//	for (i = 0; i < len; i++) {
//		print("%2X ", bytes[i]);
//	}
//	print("\n");
//}

/*
 * In the implementation of the next routines we assume the following:
 *
 * A) if a type is "byVal" then all the information is stored in the
 * Datum itself (i.e. no pointers involved!). In this case the
 * length of the type is always greater than zero and not more than
 * "sizeof(Datum)"
 *
 * B) if a type is not "byVal" and it has a fixed length (typlen > 0),
 * then the "Datum" always contains a pointer to a stream of bytes.
 * The number of significant bytes are always equal to the typlen.
 *
 * C) if a type is not "byVal" and has typlen == -1,
 * then the "Datum" always points to a "struct varlena".
 * This varlena structure has information about the actual length of this
 * particular instance of the type and about its value.
 *
 * D) if a type is not "byVal" and has typlen == -2,
 * then the "Datum" always points to a null-terminated C string.
 *
 * Note that we do not treat "toasted" datums specially; therefore what
 * will be copied or compared is the compressed data or toast reference.
 */

/*
 * Tuple packing routine
 */
StringInfoData par_tupack(TupleTableSlot *slot)
{
	TupleDesc typeinfo = slot->tts_tupleDescriptor;
	StringInfoData buf;
	int natts = typeinfo->natts;
	int i;

	/* Fill the type info arrays */
	int16 *typlen = (int *)palloc(natts * sizeof(int16));
	bool *typbyval = (bool *)palloc(natts * sizeof(bool));
	for (i = 0; i < natts; i++)
	{
		Oid typid = typeinfo->attrs[i]->atttypid;
		get_typlenbyval(typid, typlen + i, typbyval + i);
	}

	/* Make sure the tuple is fully deconstructed */
	slot_getallattrs(slot);

	initStringInfo(&buf);
	resetStringInfo(&buf);

	if (TupIsNull(slot))
	{
		pq_sendint(&buf, 0, 2);
	}
	else
	{
		pq_sendint(&buf, natts, 2);

		/*
		 * send the attributes of this tuple
		 */
		for (i = 0; i < natts; ++i)
		{
			Datum origattr = slot->tts_values[i];
			Datum attr;
			bytea *outputbytes;

			if (slot->tts_isnull[i])
			{
				pq_sendint(&buf, -1, 4);
				continue;
			}

			if (typbyval[i]) {
				// Datum is the actual value
				pq_sendint(&buf, 4, 4);
				pq_sendint(&buf, origattr, 4);
			} else {
				if (typlen[i] > 0) {
					// Datum is the pointer, typlen is the length
					pq_sendint(&buf, typlen[i], 4);
//					if (typlen[i] > 1000) {
//						printf("WTF1, i = %d, len = %d\n", i, typlen[i]);
//						continue;
//					}
					//printf("Send attr[%d]: Datum is pointer, typlen is the lenth\n", i);
					//printf("blkid ==== %u\n", ItemPointerGetBlockNumber((ItemPointer)DatumGetPointer(origattr)));
					//printbytes(DatumGetPointer(origattr), typlen[i]);
					pq_sendbytes(&buf, origattr, typlen[i]);
				} else if (typlen[i] == -1) {
					// Varlena!
					attr = PointerGetDatum(PG_DETOAST_DATUM(origattr));
					pq_sendint(&buf, VARSIZE_ANY(attr), 4);
//					if (VARSIZE_ANY(attr) > 1000) {
//						printf("WTF2\n");
//						continue;
//					}
					//printf("Send attr[%d]: Varlena!\n", i);
					pq_sendbytes(&buf, VARDATA_ANY(attr), VARSIZE_ANY(attr));
					/* Clean up detoasted copy, if any */
					if (DatumGetPointer(attr) != DatumGetPointer(origattr))
						pfree(DatumGetPointer(attr));
				} else if (typlen[i] == -2) {
					// C string
					int len = strlen(origattr);
					pq_sendint(&buf, len, 4);
//					if (len > 1000) {
//						printf("WTF3\n");
//						continue;
//					}
					pq_sendbytes(&buf, origattr, len);
				}
			}
		}
	}

	pfree(typlen);
	pfree(typbyval);

	//dump_stringinfo(buf);
	return buf;
}

/*
 * Tuple unpacking routine
 */
void par_tunpack(StringInfoData buf, TupleTableSlot *slot)
{
	int i;
	int natts;
	TupleDesc typeinfo;

	ExecClearTuple(slot);
	slot->tts_shouldFree = true;

	//dump_stringinfo(buf);
	buf.cursor = 0;

	typeinfo = slot->tts_tupleDescriptor;
	natts = typeinfo->natts;

	/* Fill the type info arrays */
	int16 *typlen = (int *)palloc(natts * sizeof(int16));
	bool *typbyval = (bool *)palloc(natts * sizeof(bool));
	for (i = 0; i < natts; i++)
	{
		Oid typid = typeinfo->attrs[i]->atttypid;
		get_typlenbyval(typid, typlen + i, typbyval + i);
	}

	slot->tts_tuple = NULL;

	if (pq_getmsgint(&buf, 2) == 0)
	{
		slot->tts_isempty = 1;
	}
	else
	{
		slot->tts_isempty = 0;
		/*
		 * recv the attributes of this tuple
		 */
		for (i = 0; i < natts; ++i)
		{
//			printf("unpacking attr%d\n", i);
			int len = pq_getmsgint(&buf, 4);
			if (len == -1)
			{
				slot->tts_isnull[i] = 1;
//				printf("tunpack: attr%d is null!\n", i);
				continue;
			}
			slot->tts_isnull[i] = 0;
			if (typbyval[i]) {
				// Datum should be the actual value
				//printf("Recv attr[%d]: Datum is the actual value\n", i);
				slot->tts_values[i] = pq_getmsgint(&buf, 4);
			} else {
				if (typlen[i] > 0) {
					// Datum should be the pointer, typlen should be the length
					slot->tts_values[i] = palloc(typlen[i]);
					//printf("Recv attr[%d]: Datum is pointer, typlen is the lenth = %d\n", i, typlen[i]);
					pq_copymsgbytes(&buf, slot->tts_values[i], typlen[i]);
					//slot->tts_values[i] = PointerGetDatum(pq_getmsgbytes(&buf, typlen[i]));
					//printbytes(slot->tts_values[i], typlen[i]);
				} else if (typlen[i] == -1) {
					// Varlena!
					//printf("Recv attr[%d]: Varlena!\n", i);
					slot->tts_values[i] = shove_bytes(pq_getmsgbytes(&buf, len), len);
				} else if (typlen[i] == -2) {
					// C string
					slot->tts_values[i] = palloc(len + 1);
					pq_copymsgbytes(&buf, slot->tts_values[i], len);
				}
			}
		}
		ExecMaterializeSlot(slot);
	}

	pfree(typlen);
	pfree(typbyval);
}
