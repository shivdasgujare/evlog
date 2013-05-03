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
 * Logs event records to be interpreted using the "default.t" template
 * of facility "local7".
 */

#define LOGERR(evtype, sev, msg, args...) evl_log_write(LOG_LOCAL7, evtype, sev, 0, "string", __FILE__, "int",  __LINE__, "string", msg, ##args)

int exitStatus = 0;

static void
checkStatus(int status, int evty)
{
	if (status != 0) {
		fprintf(stderr, "write of event type %d failed:\n", evty);
		errno = status;
		perror("evl_log_write");
		exitStatus = 1;
	}
}

main(int argc, char **argv)
{
	int status;
	int type;
	posix_log_severity_t sev = LOG_INFO;
	
	type = 1001;
	status = LOGERR(type, sev,
		"This event record has a message string, but no further data.",
		"endofdata");
	checkStatus(status, type);
	
	type = 1002;
	status = LOGERR(type, sev,
		"This event record has a message string plus 3 ints.",
		"3*int", 11, 21, 1998,
		"endofdata");
	checkStatus(status, type);
	
	type = 1003;
	status = LOGERR(type, sev,
		"Basically the same data as for event type 1002,\n"
		"but formatted with a non-default template.",
		"3*int", 11, 21, 1998,
		"endofdata");
	checkStatus(status, type);

	type = 1004;
	status = LOGERR(type, sev,
		"This event record has a message string, plus another string.",
		"string", "This is the other string.",
		"endofdata");
	checkStatus(status, type);
	
	type = 1003;
	status = LOGERR(type, sev,
		"This is another instance of event type 1003,\n"
		"with a different message and different ints.",
		"3*int", 9, 30, 2000,
		"endofdata");
	checkStatus(status, type);
	
	exit(exitStatus);
}
