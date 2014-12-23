#ifdef PAR_CREATEPLAN_C
#error "par_createplan.c is already included elsewhere"
#else // PAR_CREATEPLAN_C is undefined
#define PAR_CREATEPLAN_C

Split *make_split(Plan *lefttree, Plan *righttree)
{
	Split	*node = makeNode(Split);
	Plan	*plan = &node->plan;

	copy_plan_costsize(plan, lefttree);
	// FIXME: don't we need to alter the cost estimations?

	plan->targetlist = lefttree->targetlist;
	plan->qual = NIL;
	plan->lefttree = lefttree;
	plan->righttree = righttree;

	return node;
}

Merge *make_merge(Plan *lefttree, Plan *righttree)
{
	Merge	*node = makeNode(Merge);

	if ((unsigned long)node > 0xf00000000000000) {
		printf("this should not happen, node == %lx\n", (unsigned long)node);
	}

	Plan	*plan = &node->plan;

	copy_plan_costsize(plan, righttree);
	// FIXME: don't we need to alter the cost estimations?

	plan->targetlist = righttree->targetlist;
	plan->qual = NIL;
	plan->lefttree = lefttree;
	plan->righttree = righttree;

	return node;
}

/*
 * sibling has to be the other child of the Split of the
 * same Exchange "metanode". It's used because Scatter
 * is a nullary operation.
 */
Scatter *make_scatter(Plan *sibling, int port, int fragattr)
{
	Scatter	*node = makeNode(Scatter);
	Plan	*plan = &node->plan;

	copy_plan_costsize(plan, sibling);
	// FIXME: don't we need to alter the cost estimations?

	plan->targetlist = sibling->targetlist;
	plan->qual = NIL;
	plan->lefttree = NULL;
	plan->righttree = NULL;
	node->port = port;
	plan->fragattr = fragattr;

	return node;
}

/*
 * sibling has to be the Split node of the same Exchange "metanode".
 * It's used because Gather is a nullary operation.
 */
Gather *make_gather(Plan *sibling, int port)
{
	Gather	*node = makeNode(Gather);
	Plan	*plan = &node->plan;

	copy_plan_costsize(plan, sibling);
	// FIXME: don't we need to alter the cost estimations?

	plan->targetlist = sibling->targetlist;
	plan->qual = NIL;
	plan->lefttree = NULL;
	plan->righttree = NULL;
	node->port = port;

	return node;
}

Plan *make_exchange(Plan *plan, int port, int fragattr)
{
	Plan *split, *merge, *scatter, *gather;
	scatter = (Plan*)make_scatter(plan, port, fragattr);
	split = (Plan*)make_split(plan, scatter);
	gather = (Plan*)make_gather(split, port);
	merge = (Plan*)make_merge(gather, split);
	if ((unsigned long)merge > 0xf00000000000000) {
		printf("a terrible failure, merge == %lx\n", (unsigned long)merge);
	} else {
		printf("OK, merge == %lx\n", (unsigned long)merge);
	}
	return merge;
}

#endif

