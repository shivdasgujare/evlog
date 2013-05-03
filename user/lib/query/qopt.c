/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2001
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#include "posix_evlog.h"
#include "evl_parse.h"
#include "q.tab.h"

/*
 * This file implements query optimization on a normalized query tree.
 *
 * So far, we do the following optimizations:
 *
 * 1) Rewrite a query such as 'facility=100 && severity>info && foo=46'
 * into a subquery that can be evaluated without referencing templates
 * (i.e., 'facility=100 && severity>info') and a subquery that may need
 * the template.  Evaluate the former first, then the latter only if
 * necessary.
 *
 * 2) Annotate the query with info that allows us to short-circuit the query
 * if non-standard attributes mentioned in the query are missing in the
 * record we're evaluating.  This allows us to know, without populating
 * the template (or even loading it, in many cases), that the query 'foo=46'
 * will fail for any record whose whose template is lacking, or whose
 * template doesn't mention foo.
 *
 * TODO: Read up on query optimization.
 */

static void divideTree(pnode_t *root);

/*
 * If possible, reorder the comparisons in the indicated tree so that the
 * cheap comparisons are made first, in hopes that the expensive ones can
 * be avoided in many cases.
 * 
 * For now, we just do the following:
 * Rewrite a query such as 'facility=100 && severity>info && foo=46'
 * into a subquery that can be evaluated without referencing templates
 * (i.e., 'facility=100 && severity>info') and a subquery that may need
 * the template.  Evaluate the former first, then the latter only if
 * necessary.
 *
 * For the above example, we rewrite the query as
 *	(facility=100 && severity>info) AND_LATER (foo=46)
 * The idea is to move as many "pure" comparisons (those that refer only
 * to standard attributes) to the left of the AND_LATER (or OR_LATER) node.
 * When the query is evaluated, the subtree to the left of the root
 * (AND_LATER or OR_LATER) node is evaluated first.  If that's not
 * sufficient to resolve the truth of the query expression, then we
 * bring in the template and evaluate the right subtree.
 *
 * TODO: Reorder subtrees based on the costs of different kinds of
 * comparisons:
 *	integer (cheapest)
 *	CONTAINS (more expensive)
 *	regular expression (even more expensive)
 *	age comparison (calls time() system call, so also expensive)
 */
void
_evlQReorderTree(pnode_t *root, evl_nonStdAtts_t *nsAtts)
{
	if (!nsAtts) {
		/*
		 * For now, we're interested only if we have non-standard
		 * attributes.
		 */
		return;
	}

	divideTree(root);
}

/*** Optimization #1 ***/

/* Information used by divideTree() and its helpers */
typedef struct _div_info {
	int		diRootOp;	/* AND or OR */
	evl_list_t	*diPureList;	/* nodes that don't ref non-std atts */
	evl_list_t	*diImpureList;	/* nodes that ref non-std atts */
	evl_list_t	*diRecycleList;	/* AND/OR nodes to reuse when */
					/* rebuilding tree */
} div_info_t;

/*
 * Returns 1 (true) if the subtree rooted at node refers to any non-standard
 * attributes, or 0 otherwise.
 */
static int
refsNonStdAtts(pnode_t *node)
{
	if (!node) {
		return 0;
	}
	switch (node->node_type) {
	case nt_op:
		switch (node->attr_flag) {
		case '!':
			return refsNonStdAtts(pnLeft(node));
		case AND:
		case OR:
			return (refsNonStdAtts(pnLeft(node))
				|| refsNonStdAtts(pnRight(node)));
		default:
			/*
			 * Note that the expression
			 *	data contains "foo"
			 * tests the contents of the variable portion
			 * of the (STRING) record, NOT the result after
			 * the template (if any) is applied.
			 */
			return (pnLeft(node)->node_type == nt_nsaname);
		}
	case nt_nsaname:
		return 1;
	case nt_attname:
		return 0;
	default:
		assert(0);
		/* NOTREACHED */
		return 0;
	}
	/* NOTREACHED */
}

/* Helper function for divideTree(), which see. */
static void
diClassify(pnode_t *node, div_info_t *di)
{
	if (node->node_type == nt_op && node->attr_flag == di->diRootOp) {
		di->diRecycleList = _evlAppendToList(di->diRecycleList, node);
		diClassify(pnLeft(node), di);
		diClassify(pnRight(node), di);
	} else {
		if (refsNonStdAtts(node)) {
			di->diImpureList = _evlAppendToList(di->diImpureList, node);
		} else {
			di->diPureList = _evlAppendToList(di->diPureList, node);
		}
	}
}

/*
 * Remove the first node on the list pointed to by plist, and return a pointer
 * to that node's data.  Update *plist to point to the new (possibly NULL)
 * list head.
 */
static pnode_t *
behead(evl_list_t **plist)
{
	pnode_t *pnode;
	assert(plist != NULL && *plist != NULL);
	pnode = (pnode_t*) (*plist)->li_data;
	*plist = _evlRemoveNode(*plist, *plist, NULL);
	return pnode;
}

/*
 * plist points to either di->diPureList or di->diImpureList.  Make a subtree
 * out of the remaining nodes in the list, depopulating the list as we go.
 * For example, given the list { n1, n2, n3, n4 } and di->diRootOp == OR,
 * we create (n1 OR (n2 OR (n3 OR n4))).  Return a pointer to the subtree's
 * root.
 */
static pnode_t *
diRebuildSubtree(evl_list_t **plist, div_info_t *di)
{
	pnode_t *node = behead(plist);
	if (*plist == NULL) {
		/* node is the last in the list. */
		return node;
	} else {
		/* Return (node AND/OR restOfList). */
		pnode_t *parent = behead(&di->diRecycleList);
		pnLeft(parent) = node;
		pnRight(parent) = diRebuildSubtree(plist, di);
		return parent;
	}
	/* NOTREACHED */
}

/*
 * Given the query
 * 	expr1 && nexpr1 && expr2 && nexpr2 && nexpr3
 * where
 * 	expr1 and expr2 reference only standard attributes
 * and
 * 	nexpr1, nexpr2, and nexpr3 reference non-standard attributes,
 * rewrite the query as
 * 	(expr1 && expr2) AND_LATER (nexpr1 && (nexpr2 && nexpr3))
 * I.e., convert the root (AND) node to an AND_LATER node, and move all the refs
 * to non-standard attributes to the right of the AND_LATER node.
 * 
 * Works similarly if the root node is OR -- we convert it to OR_LATER, etc.
 * 
 * If the query cannot be transformed in this way (e.g., all subexpressions
 * refer to non-standard attributes), do nothing.
 * 
 * Asserts if the query doesn't reference at least one non-standard attribute.
 */
static void
divideTree(pnode_t *root)
{
	int rootOp;
	div_info_t *di;

	if (root->node_type != nt_op) {
		return;
	}
	rootOp = root->attr_flag;
	if (rootOp != AND && rootOp != OR) {
		return;
	}

	di = (div_info_t*) malloc(sizeof(div_info_t));
	assert(di != NULL);
	di->diRootOp = rootOp;
	di->diPureList = NULL;
	di->diImpureList = NULL;
	di->diRecycleList = NULL;

	/*
	 * Classify the subtrees.  Given the aforementioned sample query,
	 * we add expr1 and expr2 to the pure list; nexpr1, nexpr2, and nexpr3
	 * to the impure list, and all the indicated AND nodes (except root)
	 * to the recycle list.
	 */
	diClassify(pnLeft(root), di);
	diClassify(pnRight(root), di);

	assert(di->diImpureList != NULL);
	if (di->diPureList == NULL) {
		/* Can't be reordered. */
		goto done;
	}

	/*
	 * Rebuild the left subtree from the pure nodes and the right
	 * subtree from the impure nodes.
	 */
	pnLeft(root) = diRebuildSubtree(&di->diPureList, di);
	pnRight(root) = diRebuildSubtree(&di->diImpureList, di);
	assert(di->diPureList == NULL);
	assert(di->diImpureList == NULL);
	assert(di->diRecycleList == NULL);

	root->attr_flag = ((rootOp == AND) ? AND_LATER : OR_LATER);
	free(di);
	return;

done:
	/* _evlFreeList() handles null lists. */
	_evlFreeList(di->diPureList, 0);
	_evlFreeList(di->diImpureList, 0);
	_evlFreeList(di->diRecycleList, 0);
	free(di);
}

/*** Optimization #2 ***/

/*
 * What would the query expression rooted at root evaluate to if the
 * attribute specified by nsa did not have a value?  Return evlFuzzyTrue
 * if it would always evaluate to true, evlFuzzyFalse if it would always
 * evaluate to false, or evlFuzzyMaybe if we can't say for sure.
 *
 * If nsa==NULL, simulate the absence of all non-standard attributes.
 *
 * Assumes the query tree is already normalized.
 */
int
simulateMissingNsa(pnode_t *root, evl_nonStdAtt_t *nsa)
{
	if (!root) {
		return evlFuzzyTrue;
	}

	switch (root->node_type) {
	case nt_attname:
		/*
		 * Name of a standard attribute.  All but data are always
		 * present.
		 */
		if (root->attr_flag == POSIX_LOG_ENTRY_DATA) {
			return evlFuzzyMaybe;
		} else {
			return evlFuzzyTrue;
		}
	case nt_nsaname:
		/* Name of a non-standard attribute */
		if (!nsa || root->attr_nsa == nsa) {
			/* By definition, this attribute is missing. */
			return evlFuzzyFalse;
		} else {
			return evlFuzzyMaybe;
		}
	case nt_op:
		switch (root->attr_flag) {
		case '!':
			switch (simulateMissingNsa(pnLeft(root), nsa)) {
			case evlFuzzyFalse:	return evlFuzzyTrue;
			case evlFuzzyTrue:	return evlFuzzyFalse;
			default:	return evlFuzzyMaybe;
			}
		case AND:
		case AND_LATER:
		    {
		    	int left = simulateMissingNsa(pnLeft(root), nsa);
		    	int right = simulateMissingNsa(pnRight(root), nsa);
			return (left & right);
		    }
		case OR:
		case OR_LATER:
		    {
		    	int left = simulateMissingNsa(pnLeft(root), nsa);
		    	int right = simulateMissingNsa(pnRight(root), nsa);
			return (left | right);
		    }
		default:
		    {
		    	pnode_t *attNode = pnLeft(root);
		    	if (attNode->node_type == nt_nsaname
			    && (!nsa || attNode->attr_nsa == nsa)) {
				/* Comparison with missing attribute */
				return evlFuzzyFalse;
		    	} else {
				return evlFuzzyMaybe;
			}
		    }
		}
	default:
		assert(0);
		/* NOTREACHED */
		return evlFuzzyMaybe;
	}
	/* NOTREACHED */
}

/*
 * Do the two optimizations outlined above:
 *
 * 1) Rearrange the query tree so that expensive operations can be
 * postponed or foregone.
 *
 * 2) For each attribute in nsAtts, set the nsaResultIfMissing value.
 * Also set nsResultIfAllAttsMissing for the query as a whole.
 * This info is used to short-circuit evaluation of records that don't
 * contain the specified non-standard attributes.
 *
 * Assumes the query tree is already normalized.
 */
void
_evlQOptimizeTree(pnode_t *qexpr, evl_nonStdAtts_t *nsAtts) {
	evl_listnode_t *head, *end, *p;

	if (!nsAtts) {
		return;
	}

	// _evlQPrTree(qexpr);
	_evlQReorderTree(qexpr, nsAtts);
	// _evlQPrTree(qexpr);

	head = nsAtts->nsAtts;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		evl_nonStdAtt_t *nsa = (evl_nonStdAtt_t*) p->li_data;
		nsa->nsaResultIfMissing = simulateMissingNsa(qexpr, nsa);
	}

	nsAtts->nsResultIfAllAttsMissing = simulateMissingNsa(qexpr, NULL);
}
