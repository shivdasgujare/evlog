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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
/* #include "scanner.h" */
#include "evl_list.h"
#include "evl_template.h"
#include "evl_util.h"

tmpl_base_type_t _evlGetTypeFromConversion(struct evl_parsed_format *pf,
	int promote, int signedOnly);

/*
 * fmt points to the character after the % in a printf-style conversion spec.
 * Parse the spec into its components and store them in *pf.  Returns -1 if
 * an error is detected, or 0 otherwise.
 *
 * Only 'l', 'L', 'h', 'z', 'j', and 't' are recognized as modifier characters.
 * Also 'Z' for printk.
 * There is no check here for the validity of the conversion letter or of the
 * modifier letters.  That's up to the caller.  (_evlGetTypeFromConversion()
 * can be used to do this check.)
 */
int
_evlParseFmtConvSpec(const char *fmt, struct evl_parsed_format *pf)
{
	static char *flags = "-+ #0'[";
	unsigned long width = 0;
	unsigned long precision = 0;
	const char *f;
	const char *c;
	const char *modifier;
	int nflags = 0, mfwidth = 0;

	for (f = fmt; strchr(flags, *f); f++) {
		/* *f is a flag.  Make sure there are no duplicates. */
		for (c = fmt; c < f; c++) {
			if (*c == *f) {
				return -1;
			}
		}
	}
	nflags = f - fmt;

	/* f points to the character after the flags. */
	width = strtoul(f, (char**) &c, 10);

	/* c points to the character after the width. */
	f = c;
	if (*f == '.') {
		f++;
		precision = strtoul(f, (char**) &c, 10);
		if (width == 0 && c == f) {
			/* Just the period, no numbers. */
			return -1;
		}
		f = c;
	}

	/* f points to the letter(s) at the end of the conversion spec. */
	modifier = f;
	while (strchr("hjlLtzZ", *f)) {
		f++;
	}
	mfwidth = f - modifier;
	if (mfwidth > 2) {
		/* Too many modifier letters. */
		return -1;
	}

	/* f had better point to the conversion letter. */
	if (!isalpha(*f)) {
		return -1;
	}

	pf->fm_width = width;
	pf->fm_precision = precision;
	pf->fm_length = 1 + (f - fmt);
	pf->fm_conversion = *f;
	if (nflags > 0) {
		(void) strncpy(pf->fm_flags, fmt, nflags);
	}
	pf->fm_flags[nflags] = '\0';
	pf->fm_array = (strchr(pf->fm_flags, '[') != NULL);
	if (mfwidth > 0) {
		(void) strncpy(pf->fm_modifier, modifier, mfwidth);
	}
	pf->fm_modifier[mfwidth] = '\0';
	return 0;
}

#define allocFormatSegment _evlAllocFormatSegment

evl_fmt_segment_t *
_evlAllocFormatSegment()
{
	evl_fmt_segment_t *seg = (evl_fmt_segment_t*)
		malloc(sizeof(evl_fmt_segment_t));
	assert(seg != NULL);
	return seg;
}

void
_evlFreeParsedFormat(evl_list_t *pf)
{
	evl_listnode_t *head, *end, *p;
	for (head=pf, p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		evl_fmt_segment_t *fs = (evl_fmt_segment_t*) p->li_data;
		if (fs->fs_type == EVL_FS_ATTNAME) {
			free(fs->u.fs_attname);
		} else if (fs->fs_type == EVL_FS_PRINTFCONV) {
			free(fs->u.fs_conversion);
			free(fs->fs_userfmt);	/* May be null */
		}
	}
	_evlFreeList(pf, 1);

}

/*
 * NOTE: Unlike other make*Segment functions, this DOES malloc a string to
 * hold the assembled message.
 */
static evl_fmt_segment_t *
makeErrorSegment(const char *attrSpec, const char *msg)
{
	evl_fmt_segment_t *seg;
	int mlen = strlen(attrSpec) + strlen(msg) + sizeof(": ") + 1;
	char *fullMsg = (char *) malloc(mlen);
	assert(fullMsg != NULL);
	snprintf(fullMsg, mlen, "%s: %s", msg, attrSpec);

	seg = allocFormatSegment();
	seg->fs_type = EVL_FS_ERROR;
	seg->u.fs_string = fullMsg;
	return seg;
}

/* NOTE: This does NOT strdup the string. */
static evl_fmt_segment_t *
makeStringSegment(char *s)
{
	evl_fmt_segment_t *seg = allocFormatSegment();
	seg->fs_type = EVL_FS_STRING;
	seg->u.fs_string = s;
	return seg;
}

/*
 * Parse the indicated conversion spec into *pf and do some validity checks.
 */
int
_evlValidateAttrFmt(const char *fmt, struct evl_parsed_format *pf, int maxWidth)
{
	char *m;
	if (_evlParseFmtConvSpec(fmt+1, pf) < 0) {
		return -1;
	}
	if (pf->fm_length == 0 || pf->fm_length != strlen(fmt+1)) {
		/* Possibly some extra stuff beyond the format */
		return -1;
	}
	if (pf->fm_width > maxWidth || pf->fm_precision > maxWidth) {
		/* Apparently some obnoxious format such as "%400d" */
		return -1;
	}

	if (_evlGetTypeFromConversion(pf, 1, 1) == TY_NONE) {
		/* Bad conversion and/or modifier/conversion combo */
		return -1;
	}

	return 0;
}

/*
 * NOTE: This does NOT strdup the string, unless it's a non-standard attribute
 * name.
 */
static evl_fmt_segment_t *
makeAttrSegment(char *attrSpec, int allowNonStdAtts)
{
	evl_fmt_segment_t *seg;
	int memberIndex;
	char *attrName, *attrFmt, *colon, *nsaName;
	struct evl_parsed_format pf;
	evl_fmt_segment_type_t segType;

	attrName = attrSpec;
	colon = strchr(attrSpec, ':');

	if (colon) {
		/*
		 * The attrSpec is "name:fmt".  First replace the colon
		 * with a null, so we can look up the attribute name.
		 */
		*colon = '\0';
	}

	memberIndex = _evlGetValueByName(_evlAttributes, attrName, -1);
	if (memberIndex == -1) {
		if (!strcmp(attrName, "data")) {
			memberIndex = POSIX_LOG_ENTRY_DATA;
			if (colon) {
				return makeErrorSegment(attrName,
					"Cannot specify alternate format for data");
			}
			segType = EVL_FS_MEMBER;
		} else if (allowNonStdAtts || !strcmp(attrName, "host")) {
			segType = EVL_FS_ATTNAME;
			nsaName = strdup(attrName);
			assert(nsaName != NULL);
		} else {
			return makeErrorSegment(attrName, "Unknown attribute");
		}
	} else {
		segType = EVL_FS_MEMBER;
	}

	if (colon) {
		/*
		 * We don't need the name any more.  Replace the null with
		 * a '%' to give us "name%fmt".  All this to avoid a malloc/free
		 * or two.
		 */
		*colon = '%';
		attrFmt = colon;
		if (!strcmp(attrFmt, "%printf")
		    || !strcmp(attrFmt, "%printk")) {
			/*
			 * %name:printk% or %name:printf%
			 * Advance past the colon -- er, percent.
			 */
			attrFmt++;
		} else if (_evlValidateAttrFmt(attrFmt, &pf,
		    POSIX_LOG_MEMSTR_MAXLEN-1) < 0) {
			*colon = ':';
			return makeErrorSegment(attrSpec, "Error in format");
		}
	} else {
		attrFmt = NULL;
	}

	seg = allocFormatSegment();
	seg->fs_type = segType;
	switch (segType) {
	case EVL_FS_MEMBER:
		seg->u2.fs_member = memberIndex;
		break;
	case EVL_FS_ATTNAME:
		seg->u.fs_attname = nsaName;
		seg->u2.fs_attribute = NULL;
		break;
	default:
		printf("default switch case %s\n", __FUNCTION__);
		break;
	}
	seg->fs_userfmt = attrFmt;
	if (attrFmt
	    && *attrFmt == '%'	/* no pf for 'printf' and 'printk' */
	    && (pf.fm_conversion == 's' || pf.fm_conversion == 'S')) {
		seg->fs_stringfmt = 1;
	} else {
		seg->fs_stringfmt = 0;
	}
	return seg;
}

/*
 * Given a format string such as 
 *	FAC=%facility%, ET=%event_type%
 *	^             ^                ^
 * c points to one of the indicated locations.  Collect the string up to
 * the next % that introduces an attribute name (e.g., "FAC="), make a
 * string-segment node out of it, and add it to the list.  Treat %% as
 * literal %.  Return a pointer to the beginning of the attribute name,
 * if any, or to the terminating null.
 *
 * Upon return, the contents of the string preceding the returned pointer
 * are undefined.
 */
static char *
collectStringSegment(char *c, evl_list_t **list)
{
	/*
	 * t keeps up with c until they reach a %%.  Then c moves ahead one and
	 * characters are copied left from c to t (thereby converting %% to %).
	 * "token" points to the string we're collecting.
	 */
	char *token = c;
	char *t = c;

	if (*c == '\0') {
		return c;
	}

	for (;;) {
		while (*c != '%' && *c != '\0') {
			if (c > t) {
				*t = *c;
			}
			c++;
			t++;
		}
		if (*c == '\0') {
			/* End of whole format string */
			if (t > token) {
				*t = '\0';
				*list = _evlAppendToList(*list,
					makeStringSegment(token));
			}
			return c;
		} else {
			/* *c = '%' */
			if (c[1] == '%') {
				*t++ = '%';
				c += 2;
				continue;
			}

			/* Lone '%' */
			if (t > token) {
				*t = '\0';
				*list = _evlAppendToList(*list,
					makeStringSegment(token));
			}
			return c+1;
		}
	}
	/*NOTREACHED*/
	return c;
}

/*
 * c points either to the terminating null in the format string, or to the
 * beginning of an attribute name.  Collect the attribute name, make an
 * attribute segment out of it, and add the segment to the list.  If there's
 * an error, append an error segment instead and return NULL.  If
 * allowNonStdAtts is non-zero, we allow non-standard attribute names.
 */
static char *
collectAttrSegment(char *c, evl_list_t **list, int allowNonStdAtts)
{
	char *token = c;
	evl_fmt_segment_t *seg;

	if (*c == '\0') {
		return c;
	}
	c = strchr(c, '%');
	if (!c) {
		*list = _evlAppendToList(*list, makeErrorSegment(token,
			"Unterminated attribute spec"));
		return NULL;
	}
	*c++ = '\0';
	seg = makeAttrSegment(token, allowNonStdAtts);
	*list = _evlAppendToList(*list, seg);
	if (seg->fs_type == EVL_FS_ERROR) {
		return NULL;
	}
	return c;
}

static char *getFmtErrorMsg(evl_list_t *fmtSegList);

/*
 * Create and return a list of format segments from the indicated format string.
 * On error, returns NULL; and if errMsg is not NULL, *errMsg points to a
 * malloced buffer that contains the reason.  If allowNonStdAtts is non-zero,
 * we accept non-standard attribute names.
 */
evl_list_t *
_evlParseFormat(char *fmt, int allowNonStdAtts, char **errMsg)
{
	evl_list_t *list = NULL;
	char *sp;

	for (sp = fmt; *sp; ) {
		sp = collectStringSegment(sp, &list);
		sp = collectAttrSegment(sp, &list, allowNonStdAtts);
		if (!sp) {
			/* Bad attribute spec. List ends in an error segment. */
			char *msg;
			if (errMsg) {
				msg = getFmtErrorMsg(list);
				*errMsg = msg;
			}
			_evlFreeParsedFormat(list);
			return NULL;
		}
	}
	if (!list) {
		if (errMsg) {
			*errMsg = strdup("Empty format specification");
		}
	}
	return list;
}

/*
 * head heads a list of string and attribute segments.  Figure out how big a
 * formatted string might be, and allocate a buffer that's big enough to hold
 * that.  For standard attributes, assume that no formatted value exceeds
 * POSIX_LOG_MEMSTR_MAXLEN in length, except that the data attribute can
 * be up to _evlGetMaxDumpLen().  Return a pointer to the buffer, with
 * the buffer size in *bufsz.
 */
char *
_evlAllocateFmtBuf(const evl_list_t *head, size_t *bufsz)
{
	size_t size = 0;
	const evl_listnode_t *p = head;
	const evl_fmt_segment_t *fs;
	char *buf;

	if (!head) {
		return NULL;
	}
	do {
		fs = (const evl_fmt_segment_t *) p->li_data;

		switch (fs->fs_type) {
		case EVL_FS_STRING:
			size += strlen(fs->u.fs_string);
			break;
		case EVL_FS_MEMBER:
			if (fs->u2.fs_member == POSIX_LOG_ENTRY_DATA) {
				size += _evlGetMaxDumpLen();
			} else {
				size += POSIX_LOG_MEMSTR_MAXLEN;
			}
			break;
		case EVL_FS_ATTNAME:
			/*
			 * TODO: _evlAllocateFmtBuf for non-standard attribute.
			 *
			 * Non-standard attribute name.  We don't really have
			 * any idea what this attribute might balloon into
			 * when formatted.  Guess high.  (But even this is
			 * not guaranteed to be high enough.)
			 */
			size += _evlGetMaxDumpLen();
			break;
		default:
			printf("default switch case %s\n", __FUNCTION__);
			break;
		}
		p = p->li_next;
	} while (p != head);

	if (size == 0) {
		return NULL;
	}
	buf = (char *) malloc(size+1);
	assert(buf != NULL);
	if (bufsz) {
		*bufsz = size+1;
	}
	return buf;
}

void
_evlSprintfMember(evl_fmt_buf_t *s, const char *fmt, int member,
	const struct posix_log_entry *entry)
{
	switch (member) {
	case POSIX_LOG_ENTRY_RECID:
		_evlBprintf(s, fmt, entry->log_recid);
		break;
	case POSIX_LOG_ENTRY_SIZE:
		_evlBprintf(s, fmt, entry->log_size);
		break;
	case POSIX_LOG_ENTRY_FORMAT:
		_evlBprintf(s, fmt, entry->log_format);
		break;
	case POSIX_LOG_ENTRY_EVENT_TYPE:
		_evlBprintf(s, fmt, entry->log_event_type);
		break;
	case POSIX_LOG_ENTRY_FACILITY:
		_evlBprintf(s, fmt, entry->log_facility);
		break;
	case POSIX_LOG_ENTRY_SEVERITY:
		_evlBprintf(s, fmt, entry->log_severity);
		break;
	case POSIX_LOG_ENTRY_UID:
		_evlBprintf(s, fmt, entry->log_uid);
		break;
	case POSIX_LOG_ENTRY_GID:
		_evlBprintf(s, fmt, entry->log_gid);
		break;
	case POSIX_LOG_ENTRY_PID:
		_evlBprintf(s, fmt, entry->log_pid);
		break;
	case POSIX_LOG_ENTRY_PGRP:
		_evlBprintf(s, fmt, entry->log_pgrp);
		break;
	case POSIX_LOG_ENTRY_TIME:
		_evlBprintf(s, fmt, entry->log_time.tv_sec);
		break;
	case POSIX_LOG_ENTRY_FLAGS:
		_evlBprintf(s, fmt, entry->log_flags);
		break;
	case POSIX_LOG_ENTRY_THREAD:
		_evlBprintf(s, fmt, entry->log_thread);
		break;
	case POSIX_LOG_ENTRY_PROCESSOR:
		_evlBprintf(s, fmt, entry->log_processor);
		break;
	default:
		/* Shouldn't reach here. */
		assert(0);
	}
}

/*
 * If there's a error segment in the list, return a pointer to the message.
 * Otherwise return NULL.  Note: returns NULL if the list is empty.
 */
static char *
getFmtErrorMsg(evl_list_t *fmtSegList)
{
	const evl_listnode_t *head = fmtSegList;
	const evl_listnode_t *p = head;
	const evl_fmt_segment_t *fs;

	if (!head) {
		return NULL;
	}
	do {
		fs = (const evl_fmt_segment_t *) p->li_data;
		if (fs->fs_type == EVL_FS_ERROR) {
			return fs->u.fs_string;
		}
		p = p->li_next;
	} while (p != head);
	return NULL;
}

/***** Parsing of printf-style format strings *****/

/*
 * Architecture-dependent types that correspond to %d modifiers that are
 * extensions in glibc printf().
 */
static tmpl_base_type_t typeof_size_t;	/* %zd */
static tmpl_base_type_t typeof_intmax_t;	/* %jd */
static tmpl_base_type_t typeof_ptrdiff_t;	/* %td */

/* Return a basic int type whose size is sz. */
static tmpl_base_type_t
computeIntType(size_t sz)
{
	if (sz == sizeof(int)) {
		return TY_INT;
	} else if (sz == sizeof(long)) {
		return TY_LONG;
	} else if (sz == sizeof(long long)) {
		return TY_LONGLONG;
	}

	assert(0);
	/*NOTREACHED*/
	return TY_INT;
}

static void
computeSpecialTypes(void)
{
	static int alreadyComputed = 0;
	if (alreadyComputed) {
		return;
	}
	typeof_size_t = computeIntType(sizeof(size_t));
	typeof_intmax_t = computeIntType(sizeof(intmax_t));
	typeof_ptrdiff_t = computeIntType(sizeof(ptrdiff_t));
	alreadyComputed = 1;
}

/* Handle %<mod>c.  We recognize only %c and %lc. */
static tmpl_base_type_t
adjustCharType(const char *mod, int promote)
{
	if (!strcmp(mod, "")) {
		return (promote ? TY_INT : TY_UCHAR);
	}
	if (!strcmp(mod, "l")) {
		return TY_WCHAR;
	}
	return TY_NONE;
}

/* Handle modifiers to %d and similar int conversions. */
static tmpl_base_type_t
adjustIntType(const char *mod, int promote)
{
	if (!strcmp(mod, "")) {
		return TY_INT;
	}
	if (!strcmp(mod, "h")) {
		return (promote ? TY_INT : TY_SHORT);
	}
	if (!strcmp(mod, "hh")) {
		return (promote ? TY_INT : TY_CHAR);
	}
	if (!strcmp(mod, "l")) {
		return TY_LONG;
	}
	if (!strcmp(mod, "ll")
	    || !strcmp(mod, "L") /* for printk */ ) {
		return TY_LONGLONG;
	}
	if (!strcmp(mod, "z")
	    || !strcmp(mod, "Z") /* for printk */ ) {
		computeSpecialTypes();
		return typeof_size_t;
	}
	if (!strcmp(mod, "j")) {
		computeSpecialTypes();
		return typeof_intmax_t;
	}
	if (!strcmp(mod, "t")) {
		computeSpecialTypes();
		return typeof_ptrdiff_t;
	}
	return TY_NONE;
}

static tmpl_base_type_t
adjustUintType(const char *mod, int promote)
{
	tmpl_base_type_t ty = adjustIntType(mod, promote);
	switch (ty) {
	case TY_CHAR:		return TY_UCHAR;
	case TY_SHORT:		return TY_USHORT;
	case TY_INT:		return TY_UINT;
	case TY_LONG:		return TY_ULONG;
	case TY_LONGLONG:	return TY_ULONGLONG;
	default:
		printf("default switch case %s\n", __FUNCTION__);
		break;
	}
	return ty;
}

/*
 * Handle modifiers to %f and similar double conversions.  The only
 * modifier we recognize is L.
 */
static tmpl_base_type_t
adjustDoubleType(const char *mod)
{
	if (!strcmp(mod, "")) {
		return TY_DOUBLE;
	}
	if (!strcmp(mod, "L")) {
		return TY_LDOUBLE;
	}
	return TY_NONE;
}

/* Handle %<mod>s.  We recognize only %s and %ls. */
static tmpl_base_type_t
adjustStringType(const char *mod)
{
	if (!strcmp(mod, "")) {
		return TY_STRING;
	}
	if (!strcmp(mod, "l")) {
		return TY_WSTRING;
	}
	return TY_NONE;
}

/* Handle %<mod>n.  Theoretically, we can get this via printk(). */
static tmpl_base_type_t
validateNConversion(const char *mod) {
	if (!strcmp(mod, "")
	    || !strcmp(mod, "l")
	    || !strcmp(mod, "ll")
	    || !strcmp(mod, "L")) {
		return TY_SPECIAL;
	}
	return TY_NONE;
}

/*
 * Given parsed conversion spec pf, return the type of value it converts.
 * If promote != 0, promote small integers to int.
 * If signedOnly != 0, return signed rather than unsigned types for %o, %u,
 *	%x, and %X.  Some callers count on this.
 * Return TY_NONE for an illegal modifier/conversion combo.
 */
tmpl_base_type_t
_evlGetTypeFromConversion(struct evl_parsed_format *pf, int promote,
	int signedOnly)
{
	const char *mod = pf->fm_modifier;

	switch (pf->fm_conversion) {
	case 'c':
		return adjustCharType(mod, promote);
	case 'd':
	case 'i':
		return adjustIntType(mod, promote);
	case 'o':
	case 'u':
	case 'x':
	case 'X':
		if (signedOnly) {
			return adjustIntType(mod, promote);
		} else {
			return adjustUintType(mod, promote);
		}
	case 'a':
	case 'A':
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
		return adjustDoubleType(mod);
	case 's':
		return adjustStringType(mod);
	case 'p':
		return (strlen(mod) == 0 ? TY_ADDRESS : TY_NONE);
	case 'C':
		return (strlen(mod) == 0 ? TY_WCHAR : TY_NONE);
	case 'S':
		return (strlen(mod) == 0 ? TY_WSTRING : TY_NONE);
	case 'n':
		return validateNConversion(mod);
	default:
		return TY_NONE;
	}
	/*NOTREACHED*/
}

/*
 * c points either to the terminating null in the format string, or to the
 * beginning of a printf conversion (e.g., to the '0' in "%02d").  Parse
 * the conversion, make a conversion segment out of it, and add the segment
 * to the list.
 *
 * If needStrings != 0, malloc a copy of the conversion spec's text (including
 * the leading '%'), and  point the fs_userfmt field of the conversion segment
 * at it.  Note that c[-1] could be a % or a null when we enter this function.
 *
 * Return a pointer to the character following the conversion spec.
 */
static char *
collectConversionSegment(char *c, evl_list_t **list, int needStrings)
{
	struct evl_parsed_format *pf;
	evl_fmt_segment_t *seg;
	int convLength;

	if (!c || *c == '\0') {
		return c;
	}

	pf = (struct evl_parsed_format*)
		malloc(sizeof(struct evl_parsed_format));
	assert(pf != NULL);

	if (_evlParseFmtConvSpec(c, pf) != 0) {
		free(pf);
		return NULL;
	}
	convLength = pf->fm_length;

	seg = allocFormatSegment();
	seg->fs_type = EVL_FS_PRINTFCONV;
	seg->u.fs_conversion = pf;
	seg->fs_stringfmt = (pf->fm_conversion == 's' ||
		pf->fm_conversion == 'S');
	if (needStrings) {
		/*
		 * Prepend the '%' and append the null, and store the
		 * conversion spec as seg->fs_userfmt.
		 */
		seg->fs_userfmt = (char*) malloc(convLength + 2);
		assert(seg->fs_userfmt != NULL);
		seg->fs_userfmt[0] = '%';
		(void) memcpy(seg->fs_userfmt + 1, c, convLength);
		seg->fs_userfmt[1 + convLength] = '\0';
	} else {
		seg->fs_userfmt = NULL;
	}
	*list = _evlAppendToList(*list, seg);
	return c + convLength;
}

/*
 * Advance c until it points to (1) the terminating null or (2) the first
 * character AFTER the '%' in a printf format conversion.  Skips over %%.
 * Returns the new value of c.
 */
static char *
skipStringSegment(char *c)
{
	if (!c) {
		return c;
	}
	for (;;) {
		while (*c != '%' && *c != '\0') {
			c++;
		}
		if (*c == '\0') {
			return c;
		}
		if (c[1] == '%') {
			/* Skip over %% */
			c += 2;
			continue;
		}
		return c+1;
	}
	/* NOTREACHED */
	return c;
}

evl_list_t *
_evlParsePrintfFormat(char *format, int needStrings, int *status)
{
	evl_list_t *list = NULL;
	char *sp;

	for (sp = format; *sp; ) {
		if (needStrings) {
			sp = collectStringSegment(sp, &list);
		} else {
			sp = skipStringSegment(sp);
		}
		sp = collectConversionSegment(sp, &list, needStrings);
		if (!sp) {
			/* Bad printf conversion. */
			_evlFreeParsedFormat(list);
			if (status) {
				*status = -1;
			}
			return NULL;
		}
	}
	if (status) {
		*status = 0;
	}
	return list;
}
