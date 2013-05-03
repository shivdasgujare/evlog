/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2001
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

#ifndef _LINUX_EVL_LOG_H
#define _LINUX_EVL_LOG_H

#ifndef __KERNEL__
#ifdef _POSIX_THREADS
#include <pthread.h>
#endif
#endif

/* Values for log_flags member */
#define POSIX_LOG_TRUNCATE  0x1
#define EVL_KERNEL_EVENT	0x2
#define EVL_INITIAL_BOOT_EVENT 	0x4
#define EVL_KERNTIME_LOCAL	0x8
#define EVL_INTERRUPT		0x10	/* Logged from interrupt context */
#define EVL_PRINTK		0x20	/* Logged by printk() */

/* Formats for optional portion of record. */
#define POSIX_LOG_NODATA    0
#define POSIX_LOG_BINARY    1
#define POSIX_LOG_STRING    2
#define POSIX_LOG_PRINTF    3

/* Maximum length of variable portion of record */
#define POSIX_LOG_ENTRY_MAXLEN      (8 * 1024)

/* Maximum length for a string returned by posix_log_memtostr */
/* Thus also the max length of a facility name */
#define POSIX_LOG_MEMSTR_MAXLEN	128

typedef unsigned int posix_log_facility_t;
typedef int posix_log_severity_t;
typedef int posix_log_recid_t;
typedef int posix_log_procid_t;

#define EVL_INVALID_FACILITY ((posix_log_facility_t)-1)

/*
 * This is the user-space record header.  Kernel event records may also
 * use this format, depending on which kernel implementation you're using.
 * Some implementations define the kernel-record header differently --
 * see cmd/evlogd/evl_kernel.h.
 */
struct posix_log_entry {
	unsigned int            log_magic;
        posix_log_recid_t   	log_recid;
        size_t          	log_size;
        int             	log_format;
        int             	log_event_type;
        posix_log_facility_t 	log_facility;
        posix_log_severity_t    log_severity;
        uid_t           	log_uid;
        gid_t           	log_gid;
        pid_t           	log_pid;
        pid_t           	log_pgrp;
        struct timespec 	log_time;
        unsigned int    	log_flags;
#ifdef __KERNEL__
        unsigned long int	log_thread;
#else
#ifdef _POSIX_THREADS
	pthread_t		log_thread;
#else
	unsigned long int	log_thread;
#endif
#endif
        posix_log_procid_t	log_processor;
}; 

#define LOGFILE_MAGIC   0xbeefface
#define LOGREC_MAGIC    0xfeefface
#define REC_HDR_SIZE    sizeof(struct posix_log_entry)

/*
 * Reserved Event Types
 */
#define EVL_SYSLOG_MESSAGE      0x1
#define EVL_PRINTK_MESSAGE      0x2
#define EVL_BUFFER_OVERRUN      0x6
#define EVL_DUPS_DISCARDED      0x7

/*
 * A record of type (LOG_LOGMGMT, EVLOG_REGISTER_FAC) is generated by
 * evl_register_facility().
 */
#define EVLOG_REGISTER_FAC 40

#define LOG_LOGMGMT              (12<<3)	/* EVL Facility */

/*
 * The optional portion of a record of type (LOG_LOGMGMT, EVLOG_REGISTER_FAC)
 * is an evl_facreg_rq object followed by a string (the facility name).
 * evlogd intercepts this record and does the requested registration, if needed.
 *
 * fr_kernel_fac_code is the facility code generated by the kernel's call
 * to evl_register_facility().  This field is filled in by the kernel.
 *
 * fr_registry_fac_code is the facility code that appears in the registry.
 * (This should match fr_kernel_fac_code.)  This field is filled in by
 * evlogd after it intercepts and executes this request.
 *
 * fr_rq_status is the request's status:
 *	frst_kernel_failed: Set by the kernel to indicate that it could not
 *		generate a valid facility code for the given name.
 *	frst_kernel_ok: Set by the kernel to indicate success so far.
 * evlogd replaces the frst_kernel_ok value with one of the following:
 *	frst_already_registered: No need to register the facility.  It's
 *		already registered, and the code matches.
 *	frst_registered_ok: We registered it, with the expected results.
 *	frst_registration_failed: We tried to register it, but couldn't.
 *	frst_faccode_mismatch: A facility by that name is in the registry,
 *		but its code doesn't match fr_kernel_fac_code.
 */
typedef enum {
	frst_kernel_failed = -1,
	frst_kernel_ok = 0,
	frst_already_registered,
	frst_registered_ok,
	frst_registration_failed,
	frst_faccode_mismatch
} evl_facreg_rq_status_t;

struct evl_facreg_rq {
	posix_log_facility_t	fr_kernel_fac_code;
	posix_log_facility_t	fr_registry_fac_code;
	evl_facreg_rq_status_t	fr_rq_status;
};

#ifdef __KERNEL__
/*
 * Reserved Facilities
 */
#define LOG_KERN        (0<<3)  /* Kernel Facility */
#define LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
/*
 * priorities (these are ordered)
 */
#define LOG_EMERG   0   /* system is unusable */
#define LOG_ALERT   1   /* action must be taken immediately */
#define LOG_CRIT    2   /* critical conditions */
#define LOG_ERR     3   /* error conditions */
#define LOG_WARNING 4   /* warning conditions */
#define LOG_NOTICE  5   /* normal but significant condition */
#define LOG_INFO    6   /* informational */
#define LOG_DEBUG   7   /* debug-level messages */

#ifdef CONFIG_EVLOG
/* Various kernel implementations provide some or all of these functions. */
extern int evl_writek(posix_log_facility_t facility, int event_type, 
		     posix_log_severity_t severity, unsigned int flags, ...);
		
extern int evl_vwritek(posix_log_facility_t facility, int event_type,
		     posix_log_severity_t severity, unsigned int flags,va_list args);
		
extern int posix_log_printf(posix_log_facility_t facility, int event_type, 
		posix_log_severity_t severity, unsigned int flags, 
		const char *fmt, ...);

extern int posix_log_vprintf(posix_log_facility_t facility, int event_type,
		posix_log_severity_t severity, unsigned int flags,
		const char *fmt, va_list args);
		
extern int posix_log_write(posix_log_facility_t facility, int event_type,
         	posix_log_severity_t severity, const void *buf,
        	size_t len, int format, unsigned int flags);

extern int evl_gen_facility_code(const char *fname,
		posix_log_facility_t *fcode);

extern int evl_register_facility(const char *fname,
		posix_log_facility_t *fcode);
#else	/* ! CONFIG_EVLOG */
inline int evl_writek(posix_log_facility_t facility, int event_type, 
		posix_log_severity_t severity, unsigned int flags, ...)
		{ return -ENOSYS; }

inline int evl_vwritek(posix_log_facility_t facility, int event_type,
		posix_log_severity_t severity, unsigned int flags,va_list args)
		{ return -ENOSYS; }
		
inline int posix_log_printf(posix_log_facility_t facility, int event_type, 
		posix_log_severity_t severity, unsigned int flags, 
		const char *fmt, ...)
		{ return -ENOSYS; }

inline int posix_log_vprintf(posix_log_facility_t facility, int event_type,
		posix_log_severity_t severity, unsigned int flags,
		const char *fmt, va_list args)
		{ return -ENOSYS; }

inline int posix_log_write(posix_log_facility_t facility, int event_type,
		posix_log_severity_t severity, const void *buf,
		size_t len, int format, unsigned int flags)
		{ return -ENOSYS; }

inline int evl_gen_facility_code(const char *fname,
		posix_log_facility_t *fcode)
		{ return -ENOSYS; }

inline int evl_register_facility(const char *fname,
		posix_log_facility_t *fcode)
		{ return -ENOSYS; }
#endif	/* CONFIG_EVLOG */
#endif	/* __KERNEL__ */

#endif