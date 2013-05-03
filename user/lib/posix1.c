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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_common.h"
#include "query/evl_parse.h"

#include "evlog.h"

#define GETPW_R_SIZE_MAX 128
#define GETGR_R_SIZE_MAX 128

/*
 * posix1.c implements the functions in the POSIX 1003.25 Event Logging API.
 * posix2.c provides support for these functions.  In particular, the functions
 * that know about the internal structure of the event log are found in
 * posix2.c.
 */

const char *evl_time_format = "%c";

static int nOpens = 0;

static posix_log_query_t screenQuery;		/* Screening query specify in the evlog.conf */
static time_t evlConfLastMod;				/* time stamp fro evlog.conf */
static char *confPath = LOG_EVLOG_CONF_DIR "/evlog.conf";

#ifdef _POSIX_THREADS
static pthread_mutex_t nOpensMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t qParserMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t nfyApiMutex =  PTHREAD_MUTEX_INITIALIZER;
#endif


/*************** entry end exit point for shared lib ******************/
#ifdef _SHARED_LIB
void _init() 
{
}

void _fini()
{
}
#endif
/********************** auxiliary functions ***************************/

static void
lockNOpens()
{
#ifdef _POSIX_THREADS
	(void) pthread_mutex_lock(&nOpensMutex);
#endif
}

static void
unlockNOpens()
{
#ifdef _POSIX_THREADS
	(void) pthread_mutex_unlock(&nOpensMutex);
#endif
}

static void
lockNfyApi()
{
#ifdef _POSIX_THREADS
	(void) pthread_mutex_lock(&nfyApiMutex);
#endif
}

static void
unlockNfyApi()
{
#ifdef _POSIX_THREADS
	(void) pthread_mutex_unlock(&nfyApiMutex);
#endif
}

/* Used for building strings like "0x3 (TRUNCATE|STRING)". */
static int
addFlag(char *s, unsigned int flags, unsigned int flag, const char *flagName,
	int anyPrevFlags)
{
	if ((flags & flag) != 0) {
		if (anyPrevFlags) {
			(void) strcat(s, "|");
		} else {
			(void) strcat(s, " (");
		}
		(void) strcat(s, flagName);
		return 1;
	} else {
		return anyPrevFlags;
	}
}

/********************** query functions *************************/
int
_evlEvaluateQuery(const posix_log_query_t *query,
	const struct posix_log_entry *entry, const void *buf)
{
	return _evlQEvaluateTree((const pnode_t *)(query->qu_tree),
		query->qu_nonStdAtts, entry, buf);
}	

static void
lockQueryParser()
{
#ifdef _POSIX_THREADS
	(void) pthread_mutex_lock(&qParserMutex);
#endif
}

static void
unlockQueryParser()
{
#ifdef _POSIX_THREADS
	(void) pthread_mutex_unlock(&qParserMutex);
#endif
}

int tryLockQueryParser()
{
#ifdef _POSIX_THREADS
	return pthread_mutex_trylock(&qParserMutex);
#else
	return 0;
#endif
} 		

/*
 * Called by posix_log_query_create() to clean up after a syntax or
 * semantic error.
 */
static void
queryError(const char *errString, char *errBuf, size_t bufsz)
{
	size_t eslen = strlen(errString);
	if (errBuf && bufsz != 0) {
		/* Copy as much as possible of errString into errBuf. */
		if (eslen+1 <= bufsz) {
			(void) strcpy(errBuf, errString);
		} else {	
			(void) strncpy(errBuf, errString, bufsz);
			errBuf[bufsz-1] = '\0';
		}
	}

	if (_evlQParseTree) {
		_evlQFreeTree(_evlQParseTree);
		_evlQParseTree = NULL;
	}	
	if (_evlQNonStdAtts) {
		_evlQFreeNonStdAtts(_evlQNonStdAtts);
		_evlQNonStdAtts = NULL;
	}
	unlockQueryParser();
}


/********************** API functions ***************************/

/* Log an event */
int
posix_log_write(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity,
	const void *buf, size_t len, int format, unsigned int flags)
{
	struct posix_log_entry entry;
	posix_log_query_t *restrictedQuery;
	int accFlags;
	int gotTimeStamp = 0;
	int writeStatus;
	char facName[POSIX_LOG_MEMSTR_MAXLEN];
	struct stat st;

	/* Capture the time stamp ASAP. */
#ifdef _POSIX_TIMERS_1
	if (clock_gettime(CLOCK_REALTIME, &(entry.log_time)) == 0) {
		gotTimeStamp = 1;
	}
#endif
	if (!gotTimeStamp) {
		entry.log_time.tv_sec = (long) time(0);
		entry.log_time.tv_nsec = 0;
	}
	/* Temporary mark the log_magic with the architecture info */
#ifdef __i386__
	entry.log_magic = LOGREC_ARCH_I386;
#elif defined(__s390__)
	entry.log_magic = LOGREC_ARCH_S390;
#elif defined(__s390x__)
	entry.log_magic = LOGREC_ARCH_S390X;
#elif defined(__ppc__)
	entry.log_magic = LOGREC_ARCH_PPC;
#elif defined(__powerpc__)
	entry.log_magic = LOGREC_ARCH_PPC;
#elif defined(__ia64__)
	entry.log_magic = LOGREC_ARCH_IA64;
#elif defined(__x86_64__)
	entry.log_magic = LOGREC_ARCH_X86_64;
#elif defined(__arm__) && defined(__ARMEB__)
	entry.log_magic = LOGREC_ARCH_ARM_BE;
#elif defined(__arm__) && !defined(__ARMEB__)
	entry.log_magic = LOGREC_ARCH_ARM_LE;
#else
	entry.log_magic = LOGREC_ARCH_NO_ARCH;
#endif
	
	/*
	 * Also get the processor ID ASAP, before this process migrates
	 * to another processor.
	 *
	 * Linux, at least, doesn't appear to have a user-mode function
	 * for getting the ID of the current CPU.  In the kernel, there's
	 * the smp_processor_id macro. Therefore, we invent our own.
	 */
	entry.log_processor = _evlGetProcId();

	/*
	 * If format is POSIX_LOG_STRING, verify that the caller really
	 * passed us a string of the indicated length.  Do other format
	 * fussing later.
	 */
	if (format == POSIX_LOG_STRING) {
		if (strlen((const char*)buf) != len-1) {
			return EBADMSG;
		}
	}

	/* Lowest number (0) = highest severity (LOG_EMERG) */
	if (severity < LOG_EMERG || severity > LOG_DEBUG) {
		return EINVAL;
	}

	/* Reject any facility code that isn't in the registry. */
	if (_evlGetFacilityAccess(facility) == -1) {
		return EINVAL;
	}
	
	if (buf == 0) {
		len = 0;
	} else if (len > POSIX_LOG_ENTRY_MAXLEN) {
#ifdef POSIX_LOG_TRUNCATE
		len = POSIX_LOG_ENTRY_MAXLEN;
		flags |= POSIX_LOG_TRUNCATE;
#else
		return EMSGSIZE;
#endif
	}

	/* Verify that the specified format is appropriate. */
	if (len == 0) {
		/* No data.  Format is NODATA, no matter what caller says. */
		entry.log_format = POSIX_LOG_NODATA;
	} else {
		if (format == POSIX_LOG_BINARY
		    || format == POSIX_LOG_STRING
		    || format == POSIX_LOG_PRINTF) {
			entry.log_format = format;
		} else {
			return EINVAL;
		}
	}

	entry.log_flags = flags;
	entry.log_size = len;

	entry.log_event_type = event_type;
	entry.log_facility = facility;
	entry.log_severity = severity;

	entry.log_uid = geteuid();
	entry.log_gid = getegid();
	entry.log_pid = getpid();
	entry.log_pgrp = getpgrp();

#ifdef _POSIX_THREADS
	entry.log_thread = pthread_self();
#else
	entry.log_thread = 0;
#endif
	/* entry.log_recid is set later by somebody else. */
	writeStatus = _evlWriteEx(&entry, buf);

	if (writeStatus == ENOSPC) {
		return ENOSPC;
	} else if (writeStatus != 0) {
		return EIO;
	}
	return 0;
}

int
posix_log_vprintf(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity, unsigned int flags,
	const char *format, va_list args)
{
	char buf[POSIX_LOG_ENTRY_MAXLEN];
	int slen;

	if (!format) {
		return EINVAL;
	}
	slen = vsnprintf(buf, POSIX_LOG_ENTRY_MAXLEN, format, args);

	if (slen < 0 || slen > POSIX_LOG_ENTRY_MAXLEN-1) {
		/* String would exceed POSIX_LOG_ENTRY_MAXLEN; truncate it. */
		buf[POSIX_LOG_ENTRY_MAXLEN-1] = '\0';
#ifdef POSIX_LOG_TRUNCATE
		flags |= POSIX_LOG_TRUNCATE;
#endif
	}

	return posix_log_write(facility, event_type, severity,
		buf, strlen(buf)+1, POSIX_LOG_STRING, flags);
}

int
posix_log_printf(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity, unsigned int flags,
	const char *format, ...)
{
	int status;
	va_list args;

	va_start(args, format);
	status = posix_log_vprintf(facility, event_type, severity, flags,
		format, args);
	va_end(args);
	return status;
}

int
posix_log_vprintb(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity, unsigned int flags,
	const char *format, va_list args)
{
	char buf[POSIX_LOG_ENTRY_MAXLEN];
	size_t datasz = 0;
	int status;

	if (!format) {
		return EINVAL;
	}
	status = evl_pack_format_and_args(format, args, buf, &datasz);
	if (status != 0) {
		return status;
	}

	return posix_log_write(facility, event_type, severity,
		buf, datasz, POSIX_LOG_PRINTF, flags);
}

int
posix_log_printb(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity, unsigned int flags,
	const char *format, ...)
{
	int status;
	va_list args;

	va_start(args, format);
	status = posix_log_vprintb(facility, event_type, severity, flags,
		format, args);
	va_end(args);
	return status;
}

/* Open an event log for read access */
int
posix_log_open(posix_logd_t *logdes, const char *path)
{
	int status = 0;

	lockNOpens();
	if (nOpens >= POSIX_LOG_OPEN_MAX) {
		unlockNOpens();
		return EMFILE;
	}
	nOpens++;
	unlockNOpens();

	if ((status = _evlOpen(logdes, path)) != 0) {
		/* Not a successful open after all. */
		lockNOpens();
		nOpens--;
		unlockNOpens();
	}
	return status;
}

/* Read from event log */
int
posix_log_read(posix_logd_t logdes, struct posix_log_entry *entry,
	void *log_buf, size_t log_len)
{
	if (_evlValidateLogdes(logdes, 1) < 0) {
		return EBADF;
	}

	if (entry == 0) {
		return EINVAL;
	}

	return _evlRead(logdes, entry, log_buf, log_len);
} 

/* Close event log */
int
posix_log_close(posix_logd_t logdes)
{
	int status;

	if (_evlValidateLogdes(logdes, 0) < 0) {
		return EBADF;
	}

	if ((status = _evlClose(logdes)) == 0) {
		lockNOpens();
		nOpens--;
		unlockNOpens();
	}
	return status;
}

/* Reposition the read pointer */
int
posix_log_seek(posix_logd_t logdes, const posix_log_query_t *query,
	int direction)
{
	int status;

	if (_evlValidateLogdes(logdes, 1) < 0) {
		return EBADF;
	}
	if (_evlValidateQuery(query, 1 /* NULL OK */) < 0) {
		return EINVAL;
	}
	
	switch (direction) {
	default:
		return EINVAL;
	case POSIX_LOG_SEEK_START:
		if (query) {
			return EINVAL;
		}	
		return _evlRewind(logdes);	
	case POSIX_LOG_SEEK_END:
		if (query) {
			return EINVAL;
		}	
		return _evlSeekEnd(logdes);
	case POSIX_LOG_SEEK_FIRST:	
		if ((status = _evlRewind(logdes)) < 0) {
			return status;
		}
		if (query) {
			return _evlSeekFwd(logdes, query);
		}
		return 0;
	case POSIX_LOG_SEEK_LAST:
		if ((status = _evlSeekEnd(logdes)) < 0) {
			return status;
		}
		return _evlSeekBkwd(logdes, query);
	case POSIX_LOG_SEEK_FORWARD:
		return _evlSeekFwd(logdes, query);
	case POSIX_LOG_SEEK_BACKWARD:
		return _evlSeekBkwd(logdes, query);
	}

	/* NOTREACHED */
	return EINVAL;
} 

/* Compare severities */
int
posix_log_severity_compare(int *order, posix_log_severity_t s1,
	posix_log_severity_t s2)
{
	if (!order) {
		return EINVAL;
	}

	/* Lowest number (0) = highest severity (LOG_EMERG) */
	*order = s2 - s1;
	return 0;
} 

/* Create log query */
int
posix_log_query_create(const char *query_string, int purpose,
	posix_log_query_t *query, char *errbuf, size_t errlen)
{
	int status = 0;

	if (!query_string || !query) {
		status = EINVAL;
	}

	if (purpose == 0 ||
	    (purpose & ~POSIX_LOG_PRPS_MASK) != 0) {
		status = EINVAL;
	}
	if ((purpose & EVL_PRPS_TEMPLATE) != 0) {
		/*
		 * Can't refer to non-standard attributes in a query used
		 * for notification or restricted logging.
		 */
		if ((purpose & (POSIX_LOG_PRPS_NOTIFY | EVL_PRPS_RESTRICTED))
		    != 0) {
			status = ENOTSUP;
		}
	}

	if (status != 0) {
		/*
		 * POSIX 1003.25 is unclear about the following...
		 * Caller might expect something useful in errbuf, although
		 * he's not entitled to it at this point.  At least store a
		 * null string.
		 *
		 * Note that POSIX does NOT allow us to touch errbuf if we
		 * succeed.
		 */
		if (errbuf && errlen > 0) {
			errbuf[0] = '\0';
		}
		return status;
	}

	if (tryLockQueryParser() == EBUSY) {
		/*
		 * We should get here only if the query refs a facility name,
		 * this causes a read of the facility registry, and in
		 * reading the facility registry we try to parse one or more
		 * queries contained therein.  See facreg.c.  So Joe User
		 * should never see EBUSY from this function.
		 */
		return EBUSY;
	}
	_evlQReinitLex(query_string);	
	_evlQParseTree = NULL;
	_evlQueryErrmsg[0] = '\0';
	_evlQFlags = purpose;
	_evlQNonStdAtts = NULL;
	status = qqparse();
	_evlQEndLex();
	if (status != 0) {
		/* Syntax error(s) */
		queryError(_evlQueryErrmsg, errbuf, errlen);
		return EINVAL;
	} else if (_evlQNormalizeTree(_evlQParseTree) != 0) {
		/* Semantic error(s) */
		queryError(_evlQueryErrmsg, errbuf, errlen);
		return EINVAL;
	}	

	_evlQOptimizeTree(_evlQParseTree, _evlQNonStdAtts);

	query->qu_tree = _evlQParseTree;
	_evlQParseTree = NULL;
	query->qu_nonStdAtts = _evlQNonStdAtts;
	_evlQNonStdAtts = NULL;
	unlockQueryParser();
	query->qu_expr = strdup(query_string);
	query->qu_purpose = purpose;
	return 0;
}

int
posix_log_query_get(const posix_log_query_t *query, int *purpose,
	char *qsbuf, size_t qslen, size_t *reqlen)
{
	size_t qelen;
	if (_evlValidateQuery(query, 0 /* NULL not OK */) < 0) {
		return EINVAL;
	}	
	
	/* Provide length of query expression even if there's not sufficient
	 * room to store it.
	 */
	qelen = strlen(query->qu_expr) + 1;
	if (reqlen) {
		*reqlen = qelen;
	}
	if (!qsbuf) {
		return EINVAL;
	}
	if (qslen < qelen) {
		return EMSGSIZE;
	}
	(void) strcpy(qsbuf, query->qu_expr);
	if (purpose) {
		*purpose = query->qu_purpose;
	}
	return 0;
}

/* Destroy log query */
int
posix_log_query_destroy(posix_log_query_t *query)
{
	if (_evlValidateQuery(query, 0 /* NULL not OK */) < 0) {
		return EINVAL;
	}	
	if (query->qu_tree) {
		_evlQFreeTree(query->qu_tree);
	}
	if (query->qu_nonStdAtts) {
		_evlQFreeNonStdAtts(query->qu_nonStdAtts);
	}
	if (query->qu_expr) {
		free(query->qu_expr);
	}
	query->qu_purpose = 0;	
	return 0;
}

/* Obtain the string equivalent of an event-record member */
int
posix_log_memtostr(int member_selector,
	const struct posix_log_entry *entry,
	char *buf, size_t buflen)
{
	char s[POSIX_LOG_MEMSTR_MAXLEN];

	if (entry == 0 || buf == 0) {
		return EINVAL;
	}

	switch (member_selector) {
	case POSIX_LOG_ENTRY_RECID:
		(void) snprintf(s, sizeof(s), "%d", entry->log_recid);
		break;
	case POSIX_LOG_ENTRY_SIZE:
		(void) snprintf(s, sizeof(s), "%u", entry->log_size);
		break;
	case POSIX_LOG_ENTRY_FORMAT:
		_evlGetNameByValue(_evlFormats, entry->log_format, s, sizeof(s), 0);
		break;
	case POSIX_LOG_ENTRY_EVENT_TYPE:
		(void) snprintf(s, sizeof(s), "0x%x", entry->log_event_type);
		break;
	case POSIX_LOG_ENTRY_FACILITY:
		if (!_evlGetFacilityName(entry->log_facility, s)) {
			(void) snprintf(s, sizeof(s), "0x%x", entry->log_facility);
		}
		break;
	case POSIX_LOG_ENTRY_SEVERITY:
		_evlGetNameByValue(_evlSeverities, entry->log_severity, s, sizeof(s), 0);
		break;
	case POSIX_LOG_ENTRY_UID:
	    {
#ifdef _POSIX_THREAD_SAFE_FUNCTIONS
		struct passwd *pw, passwd;
		char buf[GETPW_R_SIZE_MAX];
		/* If host is a rmt host - don't resolve the user name */
		if ((entry->log_processor >> 16) != 0) goto _rmt_user_exit; 
#ifndef __Lynx__
		(void) getpwuid_r(entry->log_uid, &passwd, buf,
			GETPW_R_SIZE_MAX, &pw);
#else
		pw = getpwuid_r(&passwd, entry->log_uid, buf, GETPW_R_SIZE_MAX);
#endif
#else
	    	struct passwd *pw;
		/* If host is a rmt host - don't resolve the user name */
		if ((entry->log_processor >> 16) != 0) goto _rmt_user_exit; 
		pw = getpwuid(entry->log_uid);
#endif
		if (pw) {
			(void) strcpy(s, pw->pw_name);
		} else {
			(void) snprintf(s, sizeof(s), "%u", entry->log_uid);
		}
	_rmt_user_exit:	/* keep gcc happy */ ;
	    }
		break;
	case POSIX_LOG_ENTRY_GID:
	    {
#ifdef _POSIX_THREAD_SAFE_FUNCTIONS
		struct group *gr, group;
		char buf[GETGR_R_SIZE_MAX];
#ifndef __Lynx__
		(void) getgrgid_r(entry->log_gid, &group, buf,
			GETGR_R_SIZE_MAX, &gr);
#else 
		gr = getgrgid_r(&group, entry->log_gid, buf, GETGR_R_SIZE_MAX);
#endif
#else
	    	struct group *gr = getgrgid(entry->log_gid);
#endif
		if (gr) {
			(void) strcpy(s, gr->gr_name);
		} else {
			(void) snprintf(s, sizeof(s), "%u", entry->log_gid);
		}
	    }
		break;
	case POSIX_LOG_ENTRY_PID:
		(void) snprintf(s, sizeof(s), "%d", entry->log_pid);
		break;
	case POSIX_LOG_ENTRY_PGRP:
		(void) snprintf(s, sizeof(s), "%d", entry->log_pgrp);
		break;
	case POSIX_LOG_ENTRY_TIME:
	    {
		char *nl;
		time_t timestamp = (time_t) entry->log_time.tv_sec;
		struct tm tm;
		(void) strftime(s, POSIX_LOG_MEMSTR_MAXLEN, evl_time_format,
			localtime_r(&timestamp, &tm));
		/* Zap newline. */
		nl = strchr(s, '\n');
		if (nl) {
			*nl = '\0';
		}
	    }
		break;
	case POSIX_LOG_ENTRY_FLAGS:
	    {
		int anyFlags = 0;
		(void) snprintf(s, sizeof(s), "0x%x", entry->log_flags);
		anyFlags = addFlag(s, entry->log_flags, EVL_KERNEL_EVENT,
			"KERNEL", anyFlags);
#ifdef EVL_PRINTK
		anyFlags = addFlag(s, entry->log_flags, EVL_PRINTK,
			"PRINTK", anyFlags);
#endif
#ifdef EVL_INTERRUPT
		anyFlags = addFlag(s, entry->log_flags, EVL_INTERRUPT,
			"INTERRUPT", anyFlags);
#endif
		anyFlags = addFlag(s, entry->log_flags, POSIX_LOG_TRUNCATE,
			"TRUNCATE", anyFlags);
		if (anyFlags) {
			(void) strcat(s, ")");
		}
		break;
	    }
	case POSIX_LOG_ENTRY_THREAD:
		(void) snprintf(s, sizeof(s), "0x%x", entry->log_thread);
		break;
	case POSIX_LOG_ENTRY_PROCESSOR:
		(void) snprintf(s, sizeof(s), "%d", entry->log_processor);
		break;
	default:
		return EINVAL;
	}

	if (strlen(s)+1 > buflen) {
		return EMSGSIZE;
	}
	(void) strcpy(buf, s);
	return 0;
}

/* Convert string to facility or facility to string */
int
posix_log_factostr(posix_log_facility_t fac, char *buf,
	size_t buflen)
{
	/* This is sort of backward -- posix_log_memtostr() should probably
	 * call factostr() -- but posix_log_memtostr() was written first.
	 */
	struct posix_log_entry entry;
	entry.log_facility = fac;
	if (_evlGetFacilityAccess(fac) == -1) {
		return EINVAL;
	}
	return posix_log_memtostr(POSIX_LOG_ENTRY_FACILITY, &entry, buf, buflen);
}	

int
posix_log_strtofac(const char *str, posix_log_facility_t *fac)
{
	int facCode;

	if (!str || !fac) {
		return EINVAL;
	}
	facCode = _evlGetFacilityCodeByCIName(str);
	if (facCode == EVL_INVALID_FACILITY) {
		return EINVAL;
	}
	*fac = facCode;
	return 0;
}	

int
posix_log_query_match(const posix_log_query_t *query,
	const struct posix_log_entry *entry, const void *buf, int *match)
{
	if (!entry || !match) {
		return EINVAL;
	}
	if ( (entry->log_size == 0 && buf != NULL)
	    || (entry->log_size != 0 && buf == NULL) ) {
		return EINVAL;
	}
	if (_evlValidateQuery(query, 0 /* NULL not OK */) < 0) {
		return EINVAL;
	}
	*match = (_evlEvaluateQuery(query, entry, buf) != 0);
	return 0;
}
/*
 * Notification related API
 */
int 
posix_log_notify_add(const posix_log_query_t *query,
        const struct sigevent *notification, int flags,
        posix_log_notify_t *nfyhandle)
{
	nfyNewRqMsg_t newReq;
	int ret=0;
	nfyMsgHdr_t resp;
	sigset_t oldset;
	int sigsBlocked;
	int _evl_nfy_sd = -1;
	struct sockaddr_un _evl_nfy_sock;
	int query_flags;
	
	if (!notification) {
		return EINVAL;
	}

	/* Validate fields of notification. */
	if (notification->sigev_notify == SIGEV_THREAD) {
		/* TODO: Support threads-based notification. */
	 	return ENOTSUP;
	} else if (notification->sigev_notify != SIGEV_SIGNAL) {
	 	return EINVAL;
	}
	if (flags > (POSIX_LOG_ONCE_ONLY | POSIX_LOG_SEND_RECID | POSIX_LOG_SEND_SIGVAL) ) {
		return EINVAL;
	}
	if(!(flags & POSIX_LOG_SEND_RECID) && !(flags & POSIX_LOG_SEND_SIGVAL)){
		flags |= POSIX_LOG_SEND_SIGVAL;
	}
	/* Enter critical section */
	lockNfyApi();
	/* Mask all signals so we don't get interrupted */
	sigsBlocked = (_evlBlockSignals(&oldset) == 0);
	
	/*
	 * Send the header to the server:
	 */
	newReq.nwHeader.nhMsgType = nmtNewRequest;
	newReq.nwQueryLength = 0;
	
	/* Validate query -- NULL is OK. */
	if (query) {
		if(_evlValidateQuery(query, 1) == -1) {
			ret= EINVAL;
			goto err_exit;
		}
		if (((query->qu_purpose & POSIX_LOG_PRPS_NOTIFY) == 0) &&
		    ((query->qu_purpose & POSIX_LOG_PRPS_GENERAL) == 0) &&
		    ((query->qu_purpose & EVL_PRPS_TEMPLATE) == 0)) {
			ret = ENOTSUP;
			goto err_exit;
		}

		query_flags = query->qu_purpose;
		/*
		if ((query->qu_purpose & POSIX_LOG_PRPS_NOTIFY) == 0) {	
			ret = ENOTSUP;
			goto err_exit;
		}
		*/
		newReq.nwQueryLength = strlen(query->qu_expr);
	} else {
		query_flags = 0;
		newReq.nwQueryLength = strlen("<null>");
	}
	
	
	if ((_evl_nfy_sd = _establishNonBlkConnection(EVLNOTIFYD_SERVER_SOCKET, &_evl_nfy_sock, 3 /*timeout*/)) < 0) {
		ret = EAGAIN;
		goto err_exit;	/* convert error code back to positive number */
	}
	if (isAccess(_evl_nfy_sd) == -1 ) {
		ret = EPERM;
		goto err_exit;
	}
	

	memcpy(&newReq.nwSigevent, notification, sizeof(struct sigevent));
	newReq.nwFlags = flags;
	if (write(_evl_nfy_sd, &newReq, EVL_NFY_MSGMAXSIZE) <= 0) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg header to notify daemon.\n");
		ret = EAGAIN;
		goto err_exit;
	}
	/* Then send the query string. */
	/* fprintf(stderr, "qlen=%i\n", newReq.nwQueryLength); */
	if(write(_evl_nfy_sd, (query)? query->qu_expr : "<null>", newReq.nwQueryLength) <= 0) {
		fprintf(stderr, "Failed to write the query str to notify daemon!\n");
		ret = EAGAIN;
		goto err_exit;
	}
	/* send out the query flags too */
	if (write(_evl_nfy_sd, &query_flags, sizeof(query_flags)) <= 0) {
		fprintf(stderr, "Failed to write the query flags to notify daemon!\n");
		ret = EAGAIN;
		goto err_exit;
	}
	/* Server replies with the following message: */

	if(read(_evl_nfy_sd, &resp, sizeof(nfyMsgHdr_t)) <= 0) {
		fprintf(stderr, "Failed to read response from notify daemon.\n");	
		ret = EAGAIN;
		goto err_exit;
	}

	if (resp.nhStatus < 0) {
		ret = -(resp.nhStatus);
		goto err_exit;
	}

	/* Success */
	if (nfyhandle) {
		*nfyhandle = resp.nhReqHandle;
	}

err_exit:
	if (_evl_nfy_sd > 0) { 
		close(_evl_nfy_sd);
	}
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	unlockNfyApi();
	return ret;	
}
  
/*
 *
 *
 */
int 
posix_log_sigval_getrecid(const union sigval sval,
        posix_log_recid_t *recid)
{
	if (recid) {
 		*recid = sval.sival_int;
 		return 0;
 	}
 	return EINVAL;
}

int posix_log_siginfo_getrecid(const siginfo_t *info,
        void *context, posix_log_recid_t *recid)
{	
	if(recid && info) {
		*recid = info->si_errno;
		return 0;
	}
	return EINVAL;
}

int 
posix_log_notify_get(posix_log_notify_t nfyhandle,
        struct sigevent *notification, int *flags,
        char *qsbuf, size_t qslen, size_t *reqlen)

{
	int ret = 0;
	nfyNewRqMsg_t newReq;
	nfyNewRqMsg_t returnReq;
	nfyMsgHdr_t resp;
	char *qstring;
	sigset_t oldset;
	int sigsBlocked;
	int _evl_nfy_sd = -1;
	struct sockaddr_un _evl_nfy_sock;
	
	if (reqlen) {
		*reqlen = 0;
	}
	if (!notification || !flags) {
		return EINVAL;
	}
	if (!qsbuf) {
		return EMSGSIZE;
	}
	/* Enter critical section */
	lockNfyApi();
	/* Mask all signals so we don't get interrupted */
	sigsBlocked = (_evlBlockSignals(&oldset) == 0);
	
		
	if ((_evl_nfy_sd = _establishNonBlkConnection(EVLNOTIFYD_SERVER_SOCKET, &_evl_nfy_sock, 3 /*timeout*/)) < 0) {	
		ret = EAGAIN;	/* convert error code back to positive number */
		goto err_exit;
	}
	if (isAccess(_evl_nfy_sd) == -1) {
		ret = EPERM;
		goto err_exit;
	}

	/*
	 * Set up the message and send it to the server
	 */
	newReq.nwHeader.nhMsgType = nmtGetRequestStatus;
	newReq.nwHeader.nhReqHandle = nfyhandle;
	
	if (write(_evl_nfy_sd, &newReq, sizeof(nfyNewRqMsg_t)) <= 0) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg header to notify daemon.\n");
		ret = EAGAIN;
		goto err_exit;
	}
	/*
	 * Server replies 
	 */
	if(read(_evl_nfy_sd, &resp, sizeof(nfyMsgHdr_t)) <= 0) {
		fprintf(stderr, "Failed to read response from notify daemon.\n");
		ret = EAGAIN;
		goto err_exit;
	}
	
	if (resp.nhStatus == nrsNoRequest) {
		/* 
		 * Request is not in the system, which should never happen
		 * Perhaps, caller gave the wrong handle
		 */
		ret = EINVAL;
		goto err_exit;
	}  

	/*
	 * Request is still in the system, read more ...
	 */
	if(read(_evl_nfy_sd, &returnReq, sizeof(nfyNewRqMsg_t)) <= 0) {
		fprintf(stderr, "Failed to read response from notify daemon - Reconnect.\n");
		ret = EAGAIN;
		goto err_exit;
	}

	if (reqlen) {
		*reqlen = returnReq.nwQueryLength + 1;
	}
	if (returnReq.nwQueryLength > 0) {
		if ((qstring = malloc(returnReq.nwQueryLength + 1)) == NULL) {
			ret = ENOMEM;
			goto err_exit;
		}
		/*
	 	* Now we know the length of the query - go read the query string
	 	*/
		if(read(_evl_nfy_sd, qstring, returnReq.nwQueryLength) <= 0) {
			fprintf(stderr, "Failed to read response from notify daemon.\n");
			free(qstring);
			ret = EAGAIN;
			goto err_exit;
		}
	
		if (qslen < returnReq.nwQueryLength+1) {
			free(qstring);
			ret = EMSGSIZE;
			goto err_exit;
		}

		qstring[returnReq.nwQueryLength] = '\0';
		(void) strcpy(qsbuf, qstring);
		free(qstring);
	} else {
		qsbuf[0] = '\0';
	}
	
	/* Per the standard, we store these values only on success. */
	memcpy(notification, &returnReq.nwSigevent, sizeof(struct sigevent));
	*flags = returnReq.nwFlags;

err_exit:
	if (_evl_nfy_sd > 0) { 
		close(_evl_nfy_sd);
	}
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	unlockNfyApi();
	return ret;	
}

int
posix_log_notify_remove(posix_log_notify_t nfyhandle)
{
	
	int ret = 0;
	nfyNewRqMsg_t newReq;
	nfyMsgHdr_t resp;
	sigset_t oldset;
	int sigsBlocked;
	int _evl_nfy_sd = -1;
	struct sockaddr_un _evl_nfy_sock;
	
	if (nfyhandle == 0) {
		return EINVAL;
	}
	lockNfyApi();
	/* Mask all signals so we don't get interrpted */
	sigsBlocked = (_evlBlockSignals(&oldset) == 0);
	
	
	if ((_evl_nfy_sd = _establishNonBlkConnection(EVLNOTIFYD_SERVER_SOCKET, &_evl_nfy_sock, 3 /* timeout */)) < 0) {
		ret = EAGAIN;
		goto err_exit;
	}
	if (isAccess(_evl_nfy_sd) == -1 ) {
		ret = EINVAL;
		goto err_exit;
	}

	/*
	 * Setting up the message and send it to the server
	 */
	newReq.nwHeader.nhMsgType = nmtRmRequest;
	newReq.nwHeader.nhReqHandle = nfyhandle;
	
	if (write(_evl_nfy_sd, &newReq, sizeof(nfyNewRqMsg_t)) <= 0) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg header to notify daemon.\n");
		ret = EAGAIN;
		goto err_exit;
	}
	/* Server replies */
	if (read(_evl_nfy_sd, &resp, sizeof(nfyMsgHdr_t)) <= 0) {
		fprintf(stderr, "Failed to read response from notify daemon.\n");
		ret = EAGAIN;
		goto err_exit;
	}
	
	if (resp.nhStatus == nrsNoRequest) {
		/* fprintf(stderr, "No such request with that handle in the system.\n"); */
		ret = EINVAL;
		goto err_exit;
	}

err_exit:
	if (_evl_nfy_sd > 0) { 
		close(_evl_nfy_sd);
	}
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	unlockNfyApi();
	return ret;	
}

