/*
 * IBM Event Logging for Linux
 * Copyright (c) International Business Machines Corp., 2001
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
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"

/*
 * Read up to POSIX_LOG_ENTRY_MAXLEN bytes of data from stdin and print it in
 * dump format.  If argv[1] is specified, it is taken as the size of the
 * buffer to be passed to _evlDumpBytes().
 */
main(int argc, char **argv)
{
	size_t buflen = 6*POSIX_LOG_ENTRY_MAXLEN;
	char *buf;
	char data[POSIX_LOG_ENTRY_MAXLEN];
	int nBytes;
	int status;
	size_t reqlen = 999;

	if (argc == 2) {
		buflen = atoi(argv[1]);
	}
	buf = (char *) malloc(buflen);
	assert(buf != NULL);

	nBytes = read(0, data, POSIX_LOG_ENTRY_MAXLEN);
	if (nBytes < 0) {
		perror("read");
		exit(1);
	}

	status = _evlDumpBytes(data, nBytes, buf, buflen, &reqlen);
	if (status != 0) {
		errno = status;
		perror("_evlDumpBytes");
		fprintf(stderr, "Required length = %u\n", reqlen);
		exit(1);
	}
	printf("%s\n", buf);
	exit(0);
}
