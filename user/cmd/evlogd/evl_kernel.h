/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2003
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

#include <asm/types.h>

/*
 * Declarations in this header need to stay in sync with the corresponding
 * declarations in the kernel's evlog.h (or evl_log.h).
 */

/*
 * struct kern_log_entry - kernel record header
 * struct kern_log_entry holds the same info as struct posix_log_entry
 * in user space, with the following exceptions:
 * - Per Linux policy, all types have well-defined sizes.
 * - Unused fields are omitted (log_magic, log_recid, log_thread).
 * - Unnecessarily large fields are reduced in size (log_size, log_format,
 *	log_severity, log_time.tv_nsec).
 * - log_kmagic and log_kversion are added so that the logging daemon
 *	(evlogd) can determine what version of this struct it's dealing with.
 */
struct kern_log_entry_v1 {
	__u16	log_kmagic;
	__u16	log_kversion;
	__u16	log_size;
	__s8	log_format;
	__s8	log_severity;
	__s32	log_event_type;
	__u32	log_facility;
	uid_t	log_uid;
	gid_t	log_gid;
	pid_t	log_pid;
	pid_t	log_pgrp;
	time_t	log_time_sec;
	__s32	log_time_nsec;
	__u32	log_flags;
	__s32	log_processor;
};

struct kern_log_entry_v2 {
	__u16	log_kmagic;
	__u16	log_kversion;
	__u16	log_size;
	__s8	log_format;
	__s8	log_severity;
	__s32	log_event_type;
	__u32	log_facility;
	uid_t	log_uid;
	gid_t	log_gid;
	pid_t	log_pid;
	pid_t	log_pgrp;
	__u32	log_flags;
	__s32	log_processor;
	time_t	log_time_sec;
	__s32	log_time_nsec;
}; 

struct kern_log_entry_v3 {
	__u16	log_kmagic;
	__u16	log_kversion;
	__u16	log_size;
	__s8	log_format;
	__s8	log_severity;
	__s32	log_event_type;
	__u32	log_flags;
	__s32	log_processor;
#ifdef _PPC_64KERN_32USER_
	__s64	log_time_sec;
#else
	time_t	log_time_sec;
#endif
	__s32	log_time_nsec;
	uid_t	log_uid;
	gid_t	log_gid;
	pid_t	log_pid;
	pid_t	log_pgrp;
	char	log_facility[16];
}; 

#define LOGREC_KMAGIC	0x7af8
#define LOGREC_KVERSION	3	/* This implementation supports v1-3. */

/* Values for log_flags member */
#define EVL_EVTYCRC	0x40	/* Daemon will set event type = CRC of */
				/* format string. */
