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
#include <errno.h>
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"

/*
 * Logs event records to be interpreted using the "degenerate.t"
 * templates of facility "user".
 */

int exitStatus = 0;

static void
checkStatus(int status, int evty, const char *func)
{
	if (status != 0) {
		fprintf(stderr, "write of event type %d failed:\n", evty);
		errno = status;
		perror(func);
		exitStatus = 1;
	}
}

main(int argc, char **argv)
{
	int status;
	int type;
	posix_log_severity_t sev = LOG_INFO;

	for (type = 4101; type <= 4105; type++) {
		status = posix_log_write(LOG_USER, type, LOG_ERR,
			NULL, 0, POSIX_LOG_NODATA, 0);
		checkStatus(status, type, "posix_log_write");
	}
	
	for (type = 4111; type <= 4116; type++) {
		status = posix_log_write(LOG_USER, type, LOG_INFO,
			NULL, 0, POSIX_LOG_NODATA, 0);
		checkStatus(status, type, "posix_log_write");
	}

	status = posix_log_printf(LOG_USER, 4121, LOG_NOTICE, 0,
		"Client connection refused: authentication failed");
	checkStatus(status, type, "posix_log_printf");

	status = posix_log_printf(LOG_USER, 4122, LOG_NOTICE, 0,
		"Client connection refused: connection limit exceeded");
	checkStatus(status, type, "posix_log_printf");
	
	exit(exitStatus);
}
