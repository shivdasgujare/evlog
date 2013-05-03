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

/*
 * Logs event records that contain arrays of structs.
 */

typedef struct _date {
	char	month;
	char	day;
	short	year;
} date_t;

typedef struct _event {
	date_t	date;
	char	desc[32];
} event_t;

event_t osuFbGames01[] = {
	{ {1,1,2001},	"Beat Notre Dame" },
	{ {9,2,2001},	"Lost to Fresno State" },
	{ {9,8,2001},	"Beat NM State" },
	{ {9,29,2001},	"Lost to UCLA" },
	{ {10,6,2001},	"Lost to WSU" },
	{ {10,13,2001},	"Beat Arizona" },
	{ {10,20,2001},	"Lost to ASU" },
	{ {10,27,2001},	"Beat Cal" },
	{ {11,3,2001},	"Lost to USC (OT)" },
	{ {11,10,2001},	"Beat Washington" },
	{ {11,17,2001},	"Beat N. Arizona" },
	{ {12,1,2001},	"Lost to Oregon" }
};

date_t easterDates[] = {
	{4, 15, 2001},
	{3, 31, 2002},
	{4, 20, 2003},
	{4, 11, 2004},
	{3, 27, 2005},
	{4, 16, 2006},
	{4, 8,  2007},
	{3, 23, 2008},
	{4, 12, 2009},
	{4, 4,  2010}
};

int exitStatus = 0;

static void
checkStatus(int status, int evty, const char *funcName)
{
	if (status != 0) {
		fprintf(stderr, "write of event type %d failed:\n", evty);
		errno = status;
		perror(funcName);
		exitStatus = 1;
	}
}

main(int argc, char **argv)
{
	int status;
	posix_log_facility_t facility;
	int type;
	posix_log_severity_t severity;
	
	facility = LOG_USER;
	severity = LOG_INFO;
	
	type = 4001;
	status = posix_log_write(facility, type, severity,
		osuFbGames01, sizeof(osuFbGames01), POSIX_LOG_BINARY, 0);
	checkStatus(status, 4001, "posix_log_write:");
	
	type = 4002;
	status = evl_log_write(facility, type, severity, 0,
		"string", "Dates of Easter, 2001-2010",
		"int", sizeof(easterDates)/sizeof(date_t),
		"char[]", sizeof(easterDates), easterDates,
		"endofdata");
	checkStatus(status, 4002, "evl_log_write:");
	
	type = 4003;
	status = evl_log_write(facility, type, severity, 0,
		"string", "Filesystem full",
		"endofdata");
	checkStatus(status, 4003, "evl_log_write:");

	exit(exitStatus);
}
