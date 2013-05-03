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
#include <malloc.h>
#include <errno.h>
#include <ctype.h>
#include "posix_evlog.h"
#include "evlog.h"

static char *progName;

void
usage()
{
	fprintf(stderr, "Usage: %s [attbufsz] attribute_name ...\n", progName);
	exit(1);
}

main(int argc, char **argv)
{
	int status;
	struct posix_log_entry entry;
	posix_logd_t desc;
	char *attbuf;
	size_t attbufsz = POSIX_LOG_ENTRY_MAXLEN;
	char recbuf[POSIX_LOG_ENTRY_MAXLEN];
	int firstatt = 1;
	int i;
	size_t reqlen;

	progName = argv[0];
	if (argc < 2) {
		usage();
	}
	if (isdigit(*argv[1])) {
		attbufsz = atoi(argv[1]);
		firstatt = 2;
		if (argc < 3) {
			usage();
		}
	}

	status = posix_log_open(&desc, 0);
	if (status != 0) {
		errno = status;
		perror("posix_log_open");
		exit(1);
	}

	for (;;) {
		status = posix_log_read(desc, &entry, recbuf,
			POSIX_LOG_ENTRY_MAXLEN);
		if (status == EAGAIN) {
			exit(0);
		} else if (status != 0) {
			errno = status;
			perror("posix_log_read");
			exit(1);
		}

		for (i = firstatt; i < argc; i++) {
			status = evl_atttostr(argv[i], &entry, recbuf, attbuf,
				attbufsz, &reqlen);
			if (status != 0) {
				errno = status;
				perror("evl_atttostr");
				fprintf(stderr, "reqlen = %zd\n", reqlen);
				exit(1);
			}
			printf("%s=%s ", argv[i], attbuf);
		}
		printf("\n");
	}

	/*NOTREACHED*/
	exit(0);
}
