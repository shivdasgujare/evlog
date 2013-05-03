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
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_list.h"
#include "evl_util.h"
#include "evl_template.h"

extern tmpl_base_type_t _evlGetTypeFromConversion(struct evl_parsed_format *pf,
	int promote, int signedOnly);
extern char * _evlGetHostNameEx(int id);

/* Used for scanning the variable part of an event record */
typedef struct recbuf {
	const char	*b_next;
	const char	*b_end;
} recbuf_t;

#define MAXSEPLEN 20

/* Append one "name=value," to the buffer. */
static int
addAttr(const struct posix_log_entry *entry, int memberSelector,
	const char *memberName, char *buf, size_t buflen,
	const char *separator, size_t maxLineLen, int fmtFlags, size_t *totalSize,
	size_t *curLineLen, int lastAttr)
{
	char val[POSIX_LOG_MEMSTR_MAXLEN];
	char attr[sizeof("VeryLongAttrName=")+POSIX_LOG_MEMSTR_MAXLEN+MAXSEPLEN];
	int status;
	size_t attrLen;
	int needNewLine = 0;
	int nodeId;
	struct posix_log_entry rechdr;

	if (memberSelector == POSIX_LOG_ENTRY_PROCESSOR) {
		nodeId = entry->log_processor >> 16;
		memcpy(&rechdr, entry, sizeof(struct posix_log_entry));
		rechdr.log_processor &= 0x0000ffff;
		/* Get the attribute's value. */
		status = posix_log_memtostr(memberSelector, &rechdr, val,
									POSIX_LOG_MEMSTR_MAXLEN);
	} else {

		/* Get the attribute's value. */
		status = posix_log_memtostr(memberSelector, entry, val,
									POSIX_LOG_MEMSTR_MAXLEN);
	}
	if (status != 0) {
		return status;
	}

	/* Build "name=value,". */
	if ((fmtFlags & EVL_COMPACT) != 0) {
		(void) strcpy(attr, val);
		if (memberSelector == POSIX_LOG_ENTRY_PROCESSOR) {
			(void) strcat(attr, separator);
			(void) strcat(attr, _evlGetHostNameEx(nodeId));
		}
	} else {
		(void) strcpy(attr, memberName);
		(void) strcat(attr, "=");
		(void) strcat(attr, val);
		if (memberSelector == POSIX_LOG_ENTRY_PROCESSOR) {
			(void) strcat(attr, separator);
			(void) strcat(attr, "host");
			(void) strcat(attr, "=");
			(void) strcat(attr, _evlGetHostNameEx(nodeId));
		}
	}

	if (!lastAttr && separator) {
		(void) strcat(attr, separator);
	}
	attrLen = strlen(attr);

	/* Do we need to start a new line? */
	if (maxLineLen == 0) {
		/* Special case */
		needNewLine = 0;
	} else if (*curLineLen > 0 && *curLineLen + attrLen > maxLineLen) {
		needNewLine = 1;
	}
	if (separator && strchr(separator, '\n')) {
		/*
		 * The separator contains a newline.  Don't add any more.
		 * The value of curLineLen will be wrong but irrelevant.
		 * If this becomes an issue, we can always compute the
		 * line length each time via
		 *	char *nl = strrchr(buf, '\n');
		 *	curLineLen = (nl ? strlen(nl+1) : strlen(buf));
		 */
		needNewLine = 0;
	}

	if (needNewLine) {
		*curLineLen = attrLen;
	} else {
		*curLineLen += attrLen;
	}

	/*
	 * Move buf past the text we've added previously, to speed up
	 * the calls to strcat.
	 */
	buf += *totalSize;

	*totalSize += needNewLine + attrLen;
	if (*totalSize + 1 > buflen) {
		/* No room for what's there plus what we want to add plus \0 */
		return EMSGSIZE;
	}

	/* Now actually append the data to the buffer. */
	if (needNewLine) {
		(void) strcat(buf, "\n");
	}
	(void) strcat(buf, attr);
	return 0;
}

int
evl_format_evrec_fixed(const struct posix_log_entry *entry, char *buf,
	size_t buflen, size_t *reqlen, const char *separator, size_t linelen,
	int fmt_flags)
{
	const struct _evlNvPair *nv;
	size_t curLineLen = 0;
	size_t totalSize = 0;
	int status = 0;

	if (reqlen) {
		/* Set this in case we have to bail early. */
		*reqlen = 0;
	}

	if (fmt_flags != 0 && fmt_flags != EVL_COMPACT) {
		return EINVAL;
	}

	if (!entry) {
		return EINVAL;
	}

	if (separator && strlen(separator) > MAXSEPLEN) {
		return EINVAL;
	}

	if (buf && buflen != 0) {
		buf[0] = '\0';
	} else {
		/*
		 * Can't return the string, but still try to count characters.
		 * But no use counting if we can't return the count.
		 */
		status = EINVAL;
		if (!reqlen) {
			return status;
		}
	}

	for (nv = _evlAttributes; nv->nv_name; nv++) {
		int lastAttr = ((nv+1)->nv_name == 0);
		int result = addAttr(entry, nv->nv_value, nv->nv_name,
			buf, buflen, separator, linelen, fmt_flags,
			&totalSize, &curLineLen, lastAttr);
		if (result != 0) {
			/*
			 * If result is EMSGSIZE, we've run out of space,
			 * but keep counting.
			 */
			status = result;
			if (!reqlen) {
				return status;
			}
			if (status != EMSGSIZE) {
				*reqlen = 0;
				return status;
			}
		}
	}

	if (reqlen) {
		*reqlen = totalSize + 1;
	}
	return status;
}

/***** Much ado about dump format *****/

static char *hexDigits = "0123456789ABCDEF";

static char *
format8bytes(const char *dp, const char *dend, char *bp)
{
	char *oldbp = bp;
	int i;

	for (i = 1; i <= 8; i++, dp++, bp += 3) {
		if (dp <= dend) {
			bp[0] = hexDigits[(*dp >> 4) & 0xF];
			bp[1] = hexDigits[*dp & 0xF];
		} else {
			bp[0] = ' ';
			bp[1] = ' ';
		}
		bp[2] = ' ';
	}
	return oldbp + (8*3);
}

static char *
format8chars(const char *dp, const char *dend, char *bp)
{
	char *oldbp = bp;
	int i;

	for (i = 1; i <= 8; i++, dp++, bp++) {
		if (dp > dend) {
			*bp = ' ';
		} else if (isprint(*dp)) {
			*bp = *dp;
		} else {
			*bp = '.';
		}
	}
	return oldbp + 8;
}

/*
 * formatDumpLine: Formats 1 line of data for _evlDumpBytes().
 * Format of a line of dump format:
 * cols 0-7: 8-digit hex offset from beginning of data
 * col 8: space
 * cols 9-32: 8 bytes of data: digit,digit,space
 * col 33: space
 * cols 34-57: 8 bytes of data: digit,digit,space
 * cols 58-59: pipe, space
 * cols 60-67: 8 bytes of data, formatted as 8 characters
 * col 68: space
 * cols 69-76: 8 bytes of data, formatted as 8 characters
 */
static char *
formatDumpLine(const char *dp, const char *dend, size_t offset, char *bp, size_t size)
{
#define BYTES_PER_LINE 16
#define DUMPLINELEN 77	/* (8+1+(8*3)+1+(8*3)+2+8+1+8) */
	snprintf(bp, size, "%08X", offset);
	bp += 8;
	*bp++ = ' ';
	bp = format8bytes(dp, dend, bp);
	*bp++ = ' ';
	bp = format8bytes(dp+8, dend, bp);
	*bp++ = '|';
	*bp++ = ' ';
	bp = format8chars(dp, dend, bp);
	*bp++ = ' ';
	bp = format8chars(dp+8, dend, bp);
	return bp;
}

/*
 * Format nBytes bytes of data from the 'data' buffer into buf, whose length
 * is buflen bytes.  If buflen is insufficient to hold the formatted data,
 * return EMSGSIZE.  In any case, if reqlen is not NULL, set *reqlen to the
 * number of bytes required.
 */
int
_evlDumpBytes(const void *data, size_t nBytes, char *buf, size_t buflen,
	size_t *reqlen)
{
	int nLines;
	size_t reqBufLen;
	const char *dbase = (const char *) data;
	const char *dend = dbase + (nBytes -1);
	const char *dp;
	char *bp;

	if (reqlen) {
		*reqlen = 0;
	}
	if (!data || nBytes == 0) {
		return EINVAL;
	}
	nLines = (nBytes + BYTES_PER_LINE - 1)/BYTES_PER_LINE;

	/*
	 * How big does buf need to be?  Count the terminating newline
	 * (or null) for each line.  Demand space for a full final line
	 * even if we don't really need every last byte.
	 */
	reqBufLen = nLines * (DUMPLINELEN + 1);
	if (reqlen) {
		*reqlen = reqBufLen;
	}
	if (buflen < reqBufLen) {
		return EMSGSIZE;
	}
	if (!buf) {
		return EINVAL;
	}

	for (dp = dbase, bp = buf; dp <= dend; dp += BYTES_PER_LINE) {
		bp = formatDumpLine(dp, dend, dp-dbase, bp, DUMPLINELEN + 1);
		*bp++ = '\n';
	}

	/* Replace the final new line with a null. */
		bp[-1] = '\0';
	return 0;
}

/*
 * Like _evlDumpBytes(), but if the buffer isn't long enough, we fill it,
 * null-terminate it, and discard the rest.  We do this by malloc-ing a
 * sufficiently large buffer, running _evlDumpBytes(), and copying what we
 * can into the caller's buffer.  As with _evlDumpBytes(), *reqlen is set
 * to the needed buffer size.
 */
int
_evlDumpBytesForce(const void *data, size_t nBytes, char *buf, size_t buflen,
	size_t *reqlen)
{
	size_t myReqlen = 0;
	size_t *pReqlen;
	int status;

	/* _evlDumpBytes doesn't check this 'til too late for us. */
	if (!buf) {
		return EINVAL;
	}

	if (reqlen) {
		pReqlen = reqlen;
	} else {
		pReqlen = &myReqlen;
	}
	status = _evlDumpBytes(data, nBytes, buf, buflen, pReqlen);
	if (status == EMSGSIZE) {
		char *mybuf = (char*) malloc(*pReqlen);
		assert(mybuf != NULL);
		status = _evlDumpBytes(data, nBytes, mybuf, *pReqlen, NULL);
		assert(status == 0);
		(void) memcpy(buf, mybuf, buflen);
		buf[buflen-1] = '\0';
		free(mybuf);
	}
	return status;
}

/*
 * Like _evlDumpBytes(), but this writes the dump to the indicated stream.
 * Note that this prints a newline at the end of every line, including the
 * last.
 */
int
_evlDumpBytesToFile(const void *data, size_t nBytes, FILE *f)
{
	const char *dbase = (const char *) data;
	const char *dend = dbase + (nBytes -1);
	const char *dp;
	char line[DUMPLINELEN+1];
	char *eol;

	if (!data || nBytes == 0) {
		return EINVAL;
	}

	for (dp = dbase; dp <= dend; dp += BYTES_PER_LINE) {
		eol = formatDumpLine(dp, dend, dp-dbase, line, sizeof(line));
		*eol = '\0';
		fprintf(f, "%s\n", line);
	}

	return 0;
}

/*
 * Returns the size of the buffer that would be required to hold a dump
 * of the largest possible variable portion of an event record.
 */
size_t
_evlGetMaxDumpLen()
{
	static size_t nLines =
		(POSIX_LOG_ENTRY_MAXLEN+BYTES_PER_LINE-1)/BYTES_PER_LINE;
	return nLines*(DUMPLINELEN+1);
}

/*
 * If there is a template for event record (entry, buf), get it and make
 * sure it is populated with that record.  Returns a pointer to the cloned,
 * populated template on success, or NULL on failure.
 */
static evltemplate_t *
getPopulatedTemplate(const struct posix_log_entry *entry, const char *buf)
{
	int status;
	evltemplate_t *tmpl;

	status = evl_gettemplate(entry, buf, &tmpl);
	if (status == ENOENT) {
		/* No such template. */
		return NULL;
	}
	assert(status == 0);

	status = evl_populatetemplate(tmpl, entry, buf);
	assert(status == 0);
	return tmpl;
}

int
evl_format_evrec_variable(const struct posix_log_entry *entry,
	const void *var_buf, char *buf, size_t buflen, size_t *reqlen)
{
	evltemplate_t *tmpl;
	if (reqlen) {
		*reqlen = 0;
	}
	if (!entry) {
		return EINVAL;
	}

	tmpl = getPopulatedTemplate(entry, var_buf);
	if (tmpl) {
		/* Format according to this event type's template. */
		int status = _evlSpecialFormatEvrec(entry, var_buf,
			tmpl, tmpl->tm_parsed_format, buf, buflen, reqlen);
		evl_releasetemplate(tmpl);
		return status;
	}

	/* No template.  Format as best we can. */
	switch (entry->log_format) {
	case POSIX_LOG_NODATA:
		if (reqlen) {
			*reqlen = 1;
		}
		if (buflen < 1) {
			return EMSGSIZE;
		}
		buf[0] = '\0';
		return 0;
	case POSIX_LOG_STRING:
		if (reqlen) {
			*reqlen = entry->log_size;
		}
		if (buflen < entry->log_size) {
			return EMSGSIZE;
		}
		(void) strcpy(buf, (const char*) var_buf);
		return 0;
	case POSIX_LOG_BINARY:
		return _evlDumpBytes(var_buf, entry->log_size, buf,
			buflen, reqlen);
	case POSIX_LOG_PRINTF:
		return _evlFormatPrintfRec(var_buf, entry->log_size, buf,
			buflen, reqlen,
			(entry->log_flags & EVL_PRINTK) != 0);
	}

	/* Unknown data format */
	return EINVAL;
}

/*
 * Format the event record specified by entry and var_buf into buf (whose
 * length is buflen bytes) according to the specified format.
 * format is a string such as "Record ID %recid%: time stamp = %time:d%".
 *
 * The -S/-F options of evlview do this same stuff, but they don't
 * call this function, because it's not efficient to do all this setup
 * and shutdown for every record.  (Also, if _evlParseFormat() rejects the
 * specified format, this function doesn't tell you WHY.)
 */
int
evl_format_evrec_sprintf(const struct posix_log_entry *entry,
	const void *var_buf, const char *format, char *buf,
	size_t buflen, size_t *reqlen)
{
	evl_list_t *parsedFormat;
	char *myformat;
	int status = 0;
	evl_listnode_t *head, *end, *p;
	int needTmpl;

	if (reqlen) {
		*reqlen = 0;
	}
	if (!entry || !format) {
		return EINVAL;
	}

	/*
	 * Make our own copy of the format spec, since we plug it with
	 * nulls and such.
	 */
	myformat = strdup(format);

	/*
	 * Parse the format specification.  For now, we accept non-standard
	 * attribute names.
	 */
	parsedFormat = _evlParseFormat(myformat, 1, NULL);
	if (!parsedFormat) {
		free(myformat);
		return EBADMSG;
	}

	/*
	 * If there are any non-standard attribute names, then we need to
	 * read and populate the template for this event record.  Then we
	 * must ensure that this template provides those attributes.
	 */
	needTmpl = 0;
	head = parsedFormat;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		evl_fmt_segment_t *fs = (evl_fmt_segment_t*) p->li_data;
		if (fs->fs_type == EVL_FS_ATTNAME) {
			needTmpl = 1;
			break;
		}
	}

	if (needTmpl) {
		evltemplate_t *tmpl;
		tmpl = getPopulatedTemplate(entry, var_buf);
		if (!tmpl) {
			/* No such template */
			_evlFreeParsedFormat(parsedFormat);
			free(myformat);
			return EBADMSG;
		}
		for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
			evl_fmt_segment_t *fs = (evl_fmt_segment_t*) p->li_data;
			if (fs->fs_type == EVL_FS_ATTNAME) {
				evlattribute_t *att;
				status = evltemplate_getatt(tmpl,
					fs->u.fs_attname, &att);
				if (status != 0) {
					/* Template has no att by this name. */
					_evlFreeParsedFormat(parsedFormat);
					free(myformat);
					evl_releasetemplate(tmpl);
					return EBADMSG;
				}
			}
		}

		/*
		 * There's a template, and it includes all the non-standard
		 * attributes named in our format.
		 */
		status = _evlSpecialFormatEvrec(entry, var_buf, tmpl,
			parsedFormat, buf, buflen, reqlen);
		_evlFreeParsedFormat(parsedFormat);
		free(myformat);
		evl_releasetemplate(tmpl);
	} else {
		/* Format without a template. */
		status = _evlSpecialFormatEvrec(entry, var_buf, NULL,
			parsedFormat, buf, buflen, reqlen);
		_evlFreeParsedFormat(parsedFormat);
		free(myformat);
	}
	return (buf ? status : EINVAL);
}

extern int
evl_atttostr(const char *attribute, const struct posix_log_entry *entry,
	const void *var_buf, char *buf, size_t buflen, size_t *reqlen)
{
	int member;
	int status = 0;
	size_t myreqlen;
	char membuf[POSIX_LOG_MEMSTR_MAXLEN];
	const char *mybuf = NULL;

	if (reqlen) {
		*reqlen = 0;
	}
	if (!attribute || !entry) {
		return EINVAL;
	}

	/* Is this a standard attribute? */
	member = _evlGetValueByName(_evlAttributes, attribute, -1);
	if (member != -1) {
		/* Yes, a standard attribute. */
		mybuf = membuf;
		status = posix_log_memtostr(member, entry, membuf,
			POSIX_LOG_MEMSTR_MAXLEN);
		assert(status == 0);
	} else {
		/*
		 * Try to find an attribute by this name in the optional
		 * part of the record.  We currently do this by putting
		 * together a format string that is simply "%attname%",
		 * and passing that to evl_format_evrec_sprintf().  We
		 * should be able to do this with evlatt_getstring(),
		 * but that function doesn't compute reqlen for us if the
		 * buffer is too small.
		 *
		 * Note that we come here if the attribute's name is "data".
		 */
		char *fmt;
		fmt = (char*) malloc(strlen(attribute)+3);
		assert(fmt != NULL);
		snprintf(fmt, strlen(attribute)+3,  "%%%s%%", attribute);
		status = evl_format_evrec_sprintf(entry, var_buf, fmt, buf,
			buflen, reqlen);
		
		/*
		 * evl_format_evrec_sprintf() returns EBADMSG for an
		 * unrecognized attribute, but we return EINVAL.
		 */
		if (status == EBADMSG) {
			status = EINVAL;
		}
		free(fmt);
		return status;
	}

	/* We get here only if attribute is a standard name (not "data"). */
	/* mybuf points to the attribute's encoded value. */
	myreqlen = strlen(mybuf) + 1;
	if (reqlen) {
		*reqlen = myreqlen;
	}
	if (!buf) {
		status = EINVAL;
	} else if (buflen < myreqlen) {
		status = EMSGSIZE;
	}
	if (status == 0) {
		(void) strcpy(buf, mybuf);
	}
	return status;
}

/*
 * Copies nbytes bytes from b->b_next to dest.  Returns -1 if fewer than
 * nbytes bytes remain in the record, or 0 otherwise.
 */
static int
collectBytes(recbuf_t *b, void *dest, size_t nbytes)
{
	if (b->b_end - b->b_next < (ptrdiff_t) nbytes) {
		b->b_next = b->b_end;
		return -1;
	}
	(void) memcpy(dest, b->b_next, nbytes);
	b->b_next += nbytes;
	return 0;
}

/*
 * Collects the string pointed to by b->b_next.  Returns a pointer to the
 * string (i.e., the initial value of b->b_next).  If there is no null
 * character between there and the end of the record, advances b->b_next
 * to the end and returns NULL.
 */
static const char *
collectString(recbuf_t *b)
{
	const char *s = b->b_next;
	while (b->b_next < b->b_end && *(b->b_next)) {
		b->b_next++;
	}
	if (b->b_next < b->b_end) {
		b->b_next++;
		return s;
	} else {
		return NULL;
	}
}

/*
 * Collects the wide-character string pointed to by b->b_next.  Returns a
 * pointer to the string (i.e., the initial value of b->b_next).  If there
 * is no wide-null character between there and the end of the record),
 * advances b->b_next to the end and returns NULL.
 */
static const wchar_t *
collectWstring(recbuf_t *b)
{
	wchar_t wc;	/* in case alignment is an issue */
	const wchar_t *s = (const wchar_t*) b->b_next;
	do {
		if (collectBytes(b, &wc, sizeof(wchar_t)) < 0) {
			return NULL;
		}
	} while (wc != 0);
	return s;
}

/*
 * String s contains one or more lines. Edit s in place, removing any
 * instances of the printk severity code (e.g., "<1>") at the beginning
 * of a line.
 */
static void
elidePrintkSeverities(char *s)
{
	char c, *p = s, *q = s;

	/* Each iteration of this loop starts with p at the start of a line. */
	do {
		if (p[0] == '<'
		    && '0' <= p[1] && p[1] <= '7'
		    && p[2] == '>') {
			p += 3;
		}
		do {
			c = *p;
			if (p > q) {
				*q = c;
			}
			p++;
			q++;
		} while (c != '\n' && c != '\0');
	} while (c == '\n');
}

/*
 * seg is a format-string segment of type EVL_FS_PRINTFCONV.  According to
 * that conversion specification, format the value at b->b_next into f.
 * Returns 0 on success (even if we reach end-of-record early) or an
 * appropriate errno value.
 */
static int
formatConversion(evl_fmt_buf_t *f, evl_fmt_segment_t *seg, recbuf_t *b)
{
	int status = 0;
	const char *fmt = seg->fs_userfmt;
	int i, dim = 0;
	const char *delim = NULL;
	char squeezedFmt[20];
	long long alval[4];	/* big, aligned buffer */
	tmpl_base_type_t type;
	size_t attSize;

	if (seg->u.fs_conversion->fm_array) {
		/* An array.  Collect the dimension and delimiter. */
		char *s;
		const char *f;

		if (collectBytes(b, &dim, sizeof(int)) < 0) {
			/* Hit end of record */
			return 0;
		}
		delim = collectString(b);
		if (!delim) {
			/* EOR */
			return 0;
		}
		/* Squeeze '[' character(s) out of fmt. */
		assert(strlen(fmt) < 20);
		for (s = squeezedFmt, f = fmt; *f; f++) {
			if (*f != '[') {
				*s++ = *f;
			}
		}
		*s = '\0';
		fmt = squeezedFmt;
		type = _evlGetTypeFromConversion(seg->u.fs_conversion, 0, 1);
	} else {
		dim = 1;
		type = _evlGetTypeFromConversion(seg->u.fs_conversion, 1, 1);
	}
	attSize = _evlTmplTypeInfo[type].ti_size;

	for (i = 0; i < dim; i++) {
		if (i > 0) {
			_evlBprintf(f, "%s", delim);
		}
		switch (type) {
		case TY_NONE:
			status = EBADMSG;
			goto out;
		case TY_SPECIAL:
			/* Something that we accept but ignore, like %n. */
			goto out;
		case TY_STRING:
		    {
		    	const char *s = collectString(b);
			if (s) {
				_evlBprintf(f, fmt, s);
			} else {
				goto out;
			}
			break;
		    }
		case TY_WSTRING:
		    {
			const wchar_t *s = collectWstring(b);
			if (s) {
				_evlBprintf(f, fmt, s);
			} else {
				goto out;
			}
			break;
		    }
		default:
		    {
			/* Scalar types */
			if (collectBytes(b, alval, attSize) < 0) {
				/* EOR */
				goto out;
			}
			switch (type) {
			case TY_CHAR:
			case TY_UCHAR:
				_evlBprintf(f, fmt, *((char*) alval));
				break;
			case TY_SHORT:
				_evlBprintf(f, fmt, *((short*) alval));
				break;
			case TY_INT:
				_evlBprintf(f, fmt, *((int*) alval));
				break;
			case TY_LONG:
				_evlBprintf(f, fmt, *((long*) alval));
				break;
			case TY_LONGLONG:
				_evlBprintf(f, fmt, *((long long*) alval));
				break;
			case TY_DOUBLE:
				_evlBprintf(f, fmt, *((double*) alval));
				break;
			case TY_LDOUBLE:
				_evlBprintf(f, fmt, *((long double*) alval));
				break;
			case TY_ADDRESS:
				_evlBprintf(f, fmt, *((void**) alval));
				break;
			case TY_WCHAR:
				_evlBprintf(f, fmt, *((wchar_t*) alval));
				break;
			default:
				assert(0);
			}
			break;
		    }	/* end of scalar types */
		}	/* end of all-types switch */
	}	/* end of iteration i */

out:
	return status;
}

/*
 * data points to the variable portion of a record of type POSIX_LOG_PRINTF;
 * its length is datasz.  buf points to a buffer of length buflen bytes.
 * Format the record into buf.  If reqlen is not NULL, set *reqlen to the
 * size of buffer we would need to format the record, whether or not buf
 * is NULL and/or too small.
 *
 * If printk != 0, apply printk-style adjustments to the formatted text:
 * remove the terminating newline and any leading <n> severity codes.
 */
int
_evlFormatPrintfRec(const char *data, size_t datasz, char *buf, size_t buflen,
	size_t *reqlen, int printk)
{
	evl_list_t *parsedFmt;
	evl_listnode_t *head, *end, *p;
	const char *format;	/* the format string in the record */
	char *formatDup;
	recbuf_t recbuf;	/* where we are in the record's data buffer */
	evl_fmt_buf_t *f;	/* where we are in buf */
	size_t myreqlen;
	int status = 0;

	if (reqlen) {
		/* In case we bail out early */
		*reqlen = 0;
	}
	if (!data || datasz == 0) {
		return EINVAL;
	}

	/* Make a copy of the format string that we can scribble on. */
	recbuf.b_next = data;
	recbuf.b_end = data + datasz;
	format = collectString(&recbuf);
	if (format == NULL) {
		return EBADMSG;
	}
	formatDup = strdup(format);
	assert(formatDup != NULL);

	/* Skip over the argsz word following the format. */
	recbuf.b_next += sizeof(int);

	parsedFmt = _evlParsePrintfFormat(formatDup, 1, &status);
	if (status != 0) {
		free(formatDup);
		return EBADMSG;
	}

	/* Note: buf may be NULL and/or buflen may be 0 here. */
	f = _evlMakeFmtBuf(buf, buflen);

	/*
	 * Loop through the segments of the parsed format specification,
	 * copying string segments and formatting the next attribute for
	 * each printf-conversion segment.
	 */
	head = parsedFmt;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		tmpl_base_type_t type;
		const char *fmt;
		evl_fmt_segment_t *seg = (evl_fmt_segment_t*) p->li_data;

		if (seg->fs_type == EVL_FS_STRING) {
			_evlBprintf(f, "%s", seg->u.fs_string);
			continue;
		}

		assert(seg->fs_type == EVL_FS_PRINTFCONV);
		if (recbuf.b_next >= recbuf.b_end) {
			/*
			 * No more data to format, but there may still be
			 * some string segments.
			 */
			continue;
		}
		status = formatConversion(f, seg, &recbuf);
		if (status != 0) {
			return status;
		}
	}	/* end of loop through segments of format string */

	/*
	 * At this point, the data have been formatted into buf.  If
	 * buf is NULL or too small, _evlBprintf() has handled it OK,
	 * and it will be caught here.
	 */
	myreqlen = 1 + f->fb_next - f->fb_buf;	/* Count the null. */
	if (reqlen) {
		*reqlen = myreqlen;
	}
	if (f->fb_oflow) {
		if (status == 0) {
			status = (buf ? EMSGSIZE : EINVAL);
		}
	} else if (printk) {
		if (myreqlen > 1 && buf[myreqlen-2] == '\n') {
			buf[myreqlen-2] = '\0';
		}
		elidePrintkSeverities(buf);
		if (reqlen) {
			*reqlen = strlen(buf)+1;
		}
	}

	free(formatDup);
	_evlFreeParsedFormat(parsedFmt);
	_evlFreeFmtBuf(f);
	return status;
}
