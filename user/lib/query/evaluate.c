/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2001
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
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_template.h"
#include "evl_parse.h"
#include "q.tab.h"

extern char * _evlGetHostNameEx(int id);
/*
 * This file contains _evlQEvaluateTree() and its helper functions.
 * _evlQEvaluateTree() determines whether a given event record matches a
 * given POSIX query.
 */

static int compareSignedInts(int op, long left, long right);
static int compareUnsignedInts(int op, unsigned long left, unsigned long right);
static int compareLonglongs(int op, long long left, long long right);
static int compareUlonglongs(int op, unsigned long long left, unsigned long long right);
static int compareDoubles(int op, double left, double right);
static int compareLongdoubles(int op, long double left, long double right);

static int
compareStrings(int op, const char *left, const char *right)
{
	switch (op) {
	case '=':		return (strcmp(left, right) == 0);
	case NE:		return (strcmp(left, right) != 0);
	case CONTAINS:	return (strstr(left, right) != NULL);
	}

	/* Can't happen. */
	return 0;
}

/*
 * opnode is the root of a comparison with a non-standard attribute.
 * Evaluate the comparison, returning 1 (true) or 0 (false).
 */
static int
evaluateNonStdComparison(const pnode_t *opnode, evlattribute_t *tmplAtts[])
{
	const pnode_t *left = pnLeft(opnode);
	const pnode_t *right = pnRight(opnode);
	int op = opnode->attr_flag;
	evlattribute_t *att;
	long sval;
	int attType;

	if (!tmplAtts) {
		return 0;
	}
	att = tmplAtts[left->attr_nsa->nsaAtt];
	if (!att || !attExists(att)) {
		return 0;
	}

	if (right->node_type == nt_string || right->node_type == nt_regex) {
		/*
		 * Get the string equivalent of the attribute value, for
		 * comparison either via strstr or via regexec.
		 */
		char leftStr[EVL_ATTRSTR_MAXLEN];
		int status;

		status = evlatt_getstring(att, leftStr, EVL_ATTRSTR_MAXLEN);
		assert(status == 0);

		if (right->node_type == nt_string) {
			const char *rightStr = right->u_att.attr_string;
			return compareStrings(op, leftStr, rightStr);
		} else {
			status = regexec(right->u_att.attr_regex, leftStr,
				0, NULL, 0);
			if (op == '~') {
				return (status == 0);
			} else {
				/* Must be !~ */
				return (status != 0);
			}
		}
	}

	assert(right->node_type == nt_val);
	sval = (long) right->u_att.attr_val;
	
	switch(evlatt_gettype(att)) {
	case TY_CHAR:
	case TY_WCHAR:
	case TY_SHORT:
	case TY_INT:
	case TY_LONG:
		return compareSignedInts(op, evl_getLongAttVal(att), sval);
	case TY_UCHAR:
	case TY_USHORT:
	case TY_UINT:
	case TY_ULONG:
		return compareUnsignedInts(op, evl_getUlongAttVal(att), sval);
	case TY_LONGLONG:
		return compareLonglongs(op, evl_getLonglongAttVal(att), sval);
	case TY_ULONGLONG:
		return compareUlonglongs(op, evl_getUlonglongAttVal(att), sval);
	case TY_FLOAT:
	case TY_DOUBLE:
		return compareDoubles(op, evl_getDoubleAttVal(att), sval);
	case TY_LDOUBLE:
		return compareLongdoubles(op, evl_getLongdoubleAttVal(att), sval);
	}
	
	/* TODO: Handle unsupported types such as TY_WSTRING. */
	return 0;
}

/*
 * Return 1 if the expression tree rooted at root evaluates to true
 * for the event record (entry, buf), or to 0 otherwise.  tmplAtts is
 * the array of pointers into the populated template for any non-standard
 * attributes referenced by the query.
 */
static int
evaluateTree(const pnode_t *root, evlattribute_t *tmplAtts[],
	const struct posix_log_entry *entry, const void *buf)
{
	const pnode_t *left;
	const pnode_t *right;
	int op;
	int attIndex;
	long sval;
	unsigned long uval;

	if (root->node_type == nt_attname) {
		/* True if this record has this attribute. */
		if (root->attr_flag == POSIX_LOG_ENTRY_DATA) {
			return (entry->log_size > 0);
		}
		return 1;
	}

	if (root->node_type == nt_nsaname) {
		/* True if this record has this non-standard attribute. */
		evlattribute_t *att;
		if (!tmplAtts) {
			return 0;
		}
		att = tmplAtts[root->attr_nsa->nsaAtt];
		return (att && attExists(att));
	}

	assert(root->node_type == nt_op);
	left = pnLeft(root);
	right = pnRight(root);
	op = root->attr_flag;

	if (op == AND) {
		return (evaluateTree(left, tmplAtts, entry, buf)
			&& evaluateTree(right, tmplAtts, entry, buf));
	}
	if (op == OR) {
		return (evaluateTree(left, tmplAtts, entry, buf)
			|| evaluateTree(right, tmplAtts, entry, buf));
	}
	if (op == '!') {
		return (! evaluateTree(left, tmplAtts, entry, buf));
	}

	if (left->node_type == nt_nsaname) {
		return evaluateNonStdComparison(root, tmplAtts);
	}

	attIndex = left->attr_flag;
	if (right->node_type == nt_string || right->node_type == nt_regex) {
		/*
		 * Get the string equivalent of the attribute value, for
		 * comparison either via strstr or via regexec.
		 */
		const char *leftStr;
		char memStr[POSIX_LOG_MEMSTR_MAXLEN];
		int status;

		if (attIndex == POSIX_LOG_ENTRY_DATA) {
			if (entry->log_format != POSIX_LOG_STRING) {
				/* Can compare only string data. */
				return 0;
			}
			leftStr = (const char*) buf;
		} else {
			if (attIndex == EVL_ENTRY_HOST) {
				leftStr = _evlGetHostNameEx(entry->log_processor>>16);
			} else {
				status = posix_log_memtostr(attIndex, entry, memStr,
											POSIX_LOG_MEMSTR_MAXLEN);
				assert(status == 0);
				leftStr = (const char*) memStr;
			}
		}

		if (right->node_type == nt_string) {
			const char *rightStr = right->u_att.attr_string;
			return compareStrings(op, leftStr, rightStr);
		} else {
			status = regexec(right->u_att.attr_regex, leftStr,
				0, NULL, 0);
			if (op == '~') {
				return (status == 0);
			} else {
				/* Must be !~ */
				return (status != 0);
			}
		}
	}

	assert(right->node_type == nt_val);
	sval = right->u_att.attr_val;
	uval = right->u_att.attr_uval;

	switch (attIndex) {
	case POSIX_LOG_ENTRY_RECID:
		return compareUnsignedInts(op, entry->log_recid, uval);
	case POSIX_LOG_ENTRY_SIZE:
		return compareUnsignedInts(op, entry->log_size, uval);
	case POSIX_LOG_ENTRY_FORMAT:
		return compareSignedInts(op, entry->log_format, sval);
	case POSIX_LOG_ENTRY_EVENT_TYPE:
		return compareSignedInts(op, entry->log_event_type, sval);
	case POSIX_LOG_ENTRY_FACILITY:
		return compareUnsignedInts(op, entry->log_facility, uval);
	case POSIX_LOG_ENTRY_SEVERITY:
	    {
		int order;
		int status = posix_log_severity_compare(&order,
			entry->log_severity, sval);
	    	assert(status == 0);
		return compareSignedInts(op, order, 0);
	    }
	case POSIX_LOG_ENTRY_UID:
		return compareUnsignedInts(op, entry->log_uid, uval);
	case POSIX_LOG_ENTRY_GID:
		return compareUnsignedInts(op, entry->log_gid, uval);
	case POSIX_LOG_ENTRY_PID:
		return compareSignedInts(op, entry->log_pid, sval);
	case POSIX_LOG_ENTRY_PGRP:
		return compareSignedInts(op, entry->log_pgrp, sval);
	case POSIX_LOG_ENTRY_TIME:
		return compareSignedInts(op, entry->log_time.tv_sec, sval);
	case POSIX_LOG_ENTRY_FLAGS:
		return compareUnsignedInts(op, entry->log_flags, uval);
	case POSIX_LOG_ENTRY_THREAD:
		return compareUnsignedInts(op, entry->log_thread, uval);
	case POSIX_LOG_ENTRY_PROCESSOR:
		return compareSignedInts(op, entry->log_processor & 0x0000ffff, sval);
	case EVL_ENTRY_AGE:
	    {
		/*
		 * 'thisEvent.age < 300' is equivalent to
		 * '300 seconds ago' < thisEvent.time
		 * normalize.c has already converted the value to seconds.
		 */
		time_t nSecondsAgo = time(NULL) - uval;
		return compareSignedInts(op, nSecondsAgo,
			entry->log_time.tv_sec);
	    }
	case EVL_ENTRY_HOST:
		return compareUnsignedInts(op, entry->log_processor >> 16, uval);
	}
	
	/* Can't happen */
	return 0;
}

static int
isNumericAtt(evlattribute_t *att)
{
	if (isArray(att)) {
		return 0;
	}

	switch(evlatt_gettype(att)) {
	case TY_CHAR:
	case TY_SHORT:
	case TY_INT:
	case TY_LONG:
	case TY_UCHAR:
	case TY_USHORT:
	case TY_UINT:
	case TY_ULONG:
	case TY_LONGLONG:
	case TY_ULONGLONG:
	case TY_FLOAT:
	case TY_DOUBLE:
	case TY_LDOUBLE:
	case TY_WCHAR:
		return 1;
	default:
		return 0;
	}
}

/* Context for evaluating an expression with non-standard attributes */
struct eval_context {
	const struct posix_log_entry	*entry;
	const void			*buf;
	evl_nonStdAtts_t		*nsAtts;
	evltemplate_t			*tmpl;
	evlattribute_t			**tmplAtts;
	int				anyValidAtts;
	int				anyStrRefs;
};

static void
init_context(struct eval_context *ec, const struct posix_log_entry *entry,
	const void *buf, evl_nonStdAtts_t *nsAtts)
{
	ec->entry = entry;
	ec->buf = buf;
	ec->nsAtts = nsAtts;
	ec->tmpl = NULL;
	ec->tmplAtts = NULL;
	ec->anyValidAtts = 0;
	ec->anyStrRefs = 0;
}

/*
 * tmpl is an unpopulated clone template.  Store pointers to its attributes
 * in tmplAtts.  If it doesn't have all the attributes we're interested in, try
 * to predict the result of the query.  Return evlFuzzyTrue or evlFuzzyFalse
 * if we can predict the result of the query, or evlFuzzyMaybe otherwise.
 *
 * Set *anyValidAtts to 1 if this template has any attributes we're
 * interested in, or 0 otherwise.
 */
static evlFuzzyBoolean
screenUnpopulatedTemplate(struct eval_context *ec)
{
	int nsi;
	evl_nonStdAtt_t *nsa;
	evlattribute_t *att;
	evl_listnode_t *head, *end, *p;
	int foundInvalidAtt = 0;
	evlFuzzyBoolean fb;
	int status;

	/*
	 * First time through, just mark each non-standard attribute as valid
	 * (i.e., useful to this query) or not.  If it's valid, we remember
	 * its address.
	 *
	 * Theoretically, nsi just counts up from 0.
	 */
	head = ec->nsAtts->nsAtts;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		evltemplate_t *t = ec->tmpl;
		nsa = (evl_nonStdAtt_t*) p->li_data;
		nsi = nsa->nsaAtt;
		if ((nsa->nsaFlags & EVLNSA_STRREF) != 0) {
			/*
			 * Since tmpl is a clone but not yet populated,
			 * evltemplate_getatt() won't find any struct
			 * members.  Look in the master template and
			 * point to that attribute (if any) for now.
			 * We'll fix up such pointers if and when we
			 * populate tmpl.
			 */
			t = t->tm_master;
			ec->anyStrRefs = 1;
		}
		status = evltemplate_getatt(t, nsa->nsaName, &att);
		if (status == 0) {
			/* Template offers an attribute with this name. */
			if ((nsa->nsaFlags & EVLNSA_NUMERIC) != 0
			    && !isNumericAtt(att)) {
				/*
				 * Can't compare a number in the query to
				 * a non-numeric attribute.  (Note that we
				 * CAN compare a numeric attribute -- at
				 * least the string representation thereof --
				 * to a string or regular expression in the
				 * query.)
				 */
				ec->tmplAtts[nsi] = NULL;
				foundInvalidAtt = 1;
			} else {
				ec->tmplAtts[nsi] = att;
				ec->anyValidAtts = 1;
			}
		} else {
			assert(status == ENOENT);
			ec->tmplAtts[nsi] = NULL;
			foundInvalidAtt = 1;
		}
	}

	if (!ec->anyValidAtts) {
		/*
		 * This template doesn't offer any interesting attributes.
		 * Note that when all attributes are missing/invalid, we
		 * don't need to check each individual attribute's
		 * nsaResultIfMissing flag.
		 */
		return  ec->nsAtts->nsResultIfAllAttsMissing;
	}

	if (!foundInvalidAtt) {
		return evlFuzzyMaybe;
	}

	/*
	 * We have at least one valid attribute and at least one invalid one.
	 * See if we can short-circuit the query based on the invalid ones.
	 */
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		nsa = (evl_nonStdAtt_t*) p->li_data;
		nsi = nsa->nsaAtt;
		if (! ec->tmplAtts[nsi]) {
			fb = nsa->nsaResultIfMissing;
			if (fb != evlFuzzyMaybe) {
				return fb;
			}
		}
	}
	return evlFuzzyMaybe;
}

/*
 * Replace struct-attribute refs from the master template with those from
 * the populated clone.
 */
static void
fixStructRefs(struct eval_context *ec)
{
	int nsi;
	evl_nonStdAtt_t *nsa;
	evl_listnode_t *head, *end, *p;
	int status;

	head = ec->nsAtts->nsAtts;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		nsa = (evl_nonStdAtt_t*) p->li_data;
		nsi = nsa->nsaAtt;
		if (ec->tmplAtts[nsi] != NULL
		    && (nsa->nsaFlags & EVLNSA_STRREF) != 0) {
			status = evltemplate_getatt(ec->tmpl, nsa->nsaName,
				&ec->tmplAtts[nsi]);
			if (status != 0) {
				ec->tmplAtts[nsi] = NULL;
			}
		}
	}
}

/*
 * We are about to evaluate a query with reference to the event record
 * (entry, buf).  The query refers to non-standard attributes.
 * Load the template (if any) associated with this event record and
 * populate it.  Since template operations are relatively expensive, we
 * use the information set up by _evlQOptimizeTree() to try to minimize
 * those operations.  For example, if we can tell without populating the
 * template that the query is going to evaluate to true (or false), we
 * just release the template and return evlFuzzyTrue (or evlFuzzyFalse).
 * If we can't predict the result of the query, we return evlFuzzyMaybe.
 *
 * If we need access to the populated template's attributes in order
 * to complete evaluation of the query, we allocate an array of pointers
 * to those attributes, and set pTmplAtts pointing to that array.
 */
static evlFuzzyBoolean
prepareNonStdAtts(struct eval_context *ec)
{
	evlFuzzyBoolean fb;
	int status;

	assert(ec->nsAtts != NULL);

	/* Look for the template. */
	status = evl_gettemplate(ec->entry, ec->buf, &ec->tmpl);
	if (status != 0) {
		/*
		 * No template available to interpret this record.
		 * This record can't possibly have any such attributes.
		 */
		return ec->nsAtts->nsResultIfAllAttsMissing;
	}

	/*
	 * tmpl is an unpopulated clone template.  Store pointers to its
	 * attributes in tmplAtts.  If it doesn't have all the attributes
	 * we're interested in, we may be able to predict the result
	 * of the query without populating the template.  And if it has
	 * none of them, there's no point in populating the template.
	 *
	 * NOTE: If this template will never (or always) provide a match
	 * for this query, it would be nice to remember that somehow...
	 */
	ec->tmplAtts = (evlattribute_t**)
		malloc(_evlGetListSize(ec->nsAtts->nsAtts) *
		sizeof(evlattribute_t*));
	assert(ec->tmplAtts != NULL);
	fb = screenUnpopulatedTemplate(ec);
	if (!ec->anyValidAtts || fb != evlFuzzyMaybe) {
		(void) evl_releasetemplate(ec->tmpl);
		return fb;
	}
	
	status = evl_populatetemplate(ec->tmpl, ec->entry, ec->buf);
	assert(status == 0);

	/*
	 * Now that the template is populated, and we know which attributes
	 * have values for this particular record, we could do even more
	 * pre-screening.  But it's probably about as efficient to let
	 * evaluateTree() handle the rest.
	 *
	 * If there are any refs to the master template in tmplAtts, we need
	 * to replace those with refs to the now-populated clone.
	 */
	if (ec->anyStrRefs) {
		fixStructRefs(ec);
	}

	return evlFuzzyMaybe;
}

/*
 * The query tree references at least one non-standard attribute.
 * If the root node is AND_LATER or OR_LATER, evaluate the left branch,
 * set leftResult to that result (0 or 1), and return the fuzzy-truth
 * value (fuzzyMaybe if the right branch will need to be evaluated).
 * Otherwise (root is something else), just return -1 and fuzzyMaybe.
 */
evlFuzzyBoolean
evaluateLeft(const pnode_t *root, const struct posix_log_entry *entry,
	const void *buf, int *leftResult)
{
	*leftResult = -1;

	if (root->node_type != nt_op) {
		return evlFuzzyMaybe;
	}
	switch (root->attr_flag) {
	case AND_LATER:
		*leftResult = evaluateTree(pnLeft(root), NULL, entry, buf);
		if (*leftResult) {
			return evlFuzzyMaybe;
		} else {
			return evlFuzzyFalse;
		}
	case OR_LATER:
		*leftResult = evaluateTree(pnLeft(root), NULL, entry, buf);
		if (*leftResult) {
			return evlFuzzyTrue;
		} else {
			return evlFuzzyMaybe;
		}
	default:
		return evlFuzzyMaybe;
	}
	/*NOTREACHED*/
}

/*
 * Evaluate the query tree rooted at root; this tree references
 * non-standard attributes.
 */
int
evaluateNsaTree(const pnode_t *root, evl_nonStdAtts_t *nsAtts,
	const struct posix_log_entry *entry, const void *buf)
{
	int result, leftResult = -1;
	evlFuzzyBoolean fbResult;
	struct eval_context ec;
	const pnode_t *evalRoot = root;

	/*
	 * First see if we can evaluate the query referencing only standard
	 * attributes.
	 */
	fbResult = evaluateLeft(root, entry, buf, &leftResult);
	if (fbResult != evlFuzzyMaybe) {
		/* We have a result already. */
		return (fbResult == evlFuzzyTrue ? 1 : 0);
	} else if (leftResult != -1) {
		/* Left branch was inconclusive; right will tell. */
		evalRoot = pnRight(root);
	}

	/*
	 * Either there was no "pure" left branch to evaluate, or
	 * the final truth will be told by the "impure" right branch.
	 * Try to load and, if necessary, populate the appropriate
	 * template to get at the non-standard attributes.
	 */
	init_context(&ec, entry, buf, nsAtts);
	fbResult = prepareNonStdAtts(&ec);
	if (fbResult != evlFuzzyMaybe) {
		/* We have a result already. */
		result = (fbResult == evlFuzzyTrue ? 1 : 0);
	} else {
		result = evaluateTree(evalRoot, ec.tmplAtts, entry, buf);
	}

	if (ec.tmpl) {
		(void) evl_releasetemplate(ec.tmpl);
	}
	if (ec.tmplAtts) {
		free(ec.tmplAtts);
	}
	return result;
}

/*
 * Return 1 if the expression tree rooted at root evaluates to true
 * for the event record (entry, buf), or to 0 otherwise.
 */
int
_evlQEvaluateTree(const pnode_t *root, evl_nonStdAtts_t *nsAtts,
	const struct posix_log_entry *entry, const void *buf)
{
	if (nsAtts) {
		return evaluateNsaTree(root, nsAtts, entry, buf);
	} else {
		return evaluateTree(root, NULL, entry, buf);
	}
}

/*
 * WARNING ABOUT MULTI-LINE MACROS: [get from template.c]
 */

#define NUMERIC_COMPARISONS \
	default:	return 0; \
	case '=':	return (left == right);	\
	case NE:	return (left != right);	\
	case '<':	return (left < right);	\
	case LE:	return (left <= right);	\
	case '>':	return (left > right);	\
	case GE:	return (left >= right);

static int
compareSignedInts(int op, long left, long right)
{
	switch(op) {
	NUMERIC_COMPARISONS
	case '&':	return ((left & right) != 0);
	}
}

static int
compareUnsignedInts(int op, unsigned long left, unsigned long right)
{
	switch(op) {
	NUMERIC_COMPARISONS
	case '&':	return ((left & right) != 0);
	}
}

static int
compareLonglongs(int op, long long left, long long right)
{
	switch(op) {
	NUMERIC_COMPARISONS
	case '&':	return ((left & right) != 0);
	}
}

static int
compareUlonglongs(int op, unsigned long long left, unsigned long long right)
{
	switch(op) {
	NUMERIC_COMPARISONS
	case '&':	return ((left & right) != 0);
	}
}

static int
compareDoubles(int op, double left, double right)
{
	switch(op) {
	NUMERIC_COMPARISONS
	}
}

static int
compareLongdoubles(int op, long double left, long double right)
{
	switch(op) {
	NUMERIC_COMPARISONS
	}
}

/*
 * Don't add code here...
 * See WARNING ABOUT MULTI-LINE MACROS: [get from template.c]
 */
