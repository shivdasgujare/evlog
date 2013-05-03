/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2002
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_template.h"

/*
 * Return a malloc-ed string containing the number and/or name of the
 * specified facility:
 * - If it's a POSIX-standard facility, return its symbol -- e.g., "LOG_KERN".
 * - If it's a non-POSIX facility we know about, return its number, with its
 *	name included as a comment -- e.g., "87 |* VolMgr *|".
 * - Otherwise, just return its number.
 */
static char *
getFacilityId(posix_log_facility_t fac)
{
	char facName[POSIX_LOG_MEMSTR_MAXLEN];
	char facId[POSIX_LOG_MEMSTR_MAXLEN + 100];

	_evlGetNameByValue(_evlPosixFacilities, fac, facName, sizeof(facName), "");
	if (strcmp(facName, "") != 0) {
		snprintf(facId, sizeof(facId), "LOG_%s", facName);
	} else if (posix_log_factostr(fac, facName, POSIX_LOG_MEMSTR_MAXLEN) == 0) {
		snprintf(facId, sizeof(facId), "%u /* %s */", fac, facName);
	} else {
		snprintf(facId, sizeof(facId), "%u", fac);
	}
	return (strdup(facId));
}

/*
 * Return a malloc-ed string that specifies the dimension for attribute att.
 */
static char *
getDimensionString(evlattribute_t *att, const char *prefix)
{
	char buf[100];
	tmpl_dimension_t *dim = att->ta_dimension;

	assert(dim != NULL);
	switch (dim->td_type) {
	case TMPL_DIM_CONST:
		snprintf(buf, sizeof(buf), "%d", dim->td_dimension);
		return (strdup(buf));
	case TMPL_DIM_REST:
		return strdup("_R_");
	case TMPL_DIM_ATTR:
	case TMPL_DIM_ATTNAME:
	    {
		const char *attName;
		char *scopedName;
		int len;
		
		if (dim->td_type == TMPL_DIM_ATTR) {
			attName = dim->u.u_attribute->ta_name;
		} else {
			attName = dim->u.u_attname;
		}
		len = strlen(prefix) + + strlen(attName) + 1;
		scopedName = (char*) malloc(len);
		assert(scopedName != NULL);
		snprintf(scopedName, len, "%s%s", prefix, attName);
		return scopedName;
	    }
	}

	/* Shouldn't get here. */
	assert(0);
	return NULL;
}

static const char *
typeName(int ty)
{
	return _evlTmplTypeInfo[ty].ti_name;
}

void
notSupported(const char *feature)
{
	fprintf(stderr, "Feature not supported by evltc -f: %s\n", feature);
}

/*
 * For each non-const attribute in template t, print the appropriate args
 * in the call to evl_log_write().
 */
static void
printAttArgs(const evltemplate_t *t, const char *prefix)
{
	evl_listnode_t *head = t->tm_attributes, *end, *p;
	const evltemplate_t *st;	/* struct attribute's template */

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		evlattribute_t *att = (evlattribute_t*) p->li_data;
		int ty;

		if (isConstAtt(att)) {
			continue;
		}
		if (isBitField(att)) {
			notSupported("bit-fields");
			continue;
		}

		ty = evlatt_gettype(att);
		if (isArray(att)) {
			char *dimString;

			dimString = getDimensionString(att, prefix);
			if (ty == TY_STRUCT) {
				/* Array of structs */
				st = att->ta_type->u.st_template;
				if ((st->tm_flags & TMPL_TF_ALIGNED) != 0) {
					/* Array of aligned structs */
					printf(
"\t\"char[]\",\t%s*sizeof(struct %s),\t%s%s,\n",
						dimString, st->tm_name, prefix,
						att->ta_name);
				} else {
					notSupported("arrays of unaligned structs");
				}
			} else {
				/* Array of scalars or strings */
				printf(
"\t\"%s[]\",\t%s,\t%s%s,\n",
					typeName(ty), dimString, prefix,
					att->ta_name);
			}

			free(dimString);
		} else {
			/* Not an array */
			if (ty == TY_STRUCT) {
				st = att->ta_type->u.st_template;
				if ((st->tm_flags & TMPL_TF_ALIGNED) != 0) {
					printf(
"\t\"char[]\",\tsizeof(struct %s),\t%s%s,\n",
						st->tm_name, prefix,
						att->ta_name);
				} else {
					/* Unaligned struct.  Print all its members. */
					size_t newPrefixSize = strlen(prefix)
						+ strlen(att->ta_name) + 1;
					char *newPrefix = (char*)
						malloc(newPrefixSize);
					assert(newPrefix != NULL);
					snprintf(newPrefix, newPrefixSize, "%s%s.", prefix,
						att->ta_name);
					printAttArgs(st, newPrefix);
					free(newPrefix);
				}
			} else {
				/* Not an array or struct */
				printf(
"\t\"%s\",\t%s%s,\n",
					typeName(ty), prefix, att->ta_name);
			}
		}
	}
}

/*
 * Print a sample evl_log_write call that might be used to log an event
 * matching template t.  If t is not an event-record template, do nothing.
 */
void
printFuncCall(const evltemplate_t *t)
{
	int evtype;
	posix_log_facility_t fac;
	char *facId;
	int alignedEvrec;
	char recStructName[100];

	if (t->tm_header.th_type != TMPL_TH_EVLOG) {
		return;
	}

	evtype = t->tm_header.u.u_evl.evl_event_type;
	fac = t->tm_header.u.u_evl.evl_facility;
	facId = getFacilityId(fac);
	alignedEvrec = ((t->tm_flags & TMPL_TF_ALIGNED) != 0);

	if (alignedEvrec) {
		/*
		 * The whole variable portion of the record is one C struct.
		 * But we don't know what its name is, so we make one up.
		 */
		snprintf(recStructName, sizeof(recStructName), "rec%d_%d", evtype, fac);
		printf(
"struct %s { /* record's attributes go here */ } rec;\n",
			recStructName);
	}

	printf(
"evl_log_write(%s, %d /* 0x%x */, severity, flags,\n",
		facId, evtype, evtype);
	if (alignedEvrec) {
		printf(
"\t\"char[]\",\tsizeof(struct %s),\t&rec,\n",
			recStructName);
	} else {
		printAttArgs(t, "");
	}
	printf("\t\"endofdata\");\n");

	free(facId);
}
