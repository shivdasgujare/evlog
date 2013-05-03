#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/klog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "config.h"
#include "linux/evl_log.h"

#define EVLOGD_EVTSERVER_SOCKET		EVLOG_STATE_DIR "/evlogdevtsocket"
#define NFY_ACCESS_DENIED		0xfa
#define NFY_ACCESS_GRANTED		0xac
#define NFY_MAX_CLIENTS			0xca

/*
 * Globals for grabbing original syslog function locations
 */
static void *glibc;
typedef void (*syslog_p)(int, const char *, ...);
static syslog_p glibc_syslog;
typedef void (*vsyslog_p)(int, const char *, va_list);
static vsyslog_p glibc_vsyslog;
typedef void (*openlog_p)(const char *, int, int);
static openlog_p glibc_openlog;

static int _evlSysWriteEx( struct posix_log_entry * entry, const char * buf);
static int _evlSysWriteEx2(int sd, struct posix_log_entry *entry, const char *buf);

static int _evlSysBlockSignals(sigset_t *oldset);
static void _evlSysRestoreSignals(sigset_t *oldset);
static int _evlSysGetProcId();
static int _nonBlkConnection(const char *socketpath, struct sockaddr_un *sa, int nsec);

/*
 * Globals copied from glibc-2.2.3/misc/syslog.c
 */
static int	LogMask = 0xff;		/* mask of priorities to be logged
								 */
static int	LogFacility = LOG_USER;	/* default facility code */
static int 	LogOption = 0;
extern char *__progname;
static const char *LogIdent = NULL;

static char libclibrary[40];
static char *configFile = "/etc/evlog.d/libevlsyslog.conf";

void trim(const char * str)
{
	char const *psrc = str;
	char *pdes = (char *) str;
	int i, len;

  	/* remove leading space */
	while(*psrc == ' ' || *psrc == '\t') {
		psrc++;
	}
	/* remove the trailing space */
	len = strlen(psrc);
	while (*(psrc+len-1) == ' ' || *(psrc+len-1) == '\t') {
		--len;
	}
	for (i=0 ; i < len; i++) {
		pdes[i] = psrc[i];
	}
	pdes[i] = '\0';
}

int _evlreadconf()
{
	char line[256];
	FILE *f;
	
	if ((f = fopen(configFile, "r")) == NULL) {
		fprintf(stderr, "can't open libevlsyslog.conf file. \n");
		return -1;
	}
	while (fgets(line, 256, f) != NULL) {
		if (line[0] == '#' || line[0] == '\n' 
			|| strcspn(line, "") == 0) {
				continue;
			}
		/* replace newline with null char */
		if (line[strlen(line) -1] == '\n') {
			line[strlen(line) -1] = '\0';
		}
		trim(line);
		strcpy(libclibrary, line);
		fclose(f);
		return 0;
	}
	fclose(f);
	return -1;
}

void undo_preload()
{
	FILE * f, *tmpf;
	char line[256];
	
	if ((f = fopen("/etc/ld.so.preload", "r")) == NULL) {
		fprintf(stderr, "can't open ld.so.preload file. \n");
		return;
	}

	if ((tmpf = fopen("/etc/ldsopreload~", "w")) == NULL) {
		fprintf(stderr, "can't create temporary file. \n");
		return;
	}
	
	fprintf(stderr, "undo preloading 'libevlsyslog.so' now ..."); 
	while (fgets(line, 256, f) != NULL) {
		trim(line);
		if (strstr(line, "libevlsyslog.so") == NULL) { 
			fputs(line, tmpf);
		}
	}
	fclose(f);
	fclose(tmpf);
	unlink("/etc/ld.so.preload");
	rename("/etc/ldsopreload~", "/etc/ld.so.preload");
	fprintf(stderr, "Done.\n");
}

/*
 * Library Initialization
 * Just get the symbol addresses for the original syslog and vsyslog
 functions
 * in glibc.
 */
void
_init(int argc, char *argv[], ...)
{
	if (_evlreadconf() == -1) {
		undo_preload();
		exit(1);
	}

	if (!(glibc = dlopen(libclibrary, RTLD_LAZY))) {
		fprintf(stderr, "%s\n", dlerror());
		undo_preload();
		exit(1);
	}
	/*
	  if (!(glibc_syslog = dlsym(glibc, "syslog"))) {
	  fprintf(stderr, "%s\n", dlerror());
	  exit(2);
	  }
	*/
	if (!(glibc_vsyslog = dlsym(glibc, "vsyslog"))) {
		fprintf(stderr, "%s\n", dlerror());
		undo_preload();
		exit(3);
	}
	if (!(glibc_openlog = dlsym(glibc, "openlog"))) {
		fprintf(stderr, "%s\n", dlerror());
		undo_preload();
		exit(4);
	}

}

/*
 * Library Finalization
 * Close our handle to glibc
 */
void
_fini(void)
{
	dlclose(glibc);
}

/*
 * vwrite_evlog -- is called with in syslog func, it writes a log record
 * directly into the kernel's event log buffer, it assumes the evlog patch
 * for the kernel was install. If the patch is not install this function is
 * probably not even built.
 *
 */
void vwrite_evlog(int pri, const char * fmt, va_list ap)
{
	char evlbuf[POSIX_LOG_ENTRY_MAXLEN + REC_HDR_SIZE + 1];
	char *plog_data, *pheader;
	int log_data_size = 0;

	struct posix_log_entry entry;
	int gotTimeStamp = 0;
	int writeStatus;
	unsigned int flags = 0;


#define	INTERNALLOG	LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
	/* Check for invalid bits. */
	if (pri & ~(LOG_PRIMASK|LOG_FACMASK)) {
		pri &= LOG_PRIMASK|LOG_FACMASK;
	}

	/* Check priority against setlogmask values. */
	if ((LOG_MASK (LOG_PRI (pri)) & LogMask) == 0)
		return;

	/* Set default facility if none specified. */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* We currently don't log AUTHPRIV */
	//	if (LOG_FAC(pri) << 3 == LOG_AUTHPRIV )
	//		return;


	/* Fill the buffer with log data first so we can get the length */
	pheader = evlbuf;
	plog_data = pheader + REC_HDR_SIZE;
	if (LogIdent == NULL)
       		LogIdent = __progname;
	strcpy(plog_data, LogIdent);
	log_data_size = strlen(plog_data);
	plog_data +=log_data_size;
	if (LogOption & LOG_PID) {
		sprintf(plog_data, "[%d]", getpid());
		log_data_size += strlen(plog_data);
		plog_data += strlen(plog_data);
	}
	if (LogIdent) {
		*plog_data++ = ':';
		*plog_data++ = ' ';
		log_data_size += 2;
	}	
		
	log_data_size += vsnprintf(plog_data, POSIX_LOG_ENTRY_MAXLEN, fmt, ap);

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

	/*
	 * Also get the processor ID ASAP, before this process migrates
	 * to another processor.
	 *
	 */
	/*	entry.log_processor = 0; */
	entry.log_processor = _evlSysGetProcId();
	if (log_data_size > POSIX_LOG_ENTRY_MAXLEN) {
#ifdef POSIX_LOG_TRUNCATE
		log_data_size = POSIX_LOG_ENTRY_MAXLEN;
		flags |= POSIX_LOG_TRUNCATE;
#else
		return;
#endif
	}

	/* Verify that the specified format is appropriate. */
	if (log_data_size == 0) {
		/* No data.  Format is NODATA, no matter what caller says.*/
		entry.log_format = POSIX_LOG_NODATA;
	} else {
		entry.log_format = POSIX_LOG_STRING;
	}

	entry.log_flags = flags;
	entry.log_size = log_data_size + 1; 	/* add one for the null char */

	entry.log_event_type = EVL_SYSLOG_MESSAGE;
	entry.log_facility = LOG_FAC(pri) << 3;
	entry.log_severity = LOG_PRI(pri);

	entry.log_uid = /*__*/geteuid();
	entry.log_gid = /*__*/getegid();
	entry.log_pid = /*__*/getpid();
	entry.log_pgrp = getpgrp();

#ifdef _POSIX_THREADS
	entry.log_thread = pthread_self();
#else
	entry.log_thread = 0;
#endif

	_evlSysWriteEx(&entry, evlbuf + REC_HDR_SIZE);

}

/*
 * Function overriding
 */

/*
 * we need to override openlog in order to get the log facility
 */
void
openlog (const char *ident, int logstat, int logfac)
{
	LogFacility = logfac;
	LogOption = logstat;
	if (ident != NULL)
		LogIdent = ident;
	else 
		LogIdent = __progname;
	glibc_openlog(ident, logstat, logfac);
}

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */
void
#if __STDC__
syslog(int pri, const char *fmt, ...)
#else
	 syslog(pri, fmt, va_alist)
	 int pri;
	 char *fmt;
	 va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vsyslog(pri, fmt, ap);
	va_end(ap);
}
void
vsyslog(pri, fmt, ap)
	 int pri;
	 register const char *fmt;
	 va_list ap;
{	
	va_list clone_ap;
	va_copy(clone_ap, ap);	
	vwrite_evlog(pri, fmt, ap);
	glibc_vsyslog(pri, fmt, clone_ap);
	va_end(clone_ap);
}


/* Block signals while reading the registry, since fgets isn't signal-safe. */
static int
_evlSysBlockSignals(sigset_t *oldset)
{
	int status;
	sigset_t allSigs;

	(void) sigfillset(&allSigs);
#ifdef _POSIX_THREADS
	status = pthread_sigmask(SIG_BLOCK, &allSigs, oldset);
	if (status != 0) {
		errno = status;
		perror("pthread_sigmask");
		return -1;
	}
#else
	status = sigprocmask(SIG_BLOCK, &allSigs, oldset);
	if (status != 0) {
		perror("sigprocmask");
		return -1;
	}
#endif
	return 0;
}

static void
_evlSysRestoreSignals(sigset_t *oldset)
{
	int status;
#ifdef _POSIX_THREADS
	status = pthread_sigmask(SIG_SETMASK, oldset, NULL);
	if (status != 0) {
		errno = status;
		perror("pthread_sigmask (unblock)");
	}
#else
	status = sigprocmask(SIG_SETMASK, oldset, NULL);
	if (status != 0) {
		perror("sigprocmask (unblock)");
	}
#endif
}

/* Return the proc id */
static int 
_evlSysGetProcId()
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
	
	return (int) strtol(last_token, (char **)NULL, 10);
}

/* 
 * Establish a socket connection - This is a non-blocking connect.
 */
static int
_nonBlkConnection(const char *socketpath, struct sockaddr_un *sa, int nsec)
{
	int sd;
	socklen_t sock_len, len;  /* Size of generic UD address structure */

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
			return -EAGAIN;
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
		close(sd);
		return -EAGAIN;
	}

	if (FD_ISSET(sd, &rset) || FD_ISSET(sd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			/* some error */
			(void)fprintf(stderr, "Error connecting to the daemon.\n");
			return -EAGAIN;
		}
	} else {
		return -EAGAIN;
	} 

 done:
	/* Restore flags and set the close-on-exec flag on sd. */
	fcntl(sd, F_SETFL, flags);
	flags = fcntl(sd, F_GETFD);
	flags |= FD_CLOEXEC;
	if (fcntl(sd, F_SETFD, flags) == -1) {
		perror("fcntl(F_SETFD)");
		return -EAGAIN;
	}	
	return sd;
}


static int 
_evlSysWriteEx2(int sd, struct posix_log_entry *entry, const char *buf)
{
	int n;
	char writebuf[POSIX_LOG_ENTRY_MAXLEN];

	/* First write the header */
	if ((n = write(sd, entry, REC_HDR_SIZE)) != REC_HDR_SIZE) {
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
		if ((n = write(sd, buf, entry->log_size)) != entry->log_size) {
			/* socket is broken */
			fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
			return EIO;
			
		}
		return 0;
	}
#endif
	if (entry->log_size > 0) {
		if ((n = write(sd, buf, entry->log_size)) != entry->log_size) {
			/* socket is broken */
			fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
			return EIO;
		}
	}
	return 0;
}

static int 
_evlSysWriteEx( struct posix_log_entry * entry, const char * buf)
{
	int ret = 0;
	sigset_t oldset;
	int sigsBlocked;
	struct sockaddr_un _evl_log_sock;
	int sd = -1;
	unsigned char c=0x0;
	/* Mask all signals so we don't get interrupted */
	sigsBlocked = (_evlSysBlockSignals(&oldset) == 0);


	if ((sd = _nonBlkConnection(EVLOGD_EVTSERVER_SOCKET, 
								&_evl_log_sock, 1)) < 0) {
		ret = ECONNREFUSED;	
		goto err_exit;
	}

	ret = _evlSysWriteEx2(sd, entry, buf);

	/* The daemon should tell the client that he finishes reading */ 
	read(sd, &c, sizeof(char));
	if(c != 0xac) {
		ret = EIO;
	}

	close(sd);
	
 err_exit:
	if (sigsBlocked) {
		_evlSysRestoreSignals(&oldset);
	}

	return ret;
}


