/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2002
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <assert.h>
#include <errno.h>
#include <syslog.h>

#include "config.h"
#include "evlog.h"
#include "posix_evlog.h"

extern unsigned int _evlCrc32(const unsigned char *data, int len);

static int hasSpecialFmt(const char *fmt);

/*
 * Called by the syslogat macro to log the message to evlog.
 * priority	- facility|severity, as passed to syslog
 * facname	- evlog facility name
 * event_type	- event type, derived from format, source file, etc.
 * fmt		- the raw format string passed to syslogat
 * unbraced_fmt	- fmt with {attname} constructs stripped out, as with
 *			evl_unbrace()
 * ...		- remaining args are as passed to syslog (plus any
 *			hidden args implied by fmt)
 *
 * If unbraced_fmt appears to contain a %[ construct, which syslog doesn't
 * know how to handle, we format the message ourselves, call syslog with
 * the formatted string, and return 1.  Otherwise we return 0 and let caller
 * call syslog.
 *
 * For the benefit of syslog's %m option, we save and restore errno.
 */
int
_evl_syslogat(int priority,
	const char *facname,
	int event_type,
	const char *fmt,
	const char *unbraced_fmt,
	...)
{
	posix_log_facility_t facility;
	posix_log_severity_t severity;
	va_list args;
	char recbuf[POSIX_LOG_ENTRY_MAXLEN];
	size_t datasz = 0;
	int status;
	int saved_errno = errno;
	int special_fmt;

	severity = LOG_PRI(priority);
	status = posix_log_strtofac(facname, &facility);
	if (status != 0) {
		if (!strcmp(facname, "EVL_FACILITY_NAME")) {
			/*
			 * EVL_FACILITY_NAME not defined as a macro.
			 * We can't default to the facility specified
			 * in the priority arg, because that info is
			 * unavailable at compile time and therefore
			 * unavailable to evlgentmpls.
			 */
			facility = LOG_USER;
		} else {
			fprintf(stderr, "Unknown facility: %s\n", facname);
			facility = LOG_USER;
		}
	}

	special_fmt = hasSpecialFmt(unbraced_fmt);

	/*
	 * Start by packing the raw format string (sic) and all args
	 * (including hidden args) into the record buffer.  See fixup below.
	 * TODO: Handle %m.
	 */
	va_start(args, unbraced_fmt);
	status = evl_pack_format_and_args(fmt, args, recbuf, &datasz);
	va_end(args);

	if (status != 0) {
		return 0;
	}

	if (strcmp(fmt, unbraced_fmt) != 0) {
		/*
		 * The unbraced format is different from the raw format,
		 * which means one or more attribute names have been
		 * stripped out.  In the record buffer, replace the raw
		 * format with the unbraced format.
		 *
		 * Note that theoretically, evl_pack_format_and_args()
		 * could run out of buffer when it really shouldn't --
		 * i.e., when the difference between flen and uflen puts
		 * us past the end.  The result is that we'd return above
		 * without logging the event.  Given the buffer size, however,
		 * this is very unlikely.
		 */
		int flen = strlen(fmt);
		int uflen = strlen(unbraced_fmt);
		int diff = flen - uflen;
		assert(diff > 0);
		(void) strcpy(recbuf, unbraced_fmt);
		memmove(recbuf + uflen,
			recbuf + flen,
			datasz - flen);
		datasz -= diff;
	}

	(void) posix_log_write(facility, event_type, severity, recbuf,
		datasz, POSIX_LOG_PRINTF, 0);

	if (special_fmt) {
		/* syslog can't format this message.  We'll do it. */
#define SYSLOG_MSG_MAXLEN (2*1024)
		char msg[SYSLOG_MSG_MAXLEN];
		status = _evlFormatPrintfRec(recbuf, datasz, msg,
			SYSLOG_MSG_MAXLEN, NULL, 0);
		if (status != 0 && status != EMSGSIZE) {
			/* Failed to format it.  Let syslog have a shot. */
			special_fmt = 0;
		} else {
			syslog(priority, "%s", msg);
		}
	}

	errno = saved_errno;
	return special_fmt;
}

#define min(x,y) ((x) < (y) ? (x) : (y))
/*
 * Copy src to dest, replacing strings of the form "{id}%" with "%".
 * If src contains "{{", strip out that and anything beyond it.
 * dest is a buffer of size bufsz.  Make sure we don't overflow it.
 *
 * This function is a copy of the evl_unbrace function we've added to the
 * kernel's printk.c.
 */
static void
unbrace(char *dest, const char *src, int bufsz)
{
	const char *copy_this = src, *scan_this = src;
	const char *c, *lcb, *rcb, *cut_here;
	int copy_len, dlen = 0;

	cut_here = strstr(src, "{{");
	dest[0] = '\0';
	for (;;) {
		lcb = strchr(scan_this, '{');
		if (!lcb) {
			goto done;
		}
		rcb = strstr(lcb+2, "}%");
		if (!rcb) {
			goto done;
		}
		if (cut_here && cut_here < rcb) {
			goto done;
		}
		/* Is it a valid identifier between the { and } ? */
		c = lcb+1;
		if (*c != '_' && !isalpha(*c)) {
			goto scan_again;
		}
		for (c++; c < rcb; c++) {
			if (*c != '_' && !isalnum(*c)) {
				goto scan_again;
			}
		}
		copy_len = min(lcb - copy_this, bufsz-(dlen+1));
		strncat(dest + dlen, copy_this, copy_len);
		dlen += copy_len;
		copy_this = rcb+1;
scan_again:
		scan_this = rcb+2;
	}
done:
	if (cut_here) {
		copy_len = min(cut_here - copy_this, bufsz-(dlen+1));
	} else {
		copy_len = bufsz-(dlen+1);
	}
	strncat(dest + dlen, copy_this, copy_len);
	return;
}

/*
 * Return a malloc-ed, unbraced copy of fmt.
 */
char *
_evl_unbrace(const char *fmt)
{
	char *ufmt;
	int bufsz;
	assert(fmt != NULL);
	bufsz = strlen(fmt) + 1;
	ufmt = (char *) malloc(bufsz);
	assert(ufmt != NULL);
	unbrace(ufmt, fmt, bufsz);
	return ufmt;
}

static unsigned int
crc32(const char *s)
{
	return _evlCrc32((const unsigned char*)s, strlen(s));
}

int
evl_gen_event_type(const char *s1, const char *s2, const char *s3)
{
	return crc32(s1) + crc32(s2) + crc32(s3);
}

/*
 * Compute event type based only on the "normalized" form of the
 * specified format string.  Normalizations should match what
 * evl_normalize_fmt() does (see below).  So far, we can do that
 * without malloc-ing a copy of fmt.
 */
int
evl_gen_event_type_v2(const char *fmt)
{
	int fmtlen;
	while (fmt[0] == '<' && isdigit(fmt[1]) && fmt[2] == '>') {
		/* Skip over printk-style severity code. */
		fmt += 3;
	}
	/* Ignore trailing newline(s). */
	fmtlen = (int) strlen(fmt);
	while (fmtlen > 0 && fmt[fmtlen-1] == '\n') {
		fmtlen--;
	}
	return _evlCrc32((const unsigned char*)fmt, fmtlen);
}

/*
 * malloc a buffer and copy the format string fmt into it, normalizing
 * as we go.  Return a pointer to the buffer.
 *
 * Normalizations:
 * - Strip the leading <n> severity code, if any.  (If there's more than
 *	one, we strip them all.  We do NOT strip <n> after the first newline.)
 * - Strip all trailing newlines.
 */
char *
evl_normalize_fmt(const char *fmt)
{
	int fmtlen;
	char *nfmt = (char*) malloc(strlen(fmt)+1);
	assert(nfmt != NULL);

	/* Skip over printk-style severity code. */
	while (fmt[0] == '<' && isdigit(fmt[1]) && fmt[2] == '>') {
		fmt += 3;
	}
	(void) strcpy(nfmt, fmt);

	/* Discard trailing newline(s). */
	fmtlen = (int) strlen(nfmt);
	while (fmtlen > 0 && nfmt[fmtlen-1] == '\n') {
		fmtlen--;
	}
	nfmt[fmtlen] = '\0';

	return nfmt;
}

/*
 * Return 1 if the indicated format string has any conversions that syslog
 * can't handle.  We just look for % followed by [, with nothing intervening
 * except other legal flag characters.
 */
static int
hasSpecialFmt(const char *fmt)
{
	/* Keep these flags in sync with those in _evlParseFmtConvSpec. */
	static char *flags = "-+ #0'";
	const char *c, *pct;

	if (!strchr(fmt, '[')) {
		return 0;
	}

	c = fmt;
	while (pct = strchr(c, '%')) {
		c = pct+1; 
		if (*c == '%') {
			/* %% */
			c++;
			continue;
		}
		c += strspn(c, flags);
		if (*c == '[') {
			return 1;
		}
	}
	return 0;
}
