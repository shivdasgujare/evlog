/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2002
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
#include "posix_evlog.h"
#include "posix_evlsup.h"

extern int _evlPackCmdArgs(int argc, const char **argv, int *iarg,
	char *databuf, size_t *datasz, char *errbuf, size_t ebufsz);
extern int _evlPackCmdArgsPerFormat(const char *format, int argc,
	const char **argv, int *iarg, char *databuf, size_t *datasz,
	char *errbuf, size_t ebufsz);

const char *progName = 0;

static void
usage()
{
	fprintf(stderr,
"Usage: %s -f | --facility facility -t | --type event_type\n"
"             [-s | --severity severity]\n"
"             [-m | --message message]\n"
"             [-p | --printf format attr-value ...]\n"
"             [-b | --binary attr-type attr-value ...]\n",
		progName);
	exit(1);
}

/*
 * If s is an integer (and nothing but an integer), return its value.
 * Else return badVal.  If unSigned == 1, require an unsigned integer.
 */
static long
extractNumber(const char *s, long badVal, int unSigned)
{
	char *endPtr = 0;
	long nl;
	if (unSigned) {
		nl = strtoul(s, &endPtr, 0);
	} else {
		nl = strtol(s, &endPtr, 0);
	}

	if (*endPtr != '\0') {
		return badVal;
	}
	return nl;
}

/*
 * arg is either a name or a sequence of digits.  If it's a name in the
 * indicated table, return the corresponding value.  Else try to
 * decode it as a number.  If that fails, return badVal.  If it's a valid
 * number, verify that it's in the table; return badVal if it isn't.
 */
static int
getNumberOrName(const char *arg, const struct _evlNvPair table[], int badVal,
	int unSigned)
{
	int n;

	if (table) {
		n = _evlGetValueByCIName(table, arg, badVal);
		if (n != badVal) {
			return n;
		}
	}

	/* Not a valid name in the indicated table.  Should be a number. */
	n = (int) extractNumber(arg, badVal, unSigned);
	if (n != badVal) {
		/* It's a number.  Is it in the table? */
		const struct _evlNvPair *nv;
		for (nv = table; nv->nv_name; nv++) {
			if (nv->nv_value == n) {
				return n;
			}
		}
	}
	return badVal;
}

main(int argc, char **argv)
{
	int i, status;
	posix_log_facility_t facility = EVL_INVALID_FACILITY;
	int type = EVL_INVALID_EVENT_TYPE;
	posix_log_severity_t severity = LOG_INFO;
	const char *msg = NULL;
	const char *format = NULL;
	int haveFac = 0;
	int haveEvt = 0;
	int haveSev = 0;
	int haveMsg = 0;
	int haveBinary = 0;
	int havePrintf = 0;
	char databuf[POSIX_LOG_ENTRY_MAXLEN];
	size_t reclen = 0;
	struct _evlNvPair *facilities = _evlSnapshotFacilities();

	progName = argv[0];

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--facility")) {
			if (++i >= argc || haveFac++) {
				usage();
			}
			facility = getNumberOrName(argv[i], facilities,
				EVL_INVALID_FACILITY, 1);
			if (facility == EVL_INVALID_FACILITY) {
fprintf(stderr, "%s: unknown facility: %s\n", progName, argv[i]);
				exit(1);
			}
		} else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--type")) {
			if (++i >= argc || haveEvt++) {
				usage();
			}
			/* Event type must be numeric. */
			type = extractNumber(argv[i], EVL_INVALID_EVENT_TYPE, 0);
			if (type == EVL_INVALID_EVENT_TYPE) {
fprintf(stderr, "%s: bad event type: %s\n", progName, argv[i]);
				exit(1);
			}
		} else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--severity")) {
			if (++i >= argc || haveSev++) {
				usage();
			}
			/* Severity can be numeric or symbolic. */
			severity = getNumberOrName(argv[i], _evlSeverities, -1, 1);
			if (severity == -1) {
fprintf(stderr, "%s: unknown severity: %s\n", progName, argv[i]);
				exit(1);
			}
		} else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--message")) {
			if (++haveMsg + haveBinary + havePrintf != 1) {
				usage();
			}
			if (++i > argc) {
				usage();
			}
			msg = argv[i];
		} else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--binary")) {
			char errbuf[100];
			if (haveMsg + ++haveBinary + havePrintf != 1) {
				usage();
			}
			i++;
			if (_evlPackCmdArgs(argc, (const char**)argv, &i,
			    databuf, &reclen, errbuf, 100) != 0) {
				fprintf(stderr, "%s: %s\n", progName, errbuf);
				exit(1);
			}
			if (i < argc) {
				/*
				 * There are more args to process.  i now
				 * points to the next arg after the -b list.
				 * i-- for now because the for statement
				 * will promptly i++.
				 */
				i--;
			}
		} else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--printf")) {
			char errbuf[100];
			if (haveMsg + haveBinary + ++havePrintf != 1) {
				usage();
			}
			if (++i >= argc) {
				usage();
			}
			format = argv[i++];
			if (_evlPackCmdArgsPerFormat(format, argc,
			    (const char**)argv, &i, databuf, &reclen,
			    errbuf, 100) != 0) {
				fprintf(stderr, "%s: %s\n", progName, errbuf);
				exit(1);
			}
			if (i < argc) {
				/* See above comment. */
				i--;
			}
		} else {
			usage();
		}
	}

	if (!haveFac) {
		usage();
	}

	if (!haveEvt) {
		if (havePrintf) {
			type = evl_gen_event_type_v2(format);
		} else {
			usage();
		}
	}

	if (haveMsg) {
		status = posix_log_write(facility, type, severity, msg,
			strlen(msg)+1, POSIX_LOG_STRING, 0);
	} else if (haveBinary) {
		status = posix_log_write(facility, type, severity, databuf,
			reclen, (reclen ? POSIX_LOG_BINARY : POSIX_LOG_NODATA), 
			0);
	} else if (havePrintf) {
		status = posix_log_write(facility, type, severity, databuf,
			reclen, POSIX_LOG_PRINTF, 0);
	} else {
		usage();
	}
	if (status != 0) {
		errno = status;
		perror("posix_log_write");
		exit(1);
	}
	exit(0);
}
