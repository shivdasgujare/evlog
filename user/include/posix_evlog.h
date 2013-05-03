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

#define LOG_LOGMGMT (12<<3) /* this facility defined by POSIX, but not syslog */
#ifndef _POSIX_EVLOG_H_
#define _POSIX_EVLOG_H_

#include <sys/types.h>
/* For sigevent */
#include <signal.h>
/* For facility (e.g., LOG_KERN) and severity (e.g., LOG_EMERG) codes */
#include <syslog.h>
/* For va_list */
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifdef _POSIX_THREADS
#include <pthread.h>
#endif
#include <linux/evl_log.h>

/* Treat a log descriptor like a file descriptor for now. */
typedef struct _log_description *posix_logd_t;

/* Member selectors for posix_log_entry, for use by posix_log_memtostr  */
#define POSIX_LOG_ENTRY_DATA	0	/* variable-length data */
#define POSIX_LOG_ENTRY_RECID	1
#define POSIX_LOG_ENTRY_SIZE	2
#define POSIX_LOG_ENTRY_FORMAT	3
#define POSIX_LOG_ENTRY_EVENT_TYPE	4
#define POSIX_LOG_ENTRY_FACILITY	5
#define POSIX_LOG_ENTRY_SEVERITY	6
#define POSIX_LOG_ENTRY_UID		7
#define POSIX_LOG_ENTRY_GID		8
#define POSIX_LOG_ENTRY_PID		9
#define POSIX_LOG_ENTRY_PGRP	10
#define POSIX_LOG_ENTRY_TIME	11
#define POSIX_LOG_ENTRY_FLAGS	12
#define POSIX_LOG_ENTRY_THREAD	13
#define POSIX_LOG_ENTRY_PROCESSOR	14

/* Query purpose flags */
#define POSIX_LOG_PRPS_NOTIFY	0x1
#define POSIX_LOG_PRPS_GENERAL	0x2
#define POSIX_LOG_PRPS_SEEK	(POSIX_LOG_PRPS_GENERAL)
#define EVL_PRPS_TEMPLATE	0x4
#define EVL_PRPS_RESTRICTED	0x8
#define POSIX_LOG_PRPS_MASK	0xf

struct node;

typedef struct _posix_log_query {
	int		qu_purpose;	/* NOTIFY, SEEK, etc. */
	char		*qu_expr;	/* Saved copy of the query expr */
	struct node	*qu_tree;	/* Parse tree */
	struct evl_nonStdAtts	*qu_nonStdAtts;	/* Non-standard attributes. */
} posix_log_query_t;


#define POSIX_LOG_ONCE_ONLY		0x01
#define POSIX_LOG_SEND_RECID		0x02
#define POSIX_LOG_SEND_SIGVAL		0x04
#define POSIX_LOG_NFY_MASK		0x07

#define POSIX_LOG_NFY_DISABLED		0x08
#define POSIX_LOG_ACTION_PERSIST	0x16

#define POSIX_LOG_NFY_PENDING_REMOVE	0x512

#define SI_EVLOG	SI_QUEUE
typedef size_t posix_log_notify_t;
/*
 * These are the types of messages that can be sent between the client (a
 * process that has called posix_log_notify_add_ex()) and the server (the
 * notification daemon).
 */
typedef enum _nfyMsgType {
	nmtNewRequest,			/* new notification request */
	nmtGetRequestStatus,	/* get status of notification request */
	nmtRmRequest,			/* remove notification request */
	nmtRequestStatus		/* response to client requests */
} notifyMsgType_t;

/*
 * The status of a notification request.
 */
typedef enum _nfyRqStatus {
	nrsEnabled,		/* Request has been accepted by server. */
	nrsDisabled,	/* Once-only request has been satisfied */
					/* by the server, but not removed by the */
					/* client. */
	nrsRemoved,		/* Request has been removed at client's request. */
	nrsNoRequest	/* Request not found by server */
} nfyRqStatus_t;

/*
 * The header of a message between client and server.
 */
typedef struct _nfyMsgHdr {
	int	nhMsgType;
	posix_log_notify_t	nhReqHandle;
	int	nhStatus;
} nfyMsgHdr_t;

typedef struct _nfyNewRqMsg {
	nfyMsgHdr_t	nwHeader;
	size_t		nwQueryLength;
	struct sigevent	nwSigevent;
	int		nwFlags;
} nfyNewRqMsg_t;

#define EVL_NFY_MSGMAXSIZE (sizeof(nfyNewRqMsg_t))

/* Log-management event-type codes */
#define POSIX_LOG_MGMT_TIMEMARK	1
#define POSIX_LOG_MGMT_STARTMAINT	2
#define POSIX_LOG_MGMT_ENDMAINT	3
#define POSIX_LOG_MGMT_STOPLOGGING	4
#define POSIX_LOG_MGMT_STARTLOGGING	5

/* Per-process maximum number of concurrent open log descriptors */
#define POSIX_LOG_OPEN_MAX	100

/* Per-process max number of concurrently registered notification requests */
#define POSIX_LOG_NOTIFY_MAX	100

/* Seek directions */
#define POSIX_LOG_SEEK_START	1
#define POSIX_LOG_SEEK_END		2
#define POSIX_LOG_SEEK_FORWARD	3
#define POSIX_LOG_SEEK_BACKWARD	4
#define POSIX_LOG_SEEK_FIRST	5
#define POSIX_LOG_SEEK_LAST		6

/* Maximum length for a string returned by posix_log_memtostr */
#define POSIX_LOG_MEMSTR_MAXLEN	128

/* Log an event */
extern int posix_log_write(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity,
	const void *buf, size_t len, int format, unsigned int flags);

extern int posix_log_vprintf(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity, unsigned int flags,
	const char *format, va_list args);

extern int posix_log_printf(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity, unsigned int flags,
	const char *format, ...);

extern int posix_log_vprintb(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity, unsigned int flags,
	const char *format, va_list args);

extern int posix_log_printb(posix_log_facility_t facility,
	int event_type, posix_log_severity_t severity, unsigned int flags,
	const char *format, ...);

/* Open an event log for read access */
extern int posix_log_open(posix_logd_t *logdes, const char *path);

/* Read from event log */
extern int posix_log_read(posix_logd_t logdes, struct posix_log_entry *entry,
	void *log_buf, size_t log_len); 

/* Register for notification about new events */
extern int posix_log_notify_add(const posix_log_query_t *query,
	const struct sigevent *notification, int flags,
	posix_log_notify_t *nfyhandle);

extern int posix_log_sigval_getrecid(const union sigval sval,
	posix_log_recid_t *recid);

extern int posix_log_siginfo_getrecid(const siginfo_t *info,
        void *context, posix_log_recid_t *recid);
        
extern int posix_log_notify_get(posix_log_notify_t nfyhandle,
	struct sigevent *notification, int *flags,
	char *qsbuf, size_t qslen, size_t *reqlen);

/* Remove notification request */
extern int posix_log_notify_remove(posix_log_notify_t nfyhandle);

/* Close event log */
extern int posix_log_close(posix_logd_t logdes);

/* Reposition the read pointer */
extern int posix_log_seek(posix_logd_t logdes, const posix_log_query_t *query,
	int direction); 

/* Compare severities */
extern int posix_log_severity_compare(int *order, posix_log_severity_t s1,
	posix_log_severity_t s2); 

/* Create log query */
extern int posix_log_query_create(const char *query_string, int purpose,
	posix_log_query_t *query, char *errbuf, size_t errlen);

extern int posix_log_query_get(const posix_log_query_t *query, int *purpose,
	char *qsbuf, size_t qslen, size_t *reqlen);

/* Destroy log query */
extern int posix_log_query_destroy(posix_log_query_t *query);

/* Obtain the string equivalent of an event-record member */
extern int posix_log_memtostr(int member_selector,
	const struct posix_log_entry *entry,
	char *buf, size_t buflen);

/* Convert string to facility or facility to string */
extern int posix_log_factostr(posix_log_facility_t fac, char *buf,
	size_t buflen);

extern int posix_log_strtofac(const char *str, posix_log_facility_t *fac);

#ifdef __cplusplus
}
#endif 	/* __cplusplus */
#endif	/* _POSIX_EVLOG_H_ */
