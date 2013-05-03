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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "scanner.h"

extern char *progName;
static char *lxPtr;
char *formatBuf;
static int lineNumber = 0;

/* lxInput and lxUnput are used by scanner.c */
static int
lxInput()
{
	if (*lxPtr == '\0') {
		return 0;
	} else {
		return *lxPtr++;
	}
}

/*ARGSUSED*/
static void
lxUnput(int c)
{
	--lxPtr;
	assert(lxPtr >= formatBuf);
}

/*
 * formatBuf points to a raw format string.  Translate \ escapes and return the
 * result.
 */
static char *
getFormatFromBuf(const char *parsee)
{
	char *s;
	lxPtr = formatBuf;
	s = lxGetString(0, lxInput, lxUnput, &lineNumber);
	if (!s) {
		fprintf(stderr, "%s: Couldn't parse %s string\n", progName,
			parsee);
		exit(1);
	}
	return s;
}

char *
getFormatFromString(char *s)
{
	formatBuf = s;
	return getFormatFromBuf("format");
}

/* The indicated file contains the format string.  Read it. */
char *
getFormatFromFile(const char *path)
{
	int fd;
	struct stat st;
	int nBytes;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: Cannot open format file\n", progName);
		perror(path);
		exit(1);
	}
	if (fstat(fd, &st) != 0) {
		perror("stat of format file");
		exit(2);
	}
	formatBuf = (char *) malloc((size_t) st.st_size + 1);
	assert(formatBuf != NULL);
	nBytes = read(fd, formatBuf, st.st_size);
	if (nBytes < 0) {
		perror("read of format file");
		exit(2);
	}
	close(fd);

	if (nBytes != st.st_size) {
		fprintf(stderr,
"%s: Expected to read %d bytes from %s, but read %d\n",
			progName, st.st_size, path, nBytes);
		exit(2);
	}
	formatBuf[st.st_size] = '\0';
	if (strlen(formatBuf) != st.st_size) {
		fprintf(stderr, "%s: %s is not a text file\n", progName, path);
		exit(1);
	}

	return getFormatFromBuf("format");
}

char *
parseSeparator(char *sep)
{
	lineNumber = 0;
	formatBuf = sep;
	return getFormatFromBuf("separator");
}
