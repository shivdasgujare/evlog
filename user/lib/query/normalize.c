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
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_parse.h"
#include "q.tab.h"

#define GETGR_R_SIZE_MAX 128
#define GETPW_R_SIZE_MAX 128

/*
 * This file contains _evlQNormalizeTree() and its helper functions.
 * _evlQNormalizeTree() takes a parse tree built by the query parser and
 * "normalizes" it.  Normalization includes:
 * - Detecting semantic errors
 * - Converting attribute names into member selectors (e.g., "size" ->
 *	POSIX_LOG_ENTRY_SIZE)
 * - Optimizing expressions where permissible.  For example, 'uid = "root"'
 *	becomes 'uid = 0'.
 *
 * This file also contains _evlQOptimizeTree() and its helper functions.
 * So far, optimization is aimed only at expressions that refer to
 * non-standard (i.e., template-defined) attributes.
 */

/* Syntax and semantic errors are recorded here. */
char _evlQueryErrmsg[QUERY_ERRMSG_MAXLEN];

/* List of non-standard attributes */
evl_nonStdAtts_t *_evlQNonStdAtts;

/*
 * posix_log_query_create() sets this global variable to EVL_PRPS_TEMPLATE
 * if we're using templates, in which case non-standard attr names are OK,
 * or to EVL_PRPS_RESTRICTED if restricted query is needed.
 */
int _evlQFlags = 0;

static struct _evlNvPair nvFlags[] = {
	{ POSIX_LOG_TRUNCATE,	"POSIX_LOG_TRUNCATE" },
	{ POSIX_LOG_TRUNCATE,	"TRUNCATE" },
	{ EVL_KERNEL_EVENT,	"KERNEL" },
#ifdef EVL_PRINTK
	{ EVL_PRINTK,		"PRINTK" },
#endif
#ifdef EVL_INTERRUPT
	{ EVL_INTERRUPT,	"INTERRUPT" },
#endif
	{ 0, 0 }
};

static void
semanticError(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(_evlQueryErrmsg, sizeof(_evlQueryErrmsg), fmt, args);
	va_end(args);
}

static int standardIntOps[] = {
	'=', NE, '<', LE, '>', GE, 0
};

static int standardStringOps[] = {
	'=', NE, CONTAINS, 0
};

static int uidGidOps[] = {
	'=', NE, 0
};

static const char *
getAttNameFromOpNode(const pnode_t *opNode)
{
	return pnLeft(opNode)->u_att.attr_name;
}

/* Return 0 if opNode's op code is in opTable, or -1 otherwise. */
static int
verifyOp(pnode_t *opNode, int opTable[])
{
	int op = opNode->attr_flag;
	int *pop;
	for (pop = opTable; *pop != 0; pop++) {
		if (op == *pop) {
			return 0;
		}
	}
	semanticError("Unsupported operation for %s attribute",
		getAttNameFromOpNode(opNode));
	return -1;
}

static void
wrongOperandType(pnode_t *opNode) {
	semanticError("Unsupported operand type for %s attribute",
		getAttNameFromOpNode(opNode));
}

/*
 * attName is the name of a non-standard attribute.  Add it to the list of
 * non-standard attributes if it's not already there.  Return the address of
 * its object in the list.
 */
static evl_nonStdAtt_t *
registerNonStdAtt(const char *attName)
{
	evl_nonStdAtt_t *nsa;
	evl_listnode_t *p;
	
	if (_evlQNonStdAtts) {
		p = _evlFindNamedItemInList(_evlQNonStdAtts->nsAtts, attName);
	} else {
		/* First nsa for this query.  Create the container. */
		evl_nonStdAtts_t *ns;
		p = NULL;
		ns = (evl_nonStdAtts_t*) malloc(sizeof(evl_nonStdAtts_t));
		assert(ns != NULL);
		ns->nsAtts = NULL;
		ns->nsResultIfAllAttsMissing = evlFuzzyMaybe;
		_evlQNonStdAtts = ns;
	}

	if (p) {
		nsa = (evl_nonStdAtt_t*) p->li_data;
	} else {
		nsa = (evl_nonStdAtt_t*) malloc(sizeof(evl_nonStdAtt_t));
		assert(nsa != NULL);
		nsa->nsaName = strdup(attName);
		assert(nsa->nsaName != NULL);
		nsa->nsaFlags = 0;
		if (strchr(attName, '.')) {
			nsa->nsaFlags |= EVLNSA_STRREF;
		}
		nsa->nsaAtt = _evlGetListSize(_evlQNonStdAtts->nsAtts);
		_evlQNonStdAtts->nsAtts = _evlAppendToList(
			_evlQNonStdAtts->nsAtts, nsa);
	}
	return nsa;
}

/* Free the specified list of non-standard attributes. */
void
_evlQFreeNonStdAtts(evl_nonStdAtts_t *nsAtts)
{
	evl_listnode_t *head, *end, *p;
	evl_nonStdAtt_t *nsa;

	head = nsAtts->nsAtts;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		nsa = (evl_nonStdAtt_t*) p->li_data;
		if (nsa->nsaName) {
			free(nsa->nsaName);
		}
	}
	_evlFreeList(head, 1);
}

/*
 * Convert nameNode from type nt_name to nt_attname by looking up
 * the attribute name and storing the attribute index in nameNode.
 *
 * If the attribute name is not recognized, it's an error -- unless the
 * _evlQFlags flag is set, in which case we:
 * 1. add the attribute name to the _evlQNonStdAtts list;
 * 2. store a pointer to the attribute's object in the _evlQNonStdAtts
 * 	list; and
 * 3. set the node type to nt_nsaname.
 * Returns 0 for a standard attribute, 1 for a legal non-standard attribute,
 * or -1 for an error.
 */
static int
makeAttnameNode(pnode_t *nameNode)
{
	int attIndex;
	assert(nameNode != NULL);
	assert(nameNode->node_type == nt_name);
	
	attIndex = _evlGetValueByName(_evlAttributes, nameNode->u_att.attr_name,
		-1);
	if (attIndex == -1) {
		if (!strcmp(nameNode->u_att.attr_name, "data")) {
			if (_evlQFlags & EVL_PRPS_RESTRICTED) {
				semanticError("Cannot refer to data attribute in restricted query");
				return -1;
			}
			attIndex = POSIX_LOG_ENTRY_DATA;
		} else if (!strcmp(nameNode->u_att.attr_name, "age")) {
			attIndex = EVL_ENTRY_AGE;
		} else if (!strcmp(nameNode->u_att.attr_name, "host")) {
			attIndex = EVL_ENTRY_HOST; 
		} else {
			if (_evlQFlags & EVL_PRPS_TEMPLATE) {
				nameNode->attr_nsa = registerNonStdAtt(
					nameNode->u_att.attr_name);
				nameNode->node_type = nt_nsaname;
				return 1;
			} else {
				semanticError("Unknown attribute: %s",
					nameNode->u_att.attr_name);
				return -1;
			}
		}
	}
	nameNode->attr_flag = attIndex;
	nameNode->node_type = nt_attname;
	return 0;
}

/*
 * pnRight(opNode) is a string.  If the operation is ~ or !~, convert
 * pnRight(opNode) to a parsed regular expression.  Returns 1 for a
 * successfully parsed regular expression, -1 for an illegal regex,
 * or 0 if it's not a regex operation at all.
 */
int
normalizeRegex(pnode_t *opNode)
{
	int op = opNode->attr_flag;
	if (op == '~' || op == RENOMATCH) {
		int errcode;
		pnode_t *right = pnRight(opNode);
		regex_t *preg = (regex_t*) malloc(sizeof(regex_t));
		assert(preg != NULL);
		errcode = regcomp(preg, right->u_att.attr_string,
			(REG_EXTENDED | REG_NOSUB));
		if (errcode == 0) {
			/*
			 * It's a valid regular expression.  Replace the
			 * string with the compiled regular expression.
			 */
			free(right->u_att.attr_string);
			right->u_att.attr_regex = preg;
			right->node_type = nt_regex;
			return 1;
		} else {
			char errbuf[100];
			(void) regerror(errcode, preg, errbuf, 100);
			semanticError("Invalid regular expression for %s attribute: %s",
				getAttNameFromOpNode(opNode), errbuf);
			free(preg);	/* Should this be regfree(preg) ? */
			return -1;
		}
	} else {
		return 0;
	}
}

/*
 * pnRight(opNode) is a string.  If the operation is ~ or !~, convert
 * pnRight(opNode) to a parsed regular expression.  Otherwise, just verify
 * that the operation is valid for a string.  Returns 0 on success, -1 on
 * failure.
 */
int
normalizeStringOrRegex(pnode_t *opNode)
{
	switch (normalizeRegex(opNode)) {
	case 1:		return 0;
	case -1:	return -1;
	case 0:
		/* Should be an ordinary string-comparison operation. */
		return verifyOp(opNode, standardStringOps);
	}
	/* Should never reach here, but keep gcc happy. */
	return -1;
}

/*
 * Verify that opNode's right child is an integer in the indicated range.
 * sign and size define the range/type: sign is -1 for signed, 1 for unsigned;
 * size is the size, in bytes, of the attribute.
 */
static int
normalizeInteger(int sign, int size, pnode_t *opNode)
{
	int error = 0;
	pnode_t *valNode = pnRight(opNode);
	unsigned long uval = valNode->u_att.attr_val;
	long sval = (long) valNode->u_att.attr_val;
	int force_sign_extend = 0;

	if (valNode->node_type != nt_val) {
		semanticError("%s attribute requires integer operand",
			getAttNameFromOpNode(opNode));
		return -1;
	}

	/* Verify that the integer is in the appropriate range. */
	if (sign == 1			/* unsigned type */
	    && valNode->attr_flag == -1	/* number had a minus sign */
	    && uval != 0) {		/* -0 is OK */ 
		semanticError("%s attribute requires unsigned operand",
			getAttNameFromOpNode(opNode));
		return -1;
	}

	if (valNode->attr_flag == -1) {
		/* Value is of a signed type. */
		if (size == sizeof(char)) {
			error = (sval < SCHAR_MIN || sval > SCHAR_MAX);
		} else if (size == sizeof(short)) {
			error = (sval < SHRT_MIN || sval > SHRT_MAX);
		} else if (size == sizeof(int)) {
			error = (sval < INT_MIN || sval > INT_MAX);
		}
	} else {
		/* Value is of an unsigned type. */
		if (size == sizeof(unsigned char)) {
			error = (uval > UCHAR_MAX);
			force_sign_extend = (uval > CHAR_MAX);
		} else if (size == sizeof(unsigned short)) {
			error = (uval > USHRT_MAX);
			force_sign_extend = (uval > SHRT_MAX);
		} else if (size == sizeof(unsigned int)) {
			error = (uval > UINT_MAX);
			force_sign_extend = (uval > INT_MAX);
		}
	}
	if (error) {
		semanticError("%s attribute's operand is out of range",
			getAttNameFromOpNode(opNode));
		return -1;
	}
	if (sign == -1 && force_sign_extend && size < sizeof(sval)) {
		/*
		 * We allow things like "event_type = 0xabcdffff" where
		 * the value is unsigned (and fits in 'size' bytes) but
		 * the attribute is signed.  In order to make the
		 * comparison work, we need to sign-extend the value.
		 * Note that this means "event_type = 4294967295" will
		 * be accepted but interpreted as "event_type = -1".
		 */
		if (size == sizeof(int)) {
			sval = (int) uval;
		} else if (size == sizeof(short)) {
			sval = (short) uval;	
		} else if (size == sizeof(char)) {
			sval = (signed char) uval;
		}
		valNode->u_att.attr_val = sval;
		valNode->attr_flag = -1;
	}
	return 0;
}

static int
normalizeAttPlainInt(pnode_t *opNode)
{
	if (normalizeInteger(-1, sizeof(int), opNode) != 0) {
		return -1;
	}
	return verifyOp(opNode, standardIntOps);
}

static int
normalizeAttRecid(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);
	if (valNode->node_type == nt_string) {
		/* Can't remember why we allow this... */
		return verifyOp(opNode, standardStringOps);
	}
	if (normalizeInteger(1, sizeof(posix_log_recid_t), opNode) != 0) {
		return -1;
	}
	return verifyOp(opNode, standardIntOps);
}

static int
normalizeAttData(pnode_t *opNode)
{
	if (pnRight(opNode)->node_type == nt_string) {
		return normalizeStringOrRegex(opNode);
	}
	semanticError("Variable data can be compared only with a string or regular expression.");
	return -1;
}

static int
normalizeAttSize(pnode_t *opNode)
{
	if (normalizeInteger(1, sizeof(size_t), opNode) != 0) {
		return -1;
	}
	/* TODO: Maybe do a stricter range check. */
	return verifyOp(opNode, standardIntOps);
}

static int
parseAge(char *ageString)
{
	char *unit;
	unsigned int nsec = (int) strtoul(ageString, &unit, 10);
	if (unit == ageString || strlen(unit) != 1) {
		return -1;
	}
	switch (*unit) {
	case 's':	return nsec;
	case 'm':	return nsec * 60;
	case 'h':	return nsec * 60*60;
	case 'd':	return nsec * 60*60*24;
	}
	return -1;
}

static int
normalizeAttAge(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);

	if (valNode->node_type == nt_val) {
		if (normalizeInteger(1, sizeof(time_t), opNode) != 0) {
			return -1;
		}
		/* 5 means 5 days.  Convert to seconds. */
		valNode->u_att.attr_val *= 24*60*60;
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string) {
		/* Convert the string node "2h" to the val node 2*60*60. */
		char *ageString = valNode->u_att.attr_string;
		int nsec = parseAge(ageString);
		if (nsec < 0) {
			semanticError("Unrecognized age specification: %s",
				ageString);
			return -1;
		}
		
		/* Change the string node to an integer node. */
		free(ageString);
		valNode->node_type = nt_val;
		valNode->attr_flag = 1;	/* unsigned integer */
		valNode->u_att.attr_val = nsec;

		return verifyOp(opNode, standardIntOps);
	}
	wrongOperandType(opNode);	/* No symbolic times. */
	return -1;
}

static int
normalizeAttHost(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);

	if (valNode->node_type == nt_val) {
		if (normalizeInteger(1, sizeof(time_t), opNode) != 0) {
			return -1;
		}
		if (valNode->u_att.attr_val < 0 || valNode->u_att.attr_val > 0xffff){
			semanticError("Out of range");
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string ||
			   valNode->node_type == nt_name) {
		int op = opNode->attr_flag;

		if (op != '~' && op != RENOMATCH && op != CONTAINS ) {
			char *id = valNode->u_att.attr_string;
			int nodeId;
			char *p;
			
			if ((p=strchr(id, '.')) != NULL) {
				char *endp = 0;
				int lowerbyte, upperbyte;
				upperbyte = (int) strtoul(id, &endp, 10);
				if (*endp != '.') {
					goto host_name;
				}
				lowerbyte = (int) strtoul(p + 1, &endp, 10);
				if (*endp != '\0') {
					goto host_name;
				}
				if (upperbyte > 255 || lowerbyte > 255) {
					semanticError("Invalid node id.");
					return -1;
				}
				valNode->node_type = nt_val;
				valNode->attr_flag = 1;	/* unsigned integer */
				valNode->u_att.attr_val = (upperbyte << 8) + lowerbyte;
				return verifyOp(opNode, standardIntOps);
			}
		host_name:
			/* host name */
			nodeId = _evlGetNodeId(valNode->u_att.attr_string);
			if (nodeId == -1) {
				semanticError("Unrecognized host name");
				return -1;
			}
			valNode->node_type = nt_val;
			valNode->attr_flag = 1;	/* unsigned integer */
			valNode->u_att.attr_val = nodeId;
			return 0;
		}
		else if (op == CONTAINS) {
			return 0;
		}
		else if ((op == '~' || op == RENOMATCH)) { 
			switch(normalizeRegex(opNode)) {
			case -1:	return -1;
			case 1:		return 0;
				/* case 0 -> not a regular-expression operation */
			}
		}
		
	}
	return -1;
}

/*
 * E.g., change KERN or kern (name) to LOG_KERN (integer).
 * This also happens to work for strings (e.g., "KERN" -> LOG_KERN) as long
 * as attr_name and attr_string are the same value.
 */
static int
changeNameNodeToInt(pnode_t *opNode, struct _evlNvPair table[])
{
	pnode_t *valNode = pnRight(opNode);
	int code = _evlGetValueByCIName(table, valNode->u_att.attr_name, -1);
	if (code == -1) {
		semanticError("Unrecognized value for attribute %s: %s",
			getAttNameFromOpNode(opNode),
			valNode->u_att.attr_name);
		return -1;
	}

	/* Change the name node to an integer node. */
	free(valNode->u_att.attr_name);
	valNode->node_type = nt_val;
	valNode->attr_flag = -1;	/* signed integer */
	valNode->u_att.attr_val = code;
	return 0;
}

static int
normalizeAttFacility(pnode_t *opNode)
{
	static int facilityOps[] = { '=', NE, 0 };
	pnode_t *valNode = pnRight(opNode);

	if (valNode->node_type == nt_string) {
		switch(normalizeRegex(opNode)) {
		case -1:	return -1;
		case 1:		return 0;
		/* case 0 -> not a regular-expression operation */
		}
	}

	/* Change KERN or "KERN" to LOG_KERN. */
	if (valNode->node_type == nt_name || valNode->node_type == nt_string) {
		posix_log_facility_t code = _evlGetFacilityCodeByCIName(
			valNode->u_att.attr_name);
		if (code == EVL_INVALID_FACILITY) {
			semanticError("Unrecognized value for facility attribute: %s",
			valNode->u_att.attr_name);
			return -1;
		}
		/* Change the name node to an integer node. */
		free(valNode->u_att.attr_name);
		valNode->node_type = nt_val;
		valNode->attr_flag = 1;	/* unsigned integer */
		valNode->u_att.attr_uval = code;
		return verifyOp(opNode, facilityOps);
	}

	/* As an extension to the standard, allow integer facility codes. */
	if (normalizeInteger(1, sizeof(posix_log_facility_t), opNode) != 0) {
		return -1;
	}
	return verifyOp(opNode, standardIntOps);
}

static int
normalizeAttSeverity(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);
	if (valNode->node_type == nt_name) {
		if (changeNameNodeToInt(opNode, _evlSeverities) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string) {
		return verifyOp(opNode, standardStringOps);
	}
	/* No integer severity codes allowed. */
	wrongOperandType(opNode);
	return -1;
}

static int
normalizeAttFormat(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);
	if (valNode->node_type == nt_name) {
		if (changeNameNodeToInt(opNode, _evlFormats) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string) {
		return verifyOp(opNode, standardStringOps);
	} else if (valNode->node_type == nt_val) {
		if (normalizeInteger(1, sizeof(int), opNode) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	}
	return -1;
}

static int
normalizeAttEventType(pnode_t *opNode)
{
	static int eventTypeStringOps[] = { '=', NE, 0 };
	pnode_t *valNode = pnRight(opNode);
	if (valNode->node_type == nt_name) {
		if (changeNameNodeToInt(opNode, _evlMgmtEventTypes) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string) {
		/* Convert the string to its CRC. */
		char *s;
		int crc;
		extern int evl_gen_event_type_v2(const char *);
		if (verifyOp(opNode, eventTypeStringOps) != 0) {
			return -1;
		}
		/* Change the string node to an integer node. */
		s = valNode->u_att.attr_string;
		crc = evl_gen_event_type_v2(s);
		free(s);
		valNode->node_type = nt_val;
		valNode->attr_flag = -1;	/* signed integer */
		valNode->u_att.attr_val = crc;
		return 0;
	} else if (valNode->node_type == nt_val) {
		if (normalizeInteger(-1, sizeof(int), opNode) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	}
	return -1;
}

static int
normalizeAttPid(pnode_t *opNode)
{
	/* pid_t is signed, per POSIX. */
	if (normalizeInteger(-1, sizeof(pid_t), opNode) != 0) {
		return -1;
	}
	return verifyOp(opNode, standardIntOps);
}

static int
normalizeAttUid(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);
	if (valNode->node_type == nt_val) {
		if (normalizeInteger(1, sizeof(uid_t), opNode) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string
	    || valNode->node_type == nt_name) {
		/* Verify that this is a valid uid and convert the valNode. */
		/*
		 * Note: As an extension, we allow 'uid=root' as well as
		 * 'uid="root"'.
		 */
		char *userName = valNode->u_att.attr_string;
#ifdef _POSIX_THREAD_SAFE_FUNCTIONS
		struct passwd *pw, passwd;
		char buf[GETPW_R_SIZE_MAX];
#ifndef __Lynx__
		(void) getpwnam_r(userName, &passwd, buf, GETPW_R_SIZE_MAX,
			&pw);
#else
		getpwnam_r(&passwd, userName, buf, GETPW_R_SIZE_MAX);
		pw = &passwd;
#endif
#else

	    	struct passwd *pw = getpwnam(userName);
#endif
		if (pw) {
			/* Change the string/name node to an integer node. */
			free(userName);
			valNode->node_type = nt_val;
			valNode->attr_flag = 1;	/* unsigned integer */
			valNode->u_att.attr_val = pw->pw_uid;
			return verifyOp(opNode, uidGidOps);
		} else {
			semanticError("Unknown user name: \"%s\"", userName);
			return -1;
		}
	}
	
	wrongOperandType(opNode);
	return -1;
}

static int
normalizeAttGid(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);
	if (valNode->node_type == nt_val) {
		if (normalizeInteger(1, sizeof(gid_t), opNode) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string
	    || valNode->node_type == nt_name) {
		/* Verify that this is a valid gid and convert the valNode. */
		/*
		 * Note: As an extension, we allow 'gid=bin' as well as
		 * 'gid="bin"'.
		 */
		char *groupName = valNode->u_att.attr_string;
#ifdef _POSIX_THREAD_SAFE_FUNCTIONS
		struct group *gr, group;
		char buf[GETGR_R_SIZE_MAX];
#ifndef __Lynx__
		(void) getgrnam_r(groupName, &group, buf, GETGR_R_SIZE_MAX,
			&gr);
#else
		gr = getgrnam_r(&group, groupName, buf, GETGR_R_SIZE_MAX);
#endif
#else

	    	struct group *gr = getgrnam(groupName);
#endif
		if (gr) {
			/* Change the string/name node to an integer node. */
			free(groupName);
			valNode->node_type = nt_val;
			valNode->attr_flag = 1;	/* unsigned integer */
			valNode->u_att.attr_val = gr->gr_gid;
			return verifyOp(opNode, uidGidOps);
		} else {
			semanticError("Unknown group name: \"%s\"", groupName);
			return -1;
		}
	}
	wrongOperandType(opNode);
	return -1;
}

static int
normalizeAttTime(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);

	if (valNode->node_type == nt_val) {
		if (normalizeInteger(-1, sizeof(time_t), opNode) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string) {
		return normalizeStringOrRegex(opNode);
	}
	wrongOperandType(opNode);	/* No symbolic times. */
	return -1;
}

static int
normalizeAttFlags(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);
	int flagOps[] = { '&', 0 };

	if (verifyOp(opNode, flagOps) != 0) {
		return -1;
	}
	if (valNode->node_type == nt_val) {
		return normalizeInteger(1, sizeof(unsigned int), opNode); 
	}
	if (valNode->node_type == nt_name) {
		return changeNameNodeToInt(opNode, nvFlags);
	}
	wrongOperandType(opNode);
	return -1;
}

static int
normalizeAttThread(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);
	if (valNode->node_type == nt_val) {
#ifdef _POSIX_THREADS
		int sign = 1;
		size_t size = sizeof(pthread_t);
#else
		int sign = -1;
		size_t size = sizeof(int);
#endif
		if (normalizeInteger(sign, size, opNode) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	}
	if (valNode->node_type == nt_string) {
		return verifyOp(opNode, standardStringOps);
	}
	wrongOperandType(opNode);
	return -1;
}

static int
normalizeAttProcessor(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);

	/* As an extension, we allow integer comparison with processor ID. */
	if (valNode->node_type == nt_val) {
		if (normalizeInteger(-1, sizeof(posix_log_procid_t), opNode) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string) {
		return normalizeStringOrRegex(opNode);
	}
	wrongOperandType(opNode);	/* No symbolic processor IDs. */
	return -1;
}

/*
 * The left (attribute) branch of the comparison rooted at opNode is a
 * non-standard attribute.  Just verify that the operation and operand are
 * compatible (e.g., reject "myatt ~ 12") and do regex fuss if needed.
 */
static int
normalizeComparisonWithNonStdAtt(pnode_t *opNode)
{
	pnode_t *valNode = pnRight(opNode);

	if (valNode->node_type == nt_val) {
		/* Right operand is numeric, so attribute must be, too. */
		pnode_t *attNode = pnLeft(opNode);
		attNode->attr_nsa->nsaFlags |= EVLNSA_NUMERIC;

		/*
		 * In the evaluate phase, the right-hand integer operand
		 * being compared with a non-standard attribute is treated
		 * as a signed long.
		 */
		if (normalizeInteger(-1, sizeof(long), opNode) != 0) {
			return -1;
		}
		return verifyOp(opNode, standardIntOps);
	} else if (valNode->node_type == nt_string) {
		return normalizeStringOrRegex(opNode);
	}
	/* No symbolic operands for non-standard attributes. */
	wrongOperandType(opNode);
	return -1;
}

static int
normalizeComparison(pnode_t *opNode)
{
	pnode_t *attNode = pnLeft(opNode);
	int attIndex;

	switch (makeAttnameNode(attNode)) {
	case 0:
		break;
	case 1:
		return normalizeComparisonWithNonStdAtt(opNode);
	default:
		return -1;
	}

	attIndex = attNode->attr_flag;
	switch (attIndex) {
	case POSIX_LOG_ENTRY_DATA:
		return normalizeAttData(opNode);
	case POSIX_LOG_ENTRY_RECID:
		return normalizeAttRecid(opNode);
	case POSIX_LOG_ENTRY_SIZE:
		return normalizeAttSize(opNode);
	case POSIX_LOG_ENTRY_FORMAT:
		return normalizeAttFormat(opNode);
	case POSIX_LOG_ENTRY_EVENT_TYPE:
		return normalizeAttEventType(opNode); 
	case POSIX_LOG_ENTRY_FACILITY:
		return normalizeAttFacility(opNode);
	case POSIX_LOG_ENTRY_SEVERITY:
		return normalizeAttSeverity(opNode);
	case POSIX_LOG_ENTRY_UID:
		return normalizeAttUid(opNode);
	case POSIX_LOG_ENTRY_GID:
		return normalizeAttGid(opNode);
	case POSIX_LOG_ENTRY_PID:
	case POSIX_LOG_ENTRY_PGRP:	/* sic */
		return normalizeAttPid(opNode);
	case POSIX_LOG_ENTRY_TIME:
		return normalizeAttTime(opNode);
	case POSIX_LOG_ENTRY_FLAGS:
		return normalizeAttFlags(opNode);
	case POSIX_LOG_ENTRY_THREAD:
		return normalizeAttThread(opNode);
	case POSIX_LOG_ENTRY_PROCESSOR:
		return normalizeAttProcessor(opNode); 
	case EVL_ENTRY_AGE:
		return normalizeAttAge(opNode);
	case EVL_ENTRY_HOST:
		return normalizeAttHost(opNode);
	}
	/* NOTREACHED */
	return -1;
}

/*
 * Note: We depend on the grammar parser to give us a syntactically correct
 * tree -- e.g., nodes for binary operators have non-null left and right
 * children.
 * 0 = success, -1 = failure.
 */
int
_evlQNormalizeTree(pnode_t *root)
{
	if (! root) {
		/* Null query matches everything. */
		return 0;
	}

	switch (root->node_type) {
	case nt_name:
		/*
		 * In this context, the name must be an attribute name.
		 * Rewrite this node as an nt_attname node, if possible.
		 */
		return (makeAttnameNode(root) == -1 ? -1 : 0);
		break;
	case nt_attname:
	case nt_nsaname:
		/* Really an internal error */
		semanticError(
			"nt_attname/nt_nsaname node %s should already be normalized.",
			root->u_att.attr_name);
		return -1;
	case nt_val:
	case nt_string:
		/* Nothing to do. */
		break;
	case nt_op:
	    {
		long op = root->attr_flag;
		switch (op) {
		case '!':
			return _evlQNormalizeTree(pnLeft(root));
		case AND:
		case OR:
			if (_evlQNormalizeTree(pnLeft(root)) < 0) {
				return -1;
			}
			return _evlQNormalizeTree(pnRight(root));
		default:
			return normalizeComparison(root);
		}
	    }
		/* NOTREACHED */
	    	break;
	default: /* keep gcc happy */;
	}
	return 0;
}
