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
#include <assert.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/klog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_list.h"

/*
 * posix1.c implements the functions in the POSIX 1003.25 Event Logging API.
 * posix2.c provides support for these functions.  In particular, the functions
 * that know about the internal structure of the event log are found in
 * posix2.c.
 */

/*
The event log is an ordinary binary file.  Each record consists of:
1. the posix_log_entry object
2. the optional (variable) data
3. the length of the record (items 1 and 2).  (This allows seeking backward.)
The event log file is written only by the log daemon.

All functions are intended to be thread-safe and signal-safe.

The log file has a magic number up front for validation.
 */

/* Keep multiple threads from trying to talk to the log daemon concurrently. */
static pthread_mutex_t logdSocketMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * In some implementations of POSIX threads (notably Linux), different threads
 * of (supposedly) the same process can have different process IDs.  Since
 * the logging daemon expects all requests on the same socket to come from
 * the same process, we have to open a different socket for each process ID.
 */
struct sdMapping {
	pid_t	pid;
	int	sd;
};
static evl_list_t *sdMap = NULL;	/* list of sdMappings */
static int getSdByPid(pid_t pid);
static void zapSd(int sd);

/* This magic number should appear at the beginning of every log file. */
static int logMagic = LOGFILE_MAGIC;

/* This magic number should appear at the beginning of every log description. */
#define LOGD_MAGIC 0xf00dface

/*
 * Description of a log file that has been opened for reading.
 * Note that a posix_logd_t is a POINTER to one of these.
 */
typedef struct _log_description {
	unsigned int	ld_magic;	/* always LOGD_MAGIC */
	int		ld_fd;		/* file descriptor */
	log_header_t	*ld_header;	/* mmap-ed log header (read-only) */
	unsigned int	ld_generation;	/* generation # at time of open */
} log_desc_t;

#define LOGD_MMAP_SZ (getpagesize())

/*
 * Is this log file undergoing maintenance?
 * For now, we assume that an odd generation number indicates that the log
 * is undergoing maintenance.
 */
#define isBusyLog(hdr) (((hdr)->log_generation & 0x1) != 0)

/*
 * Allocate a log description and return a pointer to it.
 */
static log_desc_t *
makeLogDescription()
{
	log_desc_t *logd = (log_desc_t*) malloc(sizeof(log_desc_t));
	assert(logd != NULL);
	memset(logd, 0, sizeof(log_desc_t));
	logd->ld_magic = LOGD_MAGIC;
	logd->ld_fd = -1;
	return logd;
}

/*
 * Release the resources associated with log description logd.
 */
static int
releaseLogDescription(log_desc_t *logd)
{
	if (logd == NULL || logd->ld_magic != LOGD_MAGIC) {
		return EBADF;
	}
	if (logd->ld_header != NULL) {
		if (munmap(logd->ld_header, LOGD_MMAP_SZ) != 0) {
			perror("munmap of log header");
		}
	}
	if (logd->ld_fd >= 0) {
		(void) close(logd->ld_fd);
	}
	free(logd);
	return 0;
}

/* Get the file descriptor from a log descriptor.  */
#define logdToFd(logdes) (logdes)->ld_fd

/* Signal-safe read. */
static int
safeRead(int fd, void *buf, size_t len)
{
	int nBytes;
	do {
		nBytes = read(fd, buf, len);
	} while (nBytes < 0 && errno == EINTR);
	return nBytes;
}

/* Signal-safe write. */
static int
safeWrite(int fd, const void *buf, size_t len)
{
	int nBytes;
	do {
		nBytes = write(fd, buf, len);
	} while (nBytes < 0 && errno == EINTR);
	return nBytes;
}

/*
 * Write the indicated record to the file whose descriptor is fd.
 * The record size is also written.  This is used by evlview -out.
 * Returns 0 on success.  On failure, returns -1 and errno is the reason.
 */
int
_evlFdWrite(int fd, const struct posix_log_entry *entry, const void *buf)
{
	int nBytes;
	evlrecsize_t wholeRecSize = REC_HDR_SIZE + entry->log_size;
	char writebuf[POSIX_LOG_ENTRY_MAXLEN + REC_HDR_SIZE + SIZESIZE];

	bcopy(entry, writebuf, REC_HDR_SIZE);
	bcopy(buf, writebuf + REC_HDR_SIZE, entry->log_size);
	bcopy(&wholeRecSize, writebuf + wholeRecSize, SIZESIZE);

	nBytes = safeWrite(fd, writebuf, wholeRecSize + SIZESIZE);

	return (nBytes < 0 ? -1 : 0);
}

/*
 * Write a log header to the file whose descriptor is fd.  This is used
 * by evlview -out.  Returns 0 on success.  On failure, returns -1 and
 * errno is the reason.
 */
int
_evlWriteLogHeader(int fd)
{
	log_header_t log_hdr;
	int nBytes;

	(void) memset(&log_hdr, 0, sizeof(log_hdr));
	log_hdr.log_magic = logMagic;
	log_hdr.log_version = 0;	/*TODO: Plug in current version. */
	log_hdr.log_generation = 0;
	nBytes = safeWrite(fd, &log_hdr, sizeof(log_hdr));
	return (nBytes < 0 ? -1 : 0);
}

int
_evlOpen(posix_logd_t *logdes, const char *path)
{
	int fd;	
	log_desc_t *logd;
	log_header_t *logHdr;
	void *mappedLog;

	if (path == 0) {
		path = LOG_CURLOG_PATH;
	}

	fd = open(path, O_RDONLY|O_NONBLOCK);
	if (fd < 0) {
		return errno;
	}

	logd = makeLogDescription();
	logd->ld_fd = fd;

	/*
	 * Mmap the beginning of the file so we can have quick access to the
	 * file header.  We need this because we compare the log descriptor's
	 * generation number with the log's generation number on every read
	 * or seek, to ensure prompt detection of ongoing maintenance activities
	 * that might clobber the log as we read it.
	 */
	assert(LOGD_MMAP_SZ >= sizeof(log_header_t));
	mappedLog = mmap(0, LOGD_MMAP_SZ, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mappedLog == MAP_FAILED) {
		perror("mmap of log header");
		(void) releaseLogDescription(logd);
		return EINVAL;
	}
	logHdr = (log_header_t*) mappedLog;
	logd->ld_header = logHdr;

	/* Check the magic number to verify that this is a valid log file. */
	if (logHdr->log_magic != logMagic) {
		(void) releaseLogDescription(logd);
		return EINVAL;
	}
	if (isBusyLog(logHdr)) {
		(void) releaseLogDescription(logd);
		return EBUSY;
	}
	logd->ld_generation = logHdr->log_generation;

	/* Seek past header to first record. */
	if (lseek(fd, sizeof(log_header_t), SEEK_CUR) == (off_t)-1) {
		perror("Seek past log header on open");
		(void) releaseLogDescription(logd);
		return EINVAL;
	}

	*logdes = logd;
	return 0;
}

/*
 * Return 0 if logdes is a valid log descriptor, or -1 otherwise.
 * We don't check the generation number if we're closing the log.
 */
int
_evlValidateLogdes(posix_logd_t logd, int checkGeneration)
{
	if (logd == NULL || logd->ld_magic != LOGD_MAGIC) {
		return -1;
	}
	if (checkGeneration
	    && logd->ld_generation != logd->ld_header->log_generation) {
		return -1;
	}
	return 0;
}

static int
readBuf(int fd, void *buf, size_t len)
{
	int nBytes = safeRead(fd, buf, len);
	if (nBytes < 0) {
		if (errno == EAGAIN) {
			return EAGAIN;
		} else {
			return EIO;
		}
	} else if (nBytes == 0) {
		return EAGAIN;
	} else if (nBytes != len) {
		return EIO;
	}
	return 0;
}

/*
 * logdes and entry have been previously validated.
 */
int
_evlRead(posix_logd_t logdes, struct posix_log_entry *entry,
	void *buf, size_t bufLen)
{
	int fd = logdToFd(logdes);
	int status;
	size_t dataLen;
	char bigBuf[POSIX_LOG_ENTRY_MAXLEN];
	char *ioBuf;
	off_t seekStatus;

	(void) flock(fd, LOCK_SH);
	/* Read the record header. */
	if ((status = readBuf(fd, entry, sizeof(struct posix_log_entry))) != 0) {
		(void) flock(fd, LOCK_UN);
		return status;
	}

	/* Now read the data into the buffer. */
	dataLen = entry->log_size;

	if (dataLen > POSIX_LOG_ENTRY_MAXLEN) {
		fprintf(stderr, "Corrupted record with log_size == %zu\n", dataLen);
		(void) flock(fd, LOCK_UN);
		return EIO;
	}

	if (dataLen > 0) {
		/* Need to read (or at least skip over) the variable data. */
		if (buf != 0 && bufLen > 0) {
			/* Caller wants the data. */
			if (bufLen >= dataLen) {
				/* And his buffer can hold it all. */
				ioBuf = buf;
			} else {
				/* Read all to our buffer, then copy his part. */
				ioBuf = bigBuf;
			}
			if ((status = readBuf(fd, ioBuf, dataLen)) != 0) {
				if (status == EAGAIN) {
					/* Shouldn't happen here. */
					status = EIO;
				}
				(void) flock(fd, LOCK_UN);
				return status;
			}
			if (ioBuf != buf) {
				memcpy(buf, ioBuf, bufLen);
			}
		} else {
			/* Caller doesn't want the data.  Just skip over it. */
			seekStatus = lseek(fd, dataLen, SEEK_CUR);
			if (seekStatus == (off_t)-1) {
				fprintf(stderr, "lseek #1 failed in _evlRead\n");
				(void) flock(fd, LOCK_UN);
				return EIO;
			}
		}
	}

	/* Skip over the log_size value at the end of the record. */
	seekStatus = lseek(fd, SIZESIZE, SEEK_CUR);
	if (seekStatus == (off_t)-1) {
		fprintf(stderr, "lseek #2 failed in _evlRead\n");
		(void) flock(fd, LOCK_UN);
		return EIO;
	}

	(void) flock(fd, LOCK_UN);
	return 0;
}

int
_evlClose(posix_logd_t logdes)
{
	return releaseLogDescription(logdes);
}

/* Set the read pointer to the first record -- just past the file header. */
int
_evlRewind(posix_logd_t logdes)
{
	int fd = logdToFd(logdes);
	if (lseek(fd, sizeof(log_header_t), SEEK_SET) == (off_t)-1) {
		return EIO;
	}
	return 0;
}

int
_evlSeekEnd(posix_logd_t logdes)
{
	int fd = logdToFd(logdes);
	if (lseek(fd, 0, SEEK_END) == (off_t)-1) {
		return EIO;
	}
	return 0;
}

int
_evlSeekFwd(posix_logd_t logdes, const posix_log_query_t *query)
{
	int fd = logdToFd(logdes);
	off_t startPosition, curPosition;
	int status;
	struct posix_log_entry entry;
	char buf[POSIX_LOG_ENTRY_MAXLEN];

	startPosition = lseek(fd, 0, SEEK_CUR);
	if (startPosition == (off_t)-1) {
		return EIO;
	}

	for (;;) {
		curPosition = lseek(fd, 0, SEEK_CUR);
		if (curPosition == (off_t)-1) {
			return EIO;
		}

		status = _evlRead(logdes, &entry, buf, POSIX_LOG_ENTRY_MAXLEN);
		if (status == EAGAIN) {
			/* End of file: not found; reset read pointer */
			if (lseek(fd, startPosition, SEEK_SET) == (off_t)-1) {
				return EIO;
			}
			return ENOENT;
		} else if (status != 0) {
			return EIO;
		}

		if (!query || _evlEvaluateQuery(query, &entry, buf)) {
			/* Found the record we want. Move the read pointer back to it. */
			if (lseek(fd, curPosition, SEEK_SET) == (off_t)-1) {
				return EIO;
			}
			return 0;
		}
	}
}

int
_evlSeekBkwd(posix_logd_t logdes, const posix_log_query_t *query)
{
	int fd = logdToFd(logdes);
	evlrecsize_t wholeRecSize;
	off_t curPosition, startPosition;
	char wholeRec[sizeof(struct posix_log_entry) + POSIX_LOG_ENTRY_MAXLEN];
	char *buf;
	struct posix_log_entry *entry;

	startPosition = lseek(fd, 0, SEEK_CUR);
	if (startPosition == (off_t)-1) {
		return EIO;
	}

	curPosition = startPosition;
	/*
	 * The first record in the log starts at offset sizeof(log_header_t)
	 * and extends for at least sizeof(struct posix_log_entry)+SIZESIZE
	 * bytes.  So if the current offset <= sizeof(struct posix_log_entry),
	 * we must be at the first record.
	 */
	while (curPosition > sizeof(struct posix_log_entry)) {
		/* Back up and read the size of the previous record's var data. */
		if (lseek(fd, -SIZESIZE, SEEK_CUR) == (off_t)-1) {
			return EIO;
		}
		if (readBuf(fd, &wholeRecSize, SIZESIZE) != 0) {
			return EIO;
		}

		/* Validate wholeRecSize */
		if (wholeRecSize > sizeof(wholeRec)) {
			return EINVAL;
		}

		/* Back up to the beginning of the previous record. */
		curPosition = lseek(fd, -(wholeRecSize+SIZESIZE), SEEK_CUR);
		if (curPosition == (off_t)-1) {
			return EIO;
		}

		/* Read the record, then back up to the start of it. */
		if (readBuf(fd, wholeRec, wholeRecSize) != 0) {
			return EIO;
		}
		if (lseek(fd, curPosition, SEEK_SET) == (off_t)-1) {
			return EIO;
		}
		entry = (struct posix_log_entry*) wholeRec;
		buf = wholeRec + sizeof(struct posix_log_entry);

		/* If this record matches the query, we're done. */
		if (!query || _evlEvaluateQuery(query, entry, buf)) {
			return 0;
		}
	}

	/* Reached beginning of file, with no match. */
	if (lseek(fd, startPosition, SEEK_SET) == (off_t)-1) {
		return EIO;
	}
	return ENOENT;
}

/* Return 0 if it's a valid query object, or -1 if it's not. */
int
_evlValidateQuery(posix_log_query_t *query, int nullOk)
{
	if (!query) {
		return (nullOk ? 0 : -1);
	}

	if (query->qu_tree == NULL || query->qu_expr == NULL
	    || query->qu_purpose == 0
	    || (query->qu_purpose & ~POSIX_LOG_PRPS_MASK) != 0) {
		return -1;
	}
	return 0;	
}	


/* 
 * Establish a socket connection - This is a non-blocking connect.
 */
int
_establishNonBlkConnection(const char *socketpath, struct sockaddr_un *sa, int nsec)
{
	int sd;
	socklen_t sock_len, len;	/* Size of generic UD address structure */

	int flags, n, error=0;
	struct timeval tval;
	fd_set rset, wset;
	
	/*
	 * Open the daemon
	 */
	if ((sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0){
		(void)fprintf(stderr, "Cannot create socket.\n");
		return -EAGAIN;
	}

	/* setting non-blocking connect */
	flags = fcntl(sd, F_GETFL);
	fcntl(sd, F_SETFL, flags | O_NONBLOCK);
	
	/*
	 * Request connect to the specified socket address.  In this case,
	 * UNIX Domain format, The socket address is really just a file.
	 */
	memset(sa, 0, sizeof(struct sockaddr_un));
	sa->sun_family = PF_UNIX;
	(void)strcpy(sa->sun_path, socketpath);
	sock_len = sizeof(sa->sun_family) + strlen(sa->sun_path);

	if ((n = connect(sd, (struct sockaddr *)sa, sock_len) < 0)) {
		if (errno != EINPROGRESS) {
//			(void)fprintf(stderr, "Failed to connect to the daemon. errno=%d.\n", errno);
			goto err_connect;
		}	
	}

	if (n == 0)
		goto done; 	/* connection completed immediatly */

	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	wset = rset;

	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ((n = select(sd + 1, &rset, &wset, NULL, nsec? &tval : NULL)) == 0) {
		/* time out */
		goto err_connect;
	}

	if (FD_ISSET(sd, &rset) || FD_ISSET(sd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			/* some error */
			(void)fprintf(stderr, "Error connecting to the daemon.\n");
			goto err_connect;
		}
	} else {
		goto err_connect;
	} 

done:
	/* Restore flags and set the close-on-exec flag on sd. */
	fcntl(sd, F_SETFL, flags);
	flags = fcntl(sd, F_GETFD);
	flags |= FD_CLOEXEC;
	if (fcntl(sd, F_SETFD, flags) == -1) {
		perror("fcntl(F_SETFD)");
		goto err_connect;
	}	
	return sd;

err_connect:
	close(sd);
	return -EAGAIN;
}

int 
isAccess(int sd)
{
	char abyte;
	
	read(sd, &abyte, sizeof(char));
	
	if (abyte == (char) NFY_ACCESS_DENIED) {
		(void) fprintf(stderr, "isAccess test failed.\n");
		return -1;
	}
	else if (abyte == (char) NFY_MAX_CLIENTS) {
		(void) fprintf(stderr, "Max number of clients reached. Access denied.\n");
		return -1;
	}
	return 0;
}	

int 
_evlWriteEx2(int sd, struct posix_log_entry *entry, const char *buf)
{
	char writebuf[POSIX_LOG_ENTRY_MAXLEN];

	/* First write the header */
	if (write(sd, entry, REC_HDR_SIZE) != REC_HDR_SIZE) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg header to evlog daemon.\n");
		return EIO;
	}

	/* then write the variable message body */
#ifdef POSIX_LOG_TRUNCATE
	if (entry->log_format == POSIX_LOG_STRING
	    && (entry->log_flags & POSIX_LOG_TRUNCATE) != 0) {
		/*
		 * buf contains a string that was truncated to
		 * POSIX_LOG_ENTRY_MAXLEN bytes.  Stick a null character at the
		 * end of our copy of the buffer to make a null-terminated
		 * string.
		 * Note: We can't do this in posix_log_write() because the
		 * caller's buffer is const.
		 * TODO: Get approval for this change in the POSIX draft.
		 */
		bcopy((void *)buf, (void *)writebuf, entry->log_size);
		writebuf[POSIX_LOG_ENTRY_MAXLEN - 1] = '\0';
		if (write(sd, writebuf, entry->log_size) != entry->log_size) {
		/* socket is broken */
			fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
			return EIO;
		}
		return 0;
	}
#endif
	if (entry->log_size > 0) {
		if (write(sd, buf, entry->log_size) != entry->log_size) {
			/* socket is broken */
			fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
			return EIO;
		}
	}
	return 0;
}

int
_evlWriteEx( struct posix_log_entry * entry, const char * buf)
{
	int ret = 0;
	sigset_t oldset;
	int sigsBlocked;
	int sd = -1;
	unsigned char c=0x0;

	_evlLockMutex(&logdSocketMutex);

	/* Mask all signals so we don't get interrupted */
	sigsBlocked = (_evlBlockSignals(&oldset) == 0);

	sd = getSdByPid(entry->log_pid);
	if (sd < 0) {
		ret = ECONNREFUSED;
		goto unlock_and_exit;
	}

	ret = _evlWriteEx2(sd, entry, buf);
	if (ret == EIO) {
		goto err_exit;
	}
	/* The daemon should tell the client that he finishes reading */ 
	read(sd, &c, sizeof(char));
	if(c != 0xac) {
		ret = EIO;
	}
	
err_exit:
	if (ret != 0) {
		/* Got valid descriptor, but subsequent write/read failed. */
		close(sd);	
		zapSd(sd);
	}
unlock_and_exit:
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	_evlUnlockMutex(&logdSocketMutex);

	return ret;
}

/*
 * Return the evlogd socket descriptor for the thread with process ID = pid.
 * Called with logdSocketMutex held and signals blocked.
 */
static int getSdByPid(pid_t pid)
{
	int sd;
	struct sdMapping *m;
	evl_listnode_t *head = sdMap, *end, *p;
	struct sockaddr_un _evl_log_sock;

	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		m = (struct sdMapping *) p->li_data;
		if (pid == m->pid) {
			return m->sd;
		}
	}

	/* No socket descriptor for this pid.  Create one. */
	if ((sd = _establishNonBlkConnection(EVLOGD_EVTSERVER_SOCKET, 
			&_evl_log_sock, 1)) < 0) {
		return -1;
	}
	m = (struct sdMapping *) malloc(sizeof(struct sdMapping));
	assert(m != NULL);
	m->sd = sd;
	m->pid = pid;
	sdMap = _evlAppendToList(sdMap, m);
	return sd;
}

/*
 * Something went wrong with the indicated socket.  Remove it from our list.
 * Called with logdSocketMutex held and signals blocked.
 */
static void zapSd(int sd)
{
	struct sdMapping *m;
	evl_listnode_t *head = sdMap, *end, *p;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		m = (struct sdMapping *) p->li_data;
		if (m->sd == sd) {
			sdMap = _evlRemoveNode(p, sdMap, NULL);
			free(m);
			return;
		}
	}
	fprintf(stderr, "libevl:zapSd: Couldn't find sd=%d\n", sd);
}
