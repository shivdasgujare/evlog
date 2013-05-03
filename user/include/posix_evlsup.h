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
#include <sys/un.h>
#ifndef _POSIX_EVLSUP_H_
#define _POSIX_EVLSUP_H_ 1

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

typedef size_t HANDLE;

/* Name-value pair */
struct _evlNvPair {
	int	nv_value;
	char	*nv_name;
};

/* Values for faAccessFlags */
#define EVL_FACC_PRIVATE 0x1	/* facility's event recs are read-protected */
#define EVL_FACC_KERN 	 0x2

/* An entry from the facility registry */
typedef struct __evlFacility {
	posix_log_facility_t	faCode;
	char					*faName;
	int						faAccessFlags;
	char 					*faFilter;
	posix_log_query_t		*faQuery;
} _evlFacility;

/* Internal view of the info in /etc/evlog.d/facility_registry */
typedef struct __evlFacilityRegistry {
	int			frNFacilities;
	_evlFacility		*frHash;	/* hash table */
	int			frHashSize;	/* # slots in hash table */
} _evlFacilityRegistry;

extern void _evlGetNameByValue(const struct _evlNvPair table[], int value,
	char *name, size_t size, const char *dflt);
extern int _evlGetValueByName(const struct _evlNvPair table[], const char *name,
	int dflt);
extern int _evlGetValueByCIName(const struct _evlNvPair table[],
	const char *name, int dflt);
extern int _evlCIStrcmp(const char *s1, const char *s2);

extern struct _evlNvPair _evlLinuxFacilities[];
extern struct _evlNvPair _evlPosixFacilities[];
extern struct _evlNvPair _evlSeverities[];
extern struct _evlNvPair _evlAttributes[];
extern struct _evlNvPair _evlFormats[];
extern struct _evlNvPair _evlMgmtEventTypes[];

#define LOG_EVLOG_DIR EVLOG_STATE_DIR
#define LOG_EVLOG_CONF_DIR "/etc/evlog.d"
#define LOG_CURLOG_PATH LOG_EVLOG_DIR "/eventlog"
#define LOG_PRIVATE_PATH LOG_EVLOG_DIR "/privatelog"

#define EVL_INVALID_FACILITY ((posix_log_facility_t)-1)
#define EVL_INVALID_EVENT_TYPE (-1)

/*
 * The "age" pseudo-attribute -- must be distinct from POSIX_LOG_ENTRY_* in
 * posix_evlog.h.
 */
#define EVL_ENTRY_AGE 20
#define EVL_ENTRY_HOST 21

/* from posix2.c */
extern int _evlWrite(struct posix_log_entry *entry, const void *buf);
extern int _evlOpen(posix_logd_t *logdes, const char *path);
extern int _evlValidateLogdes(posix_logd_t logdes, int checkGeneration);
extern int _evlRead(posix_logd_t logdes, struct posix_log_entry *entry,
	void *buf, size_t bufLen);
extern int _evlClose(posix_logd_t fd);
extern int _evlRewind(posix_logd_t logdes);
extern int _evlSeekEnd(posix_logd_t logdes);
extern int _evlSeekFwd(posix_logd_t logdes, const posix_log_query_t *query);
extern int _evlSeekBkwd(posix_logd_t logdes, const posix_log_query_t *query);
extern int _evlFdWrite(int fd, const struct posix_log_entry *entry,
	const void *buf);
extern int _evlWriteLogHeader(int fd);
extern int _evlValidateQuery(posix_log_query_t *query, int nullOk);

/* facility registry functions */
extern void _evlLockFacReg(void);
extern void _evlUnlockFacReg(void);
extern int _evlBlockSignals(sigset_t *oldset);
extern void _evlRestoreSignals(sigset_t *oldset);
extern _evlFacilityRegistry *_evlReadFacilities(const char *facRegPath);
extern posix_log_facility_t _evlGetFacilityCodeByCIName(const char *name);
extern char *_evlGetFacilityName(posix_log_facility_t facNum, char *buf);
extern int _evlGetFacilityAccess(posix_log_facility_t facNum);
extern posix_log_query_t * _evlGetFacilityAccessQuery(posix_log_facility_t facNum, int * acc);
extern struct _evlNvPair *_evlSnapshotFacilities();
extern int _evlAddFacilityToRegistry(const char *fname, unsigned int flags,
	const char *filter);
extern unsigned int _evlCrc32(const unsigned char *data, int len);

/* posix1.c */
extern int _evlGetProcId();
extern int isAccess(int sd);
extern int _establishNonBlkConnection(const char * socketpath, struct sockaddr_un *sa, int nsec);
extern int _evlWriteEx(struct posix_log_entry *hdr, const char *buf);
extern int _evlEvaluateQuery(const posix_log_query_t *query,
	const struct posix_log_entry *entry, const void *buf);

#ifdef __cplusplus
}
#endif	/* __cplusplus */
#endif 	/* _POSIX_EVLSUP_H_ */
