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
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

#include "evl_util.h"

#define SCRATCH_PAD_SIZE 4

evl_fmt_buf_t *
_evlMakeFmtBuf(char *buf, size_t bufsz)
{
	evl_fmt_buf_t *fb;
	fb = (evl_fmt_buf_t *) malloc(sizeof(evl_fmt_buf_t));
	assert(fb != NULL);

	if (buf == NULL || bufsz == 0) {
		/*
		 * Caller doesn't want us to write data into the buffer,
		 * just to tell him how big a buffer he'd need.
		 */
		buf = fb->fb_dummy;
		bufsz = 1;
		fb->fb_oflow = 1;
	} else {
		fb->fb_oflow = 0;
	}

	fb->fb_buf = buf;
	fb->fb_next = buf;
	fb->fb_end = buf + bufsz;

	/* Start with a null string. */
	buf[0] = '\0';
	return fb;
}

void
_evlFreeFmtBuf(evl_fmt_buf_t *f)
{
	free(f);
}

/*
 * Basically snprintf to the buffer f.  Note that although we never write past
 * fb_end, fb_next always reflects where we would be writing if we had enough
 * room, and we always run vsnprintf() to determine that.  This allows us to
 * answer the question "How big a buffer would I need to format this record?"
 */
void
_evlBprintf(evl_fmt_buf_t *f, const char *fmt, ...)
{
	va_list args;
	int n;			/* # bytes written, not counting '\0' */
	char scratchPad[SCRATCH_PAD_SIZE];
	char *writeHere;
	int writeThisMuch;
	ptrdiff_t room;		/* room left in our buffer */
	
	room = f->fb_end - f->fb_next;
	if (room <= 1) {
		/*
		 * Room for at most the terminating null, which should already
		 * be there.  Have vsnprintf() write to the scratch pad.
		 */
		writeThisMuch = SCRATCH_PAD_SIZE;
		writeHere = scratchPad;
	} else {
		writeThisMuch = room;
		writeHere = f->fb_next;
	}

	va_start(args, fmt);
	n = vsnprintf(writeHere, writeThisMuch, fmt, args);
	va_end(args);

	if (n < 0) {
		/* vsnprintf() failed.  Nothing encoded. */
		n = 0;
		f->fb_next[0] = '\0';
	}
	f->fb_next += n;
	if (n+1 > room) {
		f->fb_oflow = 1;
		f->fb_end[-1] = '\0';
	}
}

/*
 * Format the indicated bytes in dump format to format buffer f.
 */
void
_evlDumpBytesToFmtBuf(const void *buf, size_t nBytes, evl_fmt_buf_t *f)
{
	char scratchPad[SCRATCH_PAD_SIZE];
	char *writeHere;
	int writeThisMuch;
	ptrdiff_t room;
	size_t roomNeeded = 0;
	int status;

	room = f->fb_end - f->fb_next;
	if (room <= 1) {
		writeHere = scratchPad;
		writeThisMuch = SCRATCH_PAD_SIZE;
	} else {
		writeHere = f->fb_next;
		writeThisMuch = room;
	}
	status = _evlDumpBytesForce(buf, nBytes, writeHere, writeThisMuch,
		&roomNeeded);
	assert(status == 0);

	if (roomNeeded > room) {
		f->fb_oflow = 1;
		f->fb_end[-1] = '\0';
		f->fb_next += roomNeeded;
	} else {
		f->fb_next += strlen(f->fb_next);
	}
}
