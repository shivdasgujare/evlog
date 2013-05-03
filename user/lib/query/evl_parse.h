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

#ident "$Header: /cvsroot/evlog/evlog/user/lib/query/evl_parse.h,v 1.5 2002/01/14 19:06:06 kenistoj Exp $"

/* 
 * evl_parse.h
 * This is the include file of data types for the parse tree
 */


#ifndef _EVL_PARSE_H_
#define _EVL_PARSE_H_

#include <sys/types.h>
#include <regex.h>
#include "evl_list.h"
#include "posix_evlog.h"

/*The filter expression tree structure*/

/* typedef enum {att_name, att_val, op} node_enum; */
typedef enum {
	nt_name,
	nt_attname,	/* standard attribute */
	nt_nsaname,	/* non-standard attribute */
	nt_val,		/* integer */
	nt_string,
	nt_regex,
	nt_op
} node_enum;

typedef struct op_node {
	struct node	*left;
	struct node	*right;
}op_node_t;

#define pnLeft(pn) (pn->u_att.op_node.left)
#define pnRight(pn) (pn->u_att.op_node.right)

/*
 * If node_type == nt_name:
 * attr_flag = -1
 * u_att.att_name is the name
 * 
 * If node_type == nt_attname:
 * attr_flag = attribute number (e.g., POSIX_LOG_ENTRY_RECID)
 * u_att.att_name is the name
 * 
 * If node_type == nt_nsaname:
 * attr_nsa = attribute's object in _evlQNonStdAtts list
 * u_att.att_name is the name of the non-standard attribute
 * 
 * If node_type == nt_val:
 *	If there is no explicit sign:
 *		attr_flag = 1
 *		u_att.attr_uval holds the (unsigned) value
 *	If the user supplied a '-':
 *		attr_flag = -1
 *		u_att.attr_val holds the (signed) value
 * 
 * If node_type == nt_string:
 * attr_flag = 0
 * u_att.attr_string is the string
 * 
 * If node_type == nt_regex:
 * attr_flag = 0
 * u_att.attr_regex points to the regcomp-compiled regular expression
 * 
 * If node_type == nt_op:
 * attr_flag = the op value -- e.g., GT
 * u_att.op_node contains the pointers to the operand nodes.
 */

typedef struct node {
	node_enum	node_type;
	union att2 {
		int			attr2_flag;
		struct evl_nonStdAtt	*attr2_nsa;
	} u_att2;
	union att{
		int	attr_no;
		char 	*attr_name;
		char	*attr_string;
		long	attr_val;
		unsigned long	attr_uval;
		regex_t	*attr_regex;
		op_node_t	op_node;
	} u_att;
} pnode_t;

#define attr_flag u_att2.attr2_flag
#define attr_nsa u_att2.attr2_nsa

/*
 * Fuzzy logic:
 * false & x = false (where x = true, false, or maybe)
 * false | x = x
 * true & x = x
 * true | x = true
 * maybe & maybe = maybe
 * maybe | maybe = maybe
 */
typedef int evlFuzzyBoolean;
#define evlFuzzyFalse	0x0
#define evlFuzzyMaybe	0x1
#define evlFuzzyTrue	0x3

/*
 * An evl_nonStdAtt_t object describes a non-standard attribute in a query.
 *
 * The nsaResultIfMissing member is interpreted as follows:
 * nsaResultIfMissing = evlFuzzyTrue means that the entire query always
 *	evaluates to true (1) for any record that doesn't have binary data or
 *	doesn't have a value for the indicated attribute.  For example, the
 *	query '!foo' is true for all records that don't have a foo attribute;
 *	so this flag is set for the foo attribute.
 * nsaResultIfMissing = evlFuzzyFalse means that the entire query always
 *	evaluates to false (0) for any record that doesn't have binary data or
 *	doesn't have a value for the indicated attribute.
 * nsaResultIfMissing = evlFuzzyMaybe means that we can't predict the value
 *	of the query just by knowing that this attribute is missing.
 *
 * The nsaFlags member is interpreted as follows:
 * EVLNSA_NUMERIC means the indicated attribute must be numeric (e.g.,
 *	because the query has a test such as 'foo > 0').
 * EVLNSA_STRMEM means the indicated attribute is a member of a struct
 *	-- e.g., "startPoint.x".
 *
 * nsaAtt is the index of this attribute in the list of non-standard
 * attributes for this query.  It is used at query-evaluation time to
 * select the appropriate attribute in the populated template.
 *
 * Remember that a query object (of which this data structure is a part)
 * is supposed to be const during query evaluation.
 */
typedef struct evl_nonStdAtt {
	char		*nsaName;	/* MUST BE FIRST.  strdup-ed. */
	evlFuzzyBoolean	nsaResultIfMissing;
#define EVLNSA_NUMERIC	0x1
#define EVLNSA_STRREF	0x2
	int		nsaFlags;
	int		nsaAtt;		/* list/array index */
} evl_nonStdAtt_t;

/* One per query (or none, if the query refs no non-standard attributes) */
typedef struct evl_nonStdAtts {
	evl_list_t	*nsAtts;	/* list of evl_nonStdAtt_t's */
	evlFuzzyBoolean	nsResultIfAllAttsMissing;
} evl_nonStdAtts_t;

extern pnode_t *_evlQParseTree;
extern evl_nonStdAtts_t *_evlQNonStdAtts;
extern int _evlQFlags;

extern void _evlQReinitLex(const char *buf);
extern void _evlQEndLex();
extern void _evlQPrTree(pnode_t *tree);
extern int yyparse();
extern pnode_t *_evlQCloneTree(const pnode_t *orig);

extern int _evlQNormalizeTree(pnode_t *tree);
extern void _evlQFreeNonStdAtts(evl_nonStdAtts_t *nsAtts);
extern void _evlQOptimizeTree(pnode_t *qexpr, evl_nonStdAtts_t *nsAtts);
extern int _evlQEvaluateTree(const pnode_t *tree, evl_nonStdAtts_t *nsAtts,
	const struct posix_log_entry *entry, const void *buf);

#define QUERY_ERRMSG_MAXLEN 1024
extern char _evlQueryErrmsg[QUERY_ERRMSG_MAXLEN];

#undef NULL
#define NULL 0

#endif /* _EVL_PARSE_H_ */
