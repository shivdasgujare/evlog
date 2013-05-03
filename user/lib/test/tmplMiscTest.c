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
 * Logs event records to be interpreted using the "tmplMiscTest.t" template
 * of facility "user".
 */

#define LOGERR(evtype, sev, msg, args...) evl_log_write(LOG_USER, evtype, sev, 0, "string", msg, ##args)

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

struct familyFlags {
	unsigned int	fatherFlag:1;
	unsigned int	motherFlag:1;
	unsigned int	sonFlag:1;
	unsigned int	daughterFlag:1;
	unsigned int	catFlag:1;
	unsigned int	dogFlag:1;
	unsigned int	computerFlag:1;
	unsigned int	otherFlag:1;
};

main(int argc, char **argv)
{
	int status;
	int type;
	posix_log_severity_t sev = LOG_INFO;
	struct familyFlags ff;
	const char *msg;
	
	type = 5001;
	status = LOGERR(type, sev,
		"This is the message for event type 5001.",
		"endofdata");
	checkStatus(status, type);
	
#define human		0x1
#define inhuman		0x0
#define adult		0x2
#define juvenile	0x0
#define female		0x4
#define male		0x0
#define encodeOrganism(name,type) "string",name,"3*int",type,type,type
	type = 5012;
	status = LOGERR(type, sev,
		"This is the message for event type 5002.",
		encodeOrganism("Donald Duck", inhuman|male|adult),
		encodeOrganism("Daisy Duck", inhuman|female|adult),
		encodeOrganism("Dewey Duck", inhuman|male|juvenile),
		encodeOrganism("April Duck", inhuman|female|juvenile),
		encodeOrganism("Fred Flintstone", human|male|adult),
		encodeOrganism("Wilma Flintstone", human|female|adult),
		encodeOrganism("Pebbles Flintstone", human|female|juvenile),
		encodeOrganism("Bambam Rubble", human|male|juvenile),
		"endofdata");
	checkStatus(status, type);

	type = 5003;
	status = LOGERR(type, sev,
		"This is the message for event type 5003.",
		"string", "George W. Bush",
		"string", "1600 Pennsylvania Ave. NW",
		"string", "Washington",
		"string", "DC",
		"string", "20502",
		"string", "US President",
		"double", 200000.00,
		"2*char", 1, 20, "short", 2001,
		"endofdata");
	checkStatus(status, type);

	type = 5004;
	status = LOGERR(type, sev,
		"The Flagstons:",
		"char", 2, "2*string", "Hi", "Lois",
		"char", 2, "2*string", "Chip", "Ditto",
		"char", 2, "2*string", "Dot", "Trixie",
		"char", 0,
		"char", 1, "string", "Dawg",
		"char", 0,
		"char", 0,
		"endofdata");
	checkStatus(status, type);

	type = 5005;
	memset(&ff, 0, sizeof(ff));
	ff.fatherFlag = 1;
	ff.motherFlag = 1;
	ff.daughterFlag = 1;
	ff.otherFlag = 1;
	status = LOGERR(type, sev,
		"The Flintstones:",
		"char[]", sizeof(ff), &ff,
		"4*string", "Fred", "Wilma", "Pebbles", "Dino",
		"endofdata");
	checkStatus(status, type);

	type = 5006;
	status = LOGERR(type, sev,
		"This message should be followed by 3 extra strings.",
		"3*string", "Extra string 1", "Extra string 2", "Extra string 3",
		"endofdata");
	checkStatus(status, type);

	msg = "Test of specifying event_type "
		"as string in template.";
	type = (int) _evlCrc32((const unsigned char *)msg, strlen(msg));
	status = posix_log_printf(LOG_USER, type, sev, 0, msg);
	if (status != 0) {
		fprintf(stderr, "write of event type %#x failed:\n", type);
		errno = status;
		perror("posix_log_printf");
		exitStatus = 1;
	}

	exit(exitStatus);
}
