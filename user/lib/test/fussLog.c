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
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"

/*
 * Copy an event log from the indicated file to stdout, fussing records
 * according to the numeric code specified as argv[2].  The significance
 * of any other args depends on the type of fussing to be done.
 *
 */

/*
Fuss code 1: Fix up the log I got from Haren:
- Change the uid to be one of: root, bin, or daemon (0, 1, or 2).
- Change the cpu to be in the range 0-7.
- Change the date to be something in the last 10 days.
- Change the format of every 4th record to be POSIX_LOG_BINARY.
- Change the format of every 10th record to be POSIX_LOG_NODATA.

Fuss code 2: Switch from the late-May 8-byte log header to whatever it is now.
 */

static void initialize(int code);
static int fussRec(int code, struct posix_log_entry *entry, char *buf);
static time_t now, then;

struct posix_log_entry entry;
char buf[POSIX_LOG_ENTRY_MAXLEN];
posix_logd_t logdesc;

main(int argc, char **argv)
{
	int status;
	int fusscode;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s log fusscode ... > newlog\n", argv[0]);
		exit(1);
	}
	fusscode = atoi(argv[2]);

	if (fusscode == 2) {
		fprintf(stderr,
"Fuss code 2 no longer supported, since a log descriptor is no longer\n"
"just a file descriptor.\n");
		exit(1);
#if 0
		logdesc = open(argv[1], O_RDONLY|O_NONBLOCK);
		if (logdesc < 0) {
			perror(argv[1]);
			exit(1);
		}
		if (lseek(logdesc, 8, SEEK_SET) == (off_t)-1) {
			perror("lseek past old log header");
			exit(1);
		}
#endif
	} else {
		status = posix_log_open(&logdesc, argv[1]);
		if (status != 0) {
			errno = status;
			perror(argv[1]);
			exit(1);
		}
	}

	if (_evlWriteLogHeader(1) != 0) {
		perror("_evlWriteLogHeader");
		exit(1);
	}

	initialize(fusscode);

	for (;;) {
		int writeRec;
		status = posix_log_read(logdesc, &entry, buf, POSIX_LOG_ENTRY_MAXLEN);
		if (status == EAGAIN) {
			/* No more records. */
			exit(0);
		} else if (status != 0) {
			errno = status;
			perror("posix_log_read");
			exit(1);
		}

		writeRec = fussRec(fusscode, &entry, buf);
		if (writeRec) {
			if (_evlFdWrite(1, &entry, buf) < 0) {
				perror("_evlFdWrite");
				exit(1);
			}
		}
	}
	/*NOTREACHED*/
}

static void
initialize(int code)
{
	if (code == 1 ) {
		now = time(NULL);
		then = now - 10*24*60*60;
	}
}

static int
fussRec(int code, struct posix_log_entry *entry, char *buf)
{
	int recid = entry->log_recid;

	if (code != 1) {
		return 1;
	}
	entry->log_uid = recid % 3;
	entry->log_processor = recid % 8;
	entry->log_time.tv_sec = then + recid;
	if (recid % 4 == 0) {
		entry->log_format = POSIX_LOG_BINARY;
	} else if (recid % 10 == 0) {
		entry->log_format = POSIX_LOG_NODATA;
		entry->log_size = 0;
	}

	then += 60*60;
	if (then > now) {
		then = now;
	}
	return 1;
}
