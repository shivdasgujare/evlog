/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2001
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>

#include "config.h"

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"

/*
 * These are the facilities defined by Linux's sys/syslog.h.
 * The facility registry must always contain at least these.
 */
struct _evlNvPair _evlLinuxFacilities[] = {
	{ LOG_AUTH,	"AUTH" },
	{ LOG_CRON,	"CRON" },
	{ LOG_DAEMON,	"DAEMON" },
	{ LOG_KERN,	"KERN" },
	{ LOG_LPR,	"LPR" },
	{ LOG_MAIL,	"MAIL" },
	{ LOG_NEWS,	"NEWS" },
	{ LOG_SYSLOG,	"SYSLOG" },
	{ LOG_USER,	"USER" },
	{ LOG_LOGMGMT,	"LOGMGMT" },

	{ LOG_UUCP,	"UUCP" },
	{ LOG_AUTHPRIV,	"AUTHPRIV" },
	{ LOG_FTP,	"FTP" },

	{ LOG_LOCAL0,	"LOCAL0" },
	{ LOG_LOCAL1,	"LOCAL1" },
	{ LOG_LOCAL2,	"LOCAL2" },
	{ LOG_LOCAL3,	"LOCAL3" },
	{ LOG_LOCAL4,	"LOCAL4" },
	{ LOG_LOCAL5,	"LOCAL5" },
	{ LOG_LOCAL6,	"LOCAL6" },
	{ LOG_LOCAL7,	"LOCAL7" },
	{ 0, 0 }
};

/*
 * These are the facilities defined by POSIX 1003.25.
 */
struct _evlNvPair _evlPosixFacilities[] = {
	{ LOG_AUTH,	"AUTH" },
	{ LOG_CRON,	"CRON" },
	{ LOG_DAEMON,	"DAEMON" },
	{ LOG_KERN,	"KERN" },
	{ LOG_LPR,	"LPR" },
	{ LOG_MAIL,	"MAIL" },
	{ LOG_NEWS,	"NEWS" },
	{ LOG_SYSLOG,	"SYSLOG" },
	{ LOG_USER,	"USER" },
	{ LOG_LOGMGMT,	"LOGMGMT" },
	{ LOG_UUCP,	"UUCP" },
	{ LOG_LOCAL0,	"LOCAL0" },
	{ LOG_LOCAL1,	"LOCAL1" },
	{ LOG_LOCAL2,	"LOCAL2" },
	{ LOG_LOCAL3,	"LOCAL3" },
	{ LOG_LOCAL4,	"LOCAL4" },
	{ LOG_LOCAL5,	"LOCAL5" },
	{ LOG_LOCAL6,	"LOCAL6" },
	{ LOG_LOCAL7,	"LOCAL7" },
	{ 0, 0 }
};

struct _evlNvPair _evlSeverities[] = {
	{ LOG_EMERG,	"EMERG" },
	{ LOG_ALERT,	"ALERT" },
	{ LOG_CRIT,	"CRIT" },
	{ LOG_ERR,	"ERR" },
	{ LOG_WARNING,	"WARNING" },
	{ LOG_NOTICE,	"NOTICE" },
	{ LOG_INFO,	"INFO" },
	{ LOG_DEBUG,	"DEBUG" },
	{ 0, 0 }
};

struct _evlNvPair _evlAttributes[] = {
	{ POSIX_LOG_ENTRY_RECID,	"recid" },
	{ POSIX_LOG_ENTRY_SIZE,	"size" },
	{ POSIX_LOG_ENTRY_FORMAT,	"format" },
	{ POSIX_LOG_ENTRY_EVENT_TYPE,	"event_type" },
	{ POSIX_LOG_ENTRY_FACILITY,	"facility" },
	{ POSIX_LOG_ENTRY_SEVERITY,	"severity" },
	{ POSIX_LOG_ENTRY_UID,	"uid" },
	{ POSIX_LOG_ENTRY_GID,	"gid" },
	{ POSIX_LOG_ENTRY_PID,	"pid" },
	{ POSIX_LOG_ENTRY_PGRP,	"pgrp" },
	{ POSIX_LOG_ENTRY_TIME,	"time" },
	{ POSIX_LOG_ENTRY_FLAGS,	"flags" },
	{ POSIX_LOG_ENTRY_THREAD,	"thread" },
	{ POSIX_LOG_ENTRY_PROCESSOR,	"processor" },
	{ 0, 0 }
};

struct _evlNvPair _evlFormats[] = {
	{ POSIX_LOG_NODATA,	"NODATA" },
	{ POSIX_LOG_BINARY,	"BINARY" },
	{ POSIX_LOG_STRING,	"STRING" },
	{ POSIX_LOG_PRINTF,	"PRINTF" },
	{ 0, 0 }
};

struct _evlNvPair _evlMgmtEventTypes[] = {
	{ POSIX_LOG_MGMT_TIMEMARK,		"MGMT_TIMEMARK" },
	{ POSIX_LOG_MGMT_STARTMAINT,	"MGMT_STARTMAINT" },
	{ POSIX_LOG_MGMT_ENDMAINT,		"MGMT_ENDMAINT" },
	{ POSIX_LOG_MGMT_STOPLOGGING,	"MGMT_STOPLOGGING" },
	{ POSIX_LOG_MGMT_STARTLOGGING,	"MGMT_STARTLOGGING" },
	{ 0, 0 }
};

void
_evlGetNameByValue(const struct _evlNvPair table[], int value, char *name, size_t size,
	const char *dflt)
{
	const struct _evlNvPair *nv;
	for (nv = table; nv->nv_name; nv++) {
		if (nv->nv_value == value) {
			(void) strcpy(name, nv->nv_name);
			return;
		}
	}

	/* No match */
	if (dflt) {
		(void) strcpy(name, dflt);
	} else {
		snprintf(name, size, "%d", value);
	}
}

/* Look up value by name, case-sensitive */
int
_evlGetValueByName(const struct _evlNvPair table[], const char *name, int dflt)
{
	const struct _evlNvPair *nv;
	for (nv = table; nv->nv_name; nv++) {
		if (!strcmp(nv->nv_name, name)) {
			return nv->nv_value;
		}
	}

	/* No match */
	return dflt;
}

/* Like strcmp, but case-insensitive */
int
_evlCIStrcmp(const char *s1, const char *s2)
{
	while (*s1 && toupper(*s1) == toupper(*s2)) {
		s1++;
		s2++;
	}
	return toupper(*s1) - toupper(*s2);
}

/* Look up value by name, case-insensitive */
int
_evlGetValueByCIName(const struct _evlNvPair table[], const char *name, int dflt)
{
	const struct _evlNvPair *nv;
	for (nv = table; nv->nv_name; nv++) {
		if (!_evlCIStrcmp(nv->nv_name, name)) {
			return nv->nv_value;
		}
	}

	/* No match */
	return dflt;
}

/* Return the proc id */
int 
_evlGetProcId()
{
	int fd, ret;
	char buf[1024];
	char *last_token;
	size_t r;
	
	if ((fd = open("/proc/self/stat", O_RDONLY)) == -1) {
		fprintf(stderr, "Can't open stat\n");
		exit(1);
	}

	if ((r = read(fd, buf, 1024)) == -1) {
		close(fd);
		fprintf(stderr, "Read stat failed\n");
		exit(1);
	}
	buf[r]='\0';
	close(fd);
	/* The last token contains the processor id */

	last_token = (char *) strrchr(buf, ' ');
	assert(last_token != NULL);	
	return (int) strtol(last_token, (char **)NULL, 10);
}
