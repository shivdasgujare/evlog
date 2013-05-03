/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2002
 *
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
#include <sys/select.h>
#include <sys/time.h>
#include "posix_evlog.h"
#include "posix_evlsup.h"

/* Global variables */
posix_log_facility_t facility;
int type;
posix_log_severity_t severity;

/* Function prototypes */
void runTest(int nevents, int niterations, long seconds, long microseconds);

int main (int argc, char **argv)
{
	int i, k;
	int test_type, nevents, niterations;
	long seconds = 0, microseconds = 0;

	facility = 128;			/* LOCAL0 */
	severity = 3;
	type = 2000;
	nevents = atoi(argv[1]);
	niterations = atoi(argv[2]);

	if (argc > 3)
		seconds = atol(argv[3]);

	if (argc > 4)
		microseconds = atol(argv[4]);

	runTest(nevents, niterations, seconds, microseconds);

	exit(0);
}

void wait(long seconds, long microseconds) {
	struct timeval t = {seconds, microseconds};
	if (seconds == 0 && microseconds == 0)
		return;
	select(0, NULL, NULL, NULL, &t);
}

/* This test just alternates between each type of event.
 * Example: abcabcabc
 */
void
runTest(int nevents, int niterations, long seconds, long microseconds) {
	int i, j;
	char s[255];

	for (i = 0; i < niterations; i++)
		for (j = 1; j <= nevents; j++) {
			snprintf(s, 255, "Test duplicate #%d", j);
			posix_log_printf(facility, type+j, severity, 0, "%s", s);
			wait(seconds, microseconds);
		}
}
