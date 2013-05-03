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
#include <errno.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"

/*
 * This test program is intended to be used as follows:
 * Run the program interactively:
 *	./frtest2
 * It reads from stdin.  Type in facility names and/or codes, one per line.
 * Verify that it responds with the corresponding entry from the facility
 * registry (/etc/evlog.d/facility_registry).  (If you type an unrecognized
 * numeric code, it'll just print that code twice, since its string value
 * is just the sprintf of its numeric value.)
 *
 * While the program is still running, change the contents of the facility
 * registry.  Keep entering names and/or codes.  Within 5 seconds, you should
 * see that the entries in the new facility registry have taken effect.  (If
 * the new facility registry has errors, you should see them printed, and
 * the previous version of the registry should stay in effect.)
 */

static char *progName;

usage()
{
	fprintf(stderr, "Usage: %s < facCodes\n", progName);
	exit(1);
}

/*
 * If s is an integer (and nothing but an integer), return its value.
 * Else return badVal.
 */
static long
extractNumber(const char *s, long badVal)
{
	char *endPtr = 0;
	long nl = strtol(s, &endPtr, 0);

	if (*endPtr != '\0') {
		return badVal;
	}
	return nl;
}

main(int argc, char **argv)
{
	char line[200];
	char nameBuf[POSIX_LOG_MEMSTR_MAXLEN];
	int nErrors = 0;
	int status = 0;

	progName = argv[0];
	if (argc != 1) {
		usage();
	}

	while (fgets(line, 200, stdin)) {
		posix_log_facility_t code;
		char *name;
		int error = 0;

		/* Strip the newline. */
		line[strlen(line)-1] = '\0';

		code = (posix_log_facility_t) extractNumber(line, -1);
		if (code == -1) {
			/* Not a number.  Try it as a name. */
			name = line;
			status = posix_log_strtofac(name, &code);
			if (status != 0) {
				fprintf(stderr, "No code for name %s\n", name);
				error = 1;
			}
		} else {
			/* We've been given a numeric code.  Get the name. */
			status = posix_log_factostr(code, nameBuf, 
				POSIX_LOG_MEMSTR_MAXLEN);
			if (status != 0) {
				fprintf(stderr, "No name for code %d\n", code);
				error = 1;
			} else {
				name = nameBuf;
			}
		}

		nErrors += error;
		if (!error) {
			printf("%d %s\n", code, name);
		}
	}

	exit(nErrors);
}
