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
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include "posix_evlog.h"
#include "posix_evlsup.h"

char *progName;

void
usage()
{
	fprintf(stderr, "Usage: %s [-c] [-l linelen] [-d delim] [-b buflen]\n",
		progName);
	exit(1);
}

main(int argc, char **argv)
{
	int status;
	struct posix_log_entry entry;
	char buf[POSIX_LOG_ENTRY_MAXLEN];
	posix_logd_t desc;
	char formattedRec[2000];
	int i;
	size_t reqlen = 0;

	int linelen = 80, compact = 0, buflen = 2000;
	char *delim = ", ";

	progName = argv[0];

	for (++argv; *argv; argv++) {
		if (!strcmp(*argv, "-c")) {
			compact = 1;
		} else if (!strcmp(*argv, "-l")) {
			++argv;
			if (!*argv) {
				usage();
			}
			linelen = atoi(*argv);
		} else if (!strcmp(*argv, "-b")) {
			++argv;
			if (!*argv) {
				usage();
			}
			buflen = atoi(*argv);
		} else if (!strcmp(*argv, "-d")) {
			++argv;
			if (!*argv) {
				usage();
			}
			delim = *argv;
			if (strlen(delim) == 0) {
				delim = 0;
			}
		} else {
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
		status = posix_log_read(desc, &entry, buf, POSIX_LOG_ENTRY_MAXLEN);
		if (status != 0) {
			errno = status;
			perror("posix_log_read");
			exit(status == EAGAIN ? 0 : 1);
		}

		status = evl_format_evrec_fixed(&entry, formattedRec, buflen,
			&reqlen, delim, linelen, (compact ? 0x1 : 0));
		if (status != 0) {
			errno = status;
			perror("evl_format_evrec_fixed");
			fprintf(stderr, "reqlen = %u\n", reqlen);
			exit(1);
		}
		printf("%s\n", formattedRec);

		if (entry.log_format == POSIX_LOG_STRING) {
			printf("%s\n", buf);
		} else {
			printf("variable data: %u bytes\n", entry.log_size);
		}
	}

	/*NOTREACHED*/
	exit(0);
}
