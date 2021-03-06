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

#ifndef _EVLOG_H_
#define _EVLOG_H_

#include <sys/types.h>
#include <stdarg.h>
#include <string.h>

#include <linux/evl_log.h>

#ifdef __cplusplus
extern "C" {
#endif
#ifndef EVLOG_STATE_DIR
#define EVLOG_STATE_DIR			"/var/evlog"
#endif
#define EVLOGD_SERVER_SOCKET		EVLOG_STATE_DIR "/evlogdsocket"
#define EVLOGD_EVTSERVER_SOCKET		EVLOG_STATE_DIR "/evlogdevtsocket"
#define EVLNOTIFYD_SERVER_SOCKET	EVLOG_STATE_DIR "/evlnotifydsocket"
#define EVLOG_CONF_SOCKET		EVLOG_STATE_DIR "/evlconfsoc"
#define EVLACTIOND_SERVER_SOCKET	EVLOG_STATE_DIR "/evlactiondsocket"

/* Record formats.  See also <linux/evl_log.h> */
#ifndef POSIX_LOG_PRINTF
#define POSIX_LOG_PRINTF	3
#endif

/* Output formats */
#define EVL_COMPACT 0x1

typedef int evlrecsize_t;
#define SIZESIZE sizeof(evlrecsize_t)

/*
 * Log Management Event Types
 * These are event types that may be logged for facility LOG_LOGMGMT.
 *
 * The following event types are reserved, and are defined in other headers:
 * 1-5: These are the standard LOG_LOGMGMT event types defined in POSIX 1003.25
 * -- e.g., POSIX_LOG_MGMT_STARTMAINT.
 * 40: EVLOG_REGISTER_FAC -- A request from the kernel to register a facility.
 */

/* Event types 1-5 are reserved. */

#define EVLOG_FORK_FAILED       8
#define EVLOG_WRITE_PID         9
#define EVLOG_PID_EXISTS        10
#define EVLOG_SIG_ACT           11
#define EVLOG_OPEN_LOG_FAILED   12
#define EVLOG_WRITE_LOG_FAILED  13
#define EVLOG_READ_LOG_FAILED   14
#define EVLOG_SEEK_LOG_FAILED   15
#define EVLOG_MALLOC_FAILED     16
#define EVLOG_READBUF_FAILED    17

#define EVLOG_WRITE_REQF_FAILED		20
#define EVLOG_READ_REQF_FAILED		21
#define EVLOG_OPEN_REQF_FAILED		22
#define EVLOG_OPEN_SOCKET_FAILED	23
#define EVLOG_BROKEN_PIPE			24
#define EVLOG_UID_OP_FAILED			25
#define EVLOG_GID_OP_FAILED			26
#define EVLOG_FAILED_APPENDTOLIST	27

#define EVLOG_PWFILE_LOOKUP_FAILED 	28
#define EVLOG_GRFILE_LOOKUP_FAILED 	29

#define EVLOG_LOADREG_FAILED		30
#define EVLOG_ACCESS_DENIED			31

/* Event type 40 is reserved. */

#define NFY_ACCESS_DENIED		0xfa
#define NFY_ACCESS_GRANTED		0xac
#define NFY_MAX_CLIENTS			0xca

#define EVLOGD_ACCESS_DENIED		NFY_ACCESS_DENIED
#define EVLOGD_ACCESS_GRANTED		NFY_ACCESS_GRANTED
#define EVLOGD_MAX_CLIENTS			NFY_MAX_CLIENTS

/* 
 * Architecture signature for each log record.
 * This will be the lower word of the of the rec->log_magic field.
 */
#define LOGREC_NO_ARCH				0x0000
#define LOGREC_ARCH_I386			0x0001
#define	LOGREC_ARCH_IA64			0x0002
#define LOGREC_ARCH_S390			0x0003
#define LOGREC_ARCH_S390X			0x0004
#define LOGREC_ARCH_PPC				0x0005
#define LOGREC_ARCH_PPC64			0x0006
#define LOGREC_ARCH_X86_64			0x0007
#define LOGREC_ARCH_ARM_BE			0x0008
#define LOGREC_ARCH_ARM_LE			0x0009

/*
 * Logfile Header structure.
 */
typedef struct log_header {
	uint		log_magic;	/* Magic number indicating log file */
	long		log_version;	/* Event logging version */
	posix_log_recid_t last_recId;	/* Last recid in log: used by evlogd */
	off_t		reserved1;
	uint		log_generation;	/* Changes during log maintenance */
} log_header_t;

/* log an event */
extern int evl_log_write(posix_log_facility_t facility, int event_type,
	posix_log_severity_t severity, unsigned int flags, ...);

extern int evl_log_vwrite(posix_log_facility_t facility, int event_type,
	posix_log_severity_t severity, unsigned int flags, va_list args);

extern int evl_pack_format_and_args(const char *format, va_list args,
	char *databuf, size_t *datasz);

/* format fixed portion of event record */
extern int evl_format_evrec_fixed(const struct posix_log_entry *entry,
	char *buf, size_t buflen, size_t *reqlen, const char *separator,
	size_t linelen, int fmt_flags);

/* format variable portion of event record */
extern int evl_format_evrec_variable(const struct posix_log_entry *entry,
	const void *var_buf, char *buf, size_t buflen, size_t *reqlen);

/* format event record according to user-supplied format */
extern int evl_format_evrec_sprintf(const struct posix_log_entry *entry,
	const void *var_buf, const char *format, char *buf,
	size_t buflen, size_t *reqlen);

/* compute facility code from name */
extern int evl_gen_facility_code(const char *fname,
	posix_log_facility_t *fcode);

/* add facility to registry, if needed, and get code */
extern int evl_register_facility(const char *fname,
	posix_log_facility_t *fcode);

/* For internal use... */
extern int _evlDumpBytes(const void *data, size_t nBytes, char *buf,
	size_t buflen, size_t *reqlen);
extern int _evlDumpBytesForce(const void *data, size_t nBytes, char *buf,
	size_t buflen, size_t *reqlen);
extern size_t _evlGetMaxDumpLen();
extern int _evlFormatPrintfRec(const char *data, size_t datasz, char *buf,
	size_t buflen, size_t *reqlen, int printk);

/* The syslog equivalent of the kernel's printkat() */
extern int _evl_syslogat(int priority, const char *facname, int event_type,
	const char *fmt, const char *unbraced_fmt, ...);
extern char * _evl_unbrace(const char *fmt);
extern int evl_gen_event_type(const char *s1, const char *s2, const char *s3);
#ifndef __Lynx__
extern void syslog(int priority, const char *fmt, ...);
#else
extern void syslog(int priority, char *fmt, ...);
#endif

#define __stringify2(s) #s
#define __stringify(s) __stringify2(s)

struct evlog_position {
   int line;
   char function[64 - sizeof(int)];
   char file[128];
};

#define _EVLOG_POS { __LINE__, __FILE__ }

struct evlog_info {
   char format[128+64];
   char facility[64];
   struct evlog_position pos;
};

#define syslogat(priority, fmt, ...) \
    do { \
	static struct evlog_info __attribute__((section(".log"),unused)) ___ \
		= { fmt, __stringify(EVL_FACILITY_NAME), _EVLOG_POS }; \
	char *unbraced_fmt = _evl_unbrace(fmt); \
	if (_evl_syslogat(priority, __stringify(EVL_FACILITY_NAME), \
	    evl_gen_event_type(__FILE__, __FUNCTION__, fmt), \
	    fmt, unbraced_fmt, ##__VA_ARGS__) != 1) \
		syslog(priority, unbraced_fmt, ##__VA_ARGS__); \
    	free(unbraced_fmt); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif  	/* _EVLOG_H_ */
