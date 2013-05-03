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

/*
 * Test the posix_log_seek options that aren't tested by evl_view.
 */

static char *progName;

static void
usage()
{
	fprintf(stderr, "Usage: %s [query]\n", progName);
	exit(1);
}

static int
seekAndPrint(posix_logd_t desc, posix_log_query_t *query, int direction)
{
	int status;
	struct posix_log_entry entry;
	char recbuf[POSIX_LOG_ENTRY_MAXLEN];
	char outbuf[2000];
	size_t reqlen;

	status = posix_log_seek(desc, query, direction);
	if (status != 0) {
		errno = status;
		perror("posix_log_seek");
		return -1;
	}
	status = posix_log_read(desc, &entry, recbuf, POSIX_LOG_ENTRY_MAXLEN);
	if (status != 0) {
		errno = status;
		perror("posix_log_read");
		return -1;
	}
	status = evl_format_evrec_fixed(&entry, outbuf, 2000, &reqlen, ", ",
		80, 0);
	if (status != 0) {
		errno = status;
		perror("evl_format_evrec_fixed");
		fprintf(stderr, "reqlen = %zd\n", reqlen);
		return -1;
	}
	printf("%s\n", outbuf);
	if (entry.log_format == POSIX_LOG_NODATA) {
		return 0;
	}
	status = evl_format_evrec_variable(&entry, recbuf, outbuf, 2000,
		&reqlen);
	if (status != 0) {
		errno = status;
		perror("evl_format_evrec_variable");
		fprintf(stderr, "reqlen = %zd\n", reqlen);
		return -1;
	}
	printf("%s\n", outbuf);
	return 0;
}

main(int argc, char **argv)
{
	int status;
	posix_logd_t desc;
	posix_log_query_t query, *qp = NULL;
	char errbuf[100];

	progName = argv[0];
	switch (argc) {
	case 1:
		break;
	case 2:
		qp = &query;
		status = posix_log_query_create(argv[1], POSIX_LOG_PRPS_SEEK,
			qp, errbuf, 100);
		if (status != 0) {
			errno = status;
			perror("posix_log_query_create");
			fprintf(stderr, "%s\n", errbuf);
			exit(1);
		}
		break;
	default:
		usage();
	}

	status = posix_log_open(&desc, 0);
	if (status != 0) {
		errno = status;
		perror("posix_log_open");
		exit(1);
	}

	printf("first:\n");
	(void) seekAndPrint(desc, qp, POSIX_LOG_SEEK_FIRST);
	printf("last:\n");
	(void) seekAndPrint(desc, qp, POSIX_LOG_SEEK_LAST);
	printf("bof:\n");
	(void) seekAndPrint(desc, NULL, POSIX_LOG_SEEK_START);

	exit(0);
}
