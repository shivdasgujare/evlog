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

#include <stdio.h>	/* Defines NULL */
#include <stdarg.h>	/* Defines va_args */
#include <fcntl.h>   	/* Defines FILE, fopen() */
#include <strings.h>  	/* Defines bcopy() */
#include <errno.h>      /* Defines errno */
#include <signal.h>     /* Defines signal handler structures */
#include <unistd.h>     /* Defines fork() */
#include <malloc.h>     /* Defines malloc() */
#include <stdlib.h>     /* Defines atexit() and exit() */
#include <sys/stat.h>   /* Defines chmod() */
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <assert.h>
#ifndef __need_timeval
#define __need_timeval
#endif
#include <sys/socket.h>
#include <sys/un.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_common.h"
#include "callback.h"
#include "ksyms.h"
#include "evl_kernel.h"

#define __LIBRARY__
#include <linux/unistd.h>
#if !defined(__GLIBC__)
# define __NR_ksyslog __NR_syslog
_syscall3(int,ksyslog,int, type, char *, buf, int, len);
#else
#include <sys/klog.h>
#define ksyslog klogctl
#endif

#if defined(__ia64__) || defined(_x86_64__)
#define __STRUCT_ALIGN__
#endif

#ifdef __i386__
#define LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_I386)
#define LOCAL_ARCH 	LOGREC_ARCH_I386
#elif defined(__s390__)
#define	LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_S390)
#define LOCAL_ARCH 	LOGREC_ARCH_S390
#elif defined(__s390x__)
#define	LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_S390X)
#define LOCAL_ARCH 	LOGREC_ARCH_S390X
#elif defined(__ppc__)
#define	LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_PPC)
#define LOCAL_ARCH 	LOGREC_ARCH_PPC
#elif defined(__powerpc__)
#define	LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_PPC)
#define LOCAL_ARCH 	LOGREC_ARCH_PPC
#elif defined(__ia64__)
#define	LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_IA64)
#define LOCAL_ARCH 	LOGREC_ARCH_IA64
#elif defined(__x86_64__) 
#define LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_X86_64)
#define LOCAL_ARCH 	LOGREC_ARCH_X86_64
#elif defined(__arm__) && defined(__ARMEB__)
#define LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_ARM_BE)
#define LOCAL_ARCH 	LOGREC_ARCH_ARM_BE
#elif defined(__arm__) && !defined(__ARMEB__)
#define LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_ARM_LE)
#define LOCAL_ARCH 	LOGREC_ARCH_ARM_LE
#else
#define	LOG_MAGIC_ARCH  ((LOGREC_MAGIC & 0xffff0000) | LOGREC_NO_ARCH)
#define LOCAL_ARCH 	LOGREC_NO_ARCH
#endif
	
/* *_DUP_INTERVAL 	Time to wait until writing discard event.
 * *_DUP_COUNT		# of duplicates to discard before 
 * 			writing discard event.
 * *_DUP_ARRAY_SIZE	Size of duplicates lookback buffer.
 * *_DUP_TIMEOUT	Period during which an event may be considered a
 * 			duplicate.
 */
#define DEFAULT_DUP_INTERVAL		60
#define DEFAULT_DUP_COUNT		10
#define DEFAULT_DUP_ARRAY_SIZE		5
#define DEFAULT_DUP_TIMEOUT		180
#define MAX_DUP_INTERVAL		3600
#define MAX_DUP_COUNT			10000
#define MAX_DUP_ARRAY_SIZE		1000
#define MAX_DUP_TIMEOUT			10800
#define CONSOLE_OUTPUT_DISABLE		111

#define MAX_RECSIZE (REC_HDR_SIZE + POSIX_LOG_ENTRY_MAXLEN)
#define TRUE	1
#define FALSE   0

#define MAX_EVL_LINE 	1024
#define DIS_COUNT 			0
#define DIS_INTERVAL 		1
#define SEV_LEVEL			2
#define DIS_SIZE			3
#define DIS_DUPS			4
#define EVENT_SCREEN		5

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stdout, fmt, ##args)
#else
#define TRACE(fmt, args...)		/* fprintf(stdout, fmt, ##args) */
#endif

#define MAX_USER_EVLBUF		512 * 1024
#define MAX_USERPRIV_EVLBUF		256 * 1024

#define MAX_CLIENTS					240
#define CLIENT_SLOT_AVAILABLE		-1
#define SHMSZ		4

typedef struct clientinfo {
	int sd;
	uid_t uid;
	pid_t pid;
} clientinfo_t;

typedef struct dup_buffer {
	char *rec;
	int dup_count;
	int buf_index;
	time_t time_start;
} dup_buffer_t;

/* TIMING */
struct timeval perf_time_start;
struct timeval perf_time_end;
struct timeval perf_time;

static clientinfo_t clients[MAX_CLIENTS];
static int maxci;
static pid_t kpid;
static pid_t rmtdpid = 0;

char * sharedData;
int waitForKids = 1;

/* Function prototype */
static void initClients();
static int getMaxClientIndex();
static void closeClientSocket(clientinfo_t *);
static void processKernelEvents();
int writeKernEvt(int sd, struct posix_log_entry *entry, const char *buf);
static void sigChild_handler();
static void checkDupEventTimeout(struct timeval *tv);
void getRecHeader(const char *buf, struct posix_log_entry *entry,
				  size_t *hdr_size);

/* backendmgr */
extern int be_init(void);
extern void be_run(const char *data1, const char *data2, evl_callback_t callback);
extern void be_cleanup(void);

typedef struct userbuf_info {
	char *buf;
	char *watermark;
	int dropped_evt;
	int fd;				/* log file descriptor */
} userbuf_info_t;

static userbuf_info_t user_evlbuf = {NULL, NULL, 0, -1};
static userbuf_info_t userpriv_evlbuf = {NULL, NULL, 0, -1};
	 
/* Storage buffer for the bytes read from the kernel along with those stored for
 * duplicate removal.
 */
typedef struct event_buffer {
	char rec[MAX_RECSIZE];
	char in_use;
#ifdef __ia64__
	char padding[7];
#endif
} event_buffer_t;

event_buffer_t *buf;
int buf_size;

typedef struct log_info {
	int fd;
	int dup_count;
	int buf_index;
	posix_log_recid_t recId;	/* This field is also used for 
								 * identify the log type for
								 * getNextRecId
								 */
	log_header_t	  *map_hdr;	/* Map to the log header structure */
	dup_buffer_t *dup_recs_array;
} log_info_t;

static posix_log_recid_t gRecId;

log_info_t pvt_log;
log_info_t evl_log;
int dup_count = DEFAULT_DUP_COUNT;
int dup_interval = DEFAULT_DUP_INTERVAL;
int dup_timeout = DEFAULT_DUP_TIMEOUT;
int dup_array_size = DEFAULT_DUP_ARRAY_SIZE;
int dis_dup_recs = TRUE;
int debug_on     = TRUE;
int console_sev_level = CONSOLE_OUTPUT_DISABLE;		/* default is disable */
char query_exp[MAX_EVL_LINE];
posix_log_query_t dis_query;

static int notify_sd = 0;     /* socket descriptor for communicate with notification daemon */
int maxsd;
fd_set all_sds;
fd_set read_sds;

/* 
 * Prototypes for internal functions 
 */

static int evlogdstopdaemon();
static void NewSIGAction();

static int readEvlogConfig();
static int Process_Event(clientinfo_t *, char *, int);

static int log_evl_rec(log_info_t *, char *);
static int openMaybeCreate(const char *, mode_t , log_info_t *);
static void mk_evl_rec(log_info_t *linfo, posix_log_facility_t, int, 
					   posix_log_severity_t, const char *, ...);
static int updateConfvalues(int);
static void writeEvtToNfyDaemon(char *lbuf);
static int createSocket(char *sockname, int *sd, socklen_t *sock_len, 
						struct sockaddr_un *sa, mode_t mode, int backlog);
static int rmSocket(int);

static void dumpDuplicatesBuffer(log_info_t *);
static void reallocBuffer(size_t, int, int);
static int isReplaceable(dup_buffer_t *, dup_buffer_t *, time_t, int *);
static int compute_score(dup_buffer_t *, time_t);
static int dropDupRecord(struct posix_log_entry *, log_info_t *, int);

int writeEx(int sd, void * buf, size_t len);
int lock_routine(int fd, int cmd, int type);
int isRTC_Local();
char *getProcessCmd(pid_t pid);
void reload_daemon();

static char *confPath = LOG_EVLOG_CONF_DIR "/evlog.conf";
static char *PidFile = "/var/run/evlogd.pid";

static int defaultPosixLog = 1;	/* posix log is default behavior */
int lookup_symbol = 1;		/* resolve kernel addresses to symbols */
static int symbol_twice = 0;	/* if 1, print original kernel address again */
static char * system_map = NULL;/* your own System.map instead of the default */

/* TIMING */
/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */

int
timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	   tv_usec  is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/*
 * Daemonize evlogd
 */
void 
_daemonize()
{
	pid_t pid;
	int i;
	int devnull;
		
	/*
	 * Fork a child and let the parent exit. This guarentees that
	 * the first child is not a process group leader.
	 */
	if ((pid = fork()) < 0) {
		fprintf(stdout, 
				"evlogd: Cannot fork child process. Check system process usage.\n"); 
		exit(1);
	} else if (pid > 0) {
		exit (0);
	}

	/*
	 * First child process.
	 * 
	 * Disassociate from controlling terminal and process group.
	 * Ensure the process can't reacquire a new controlling terminal
	 */

	(void)setpgrp();

	/* 
	 * Immunize from process group leader death. 
	 */

	(void)signal(SIGHUP, SIG_IGN);

	if ((pid = fork()) < 0) {
		fprintf(stderr,
				"evlogd: Cannot fork child process. Check system process usage.\n"); 
		exit(1);
	} else if (pid > 0) {
		exit(0);
	}

	TRACE("Determining PID.\n");
	/* stuck my process id away */
	if (!_evlUpdate_pid(PidFile)) {
		mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_WRITE_PID, 
				   LOG_CRIT,"evlogd: Cannot write 'evlogd' PID to '%s' file\n",
				   PidFile); 
		exit(1);
	}

	/* 
	 * Second child process. 
	 * 
	 *
	 * redirect fd 0 1 2 to /dev/null .  
	 */
#ifndef DEBUG2	
	devnull = open("/dev/null", O_WRONLY);
	if (devnull != -1) {
		for (i=0; i <= 2; i++) {
			(void)close(i);
			dup2(devnull, i);
		} 
		close(devnull);
	}
#endif
	/*
	 * Clear any inherited file mode creation mask.
	 */

	(void)umask(0);

	(void)mkdir(LOG_EVLOG_DIR, 755);
	(void)chdir(LOG_EVLOG_DIR);
}

/* 
 * Initialize the log files (eventlog and privatelog)
 */
void
initLogFiles()
{
	static char log_file_name[FILENAME_MAX] = LOG_CURLOG_PATH;
	static char privatelog[FILENAME_MAX] = LOG_PRIVATE_PATH;
	
	memset(&dis_query, 0, sizeof(posix_log_query_t));
	memset(&evl_log, 0, sizeof(log_info_t));
	if (openMaybeCreate(log_file_name, 0644, &evl_log) < 0) {
		if (evl_log.fd >= 0) {
			(void) flock(evl_log.fd, LOCK_UN);
			close(evl_log.fd);	
		}
		exit(1);
	}
	user_evlbuf.fd = evl_log.fd;

	memset(&pvt_log, 0, sizeof(log_info_t));
	if (openMaybeCreate(privatelog, 0600, &pvt_log) < 0) {
		if (pvt_log.fd >= 0) {
			(void) flock(pvt_log.fd, LOCK_UN);
			close(pvt_log.fd);
		}
		exit(1);
	}
	userpriv_evlbuf.fd = pvt_log.fd;

	if (pvt_log.recId == 0 && evl_log.recId == 0) {
		gRecId = 0;
		
	} else {
		gRecId = MAX(pvt_log.recId, evl_log.recId);
	}
	/*
	 * Set this field for identifying the log type (regular or private).
	 * Pass this to the getNextRecId function to get the
	 * next record id.
	 */
	evl_log.recId = 1;	
	pvt_log.recId = 0;
	
	/* Initially the query tree is null */
	dis_query.qu_tree = NULL;
	if (readEvlogConfig() != 0) {
		fprintf(stderr, "evlogd: Read config file failed\n");
		exit(1);
	}
}

/*
 * Initialize shared memory region (for IPC between child and parent)
 */
void 
initSharedMem()
{
	key_t key;
	char *shm;
	int shmid, i;
	key = 6712;
	
	shmid = shmget(key, SHMSZ, IPC_CREAT | 0600);
	shm = shmat(shmid, NULL, 0);
	sharedData = shm;
	for (i=0; i < SHMSZ; i++) {
		sharedData[i] = 0;
	}
}

/*
 * Create new signal handler for SIGTERM. This will remove dr_socket and
 * terminate gracefully.
 */
void
initSignals()
{
	
	static struct sigaction SigAllAction; /* Signal handler to terminate gracefully */
	static struct sigaction act;
	static struct sigaction SigChild; 	  /* Signal handler SIGCHLD */
	void sigChild_handler();
	void NewSIGAction();
	
	(void)memset(&SigAllAction, 0, sizeof(struct sigaction));
	SigAllAction.sa_handler = NewSIGAction;
	SigAllAction.sa_flags = 0;

	if (sigaction(SIGTERM, &SigAllAction, NULL) < 0){
		mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_SIG_ACT, LOG_WARNING,
				   "evlogd: WARNING - sigaction failed for SIGTERM.\n"); 
	}
	if (sigaction(SIGINT, &SigAllAction, NULL) < 0){
		mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_SIG_ACT, LOG_WARNING,
				   "evlogd: WARNING - sigaction failed for new SIGINT.\n"); 
	}
	if (sigaction(SIGHUP, &SigAllAction, NULL) < 0){
		mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_SIG_ACT, LOG_WARNING,
				   "evlogd: WARNING - sigaction failed for new SIGHUP.\n");
	}

	/*
	 * Create signal handler to handle SIGCHILD - 
	 */
	(void)memset(&SigChild, 0, sizeof(struct sigaction));
	SigChild.sa_handler =sigChild_handler;
	SigChild.sa_flags = 0;
	if (sigaction(SIGCHLD, &SigChild, NULL) < 0){
		mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_SIG_ACT, LOG_WARNING,
				   "evlogd: WARNING - sigaction failed for SIGCHLD.\n"); 
	}

	/* Ignore SIGUSR1, SIGUSR2 except process kpid */
	signal (SIGUSR1, SIG_IGN);
	signal (SIGUSR2, SIG_IGN);

	/* Ignore SIGPIPE */
	(void) memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	if (sigaction(SIGPIPE, &act, NULL) < 0){
		(void)fprintf(stderr, "evlogd: sigaction failed for new SIGPIPE.\n");
		perror("sigaction");
		exit(1);
	}
}

/* 
 * Run the child process - This process handles the kernel events
 */
void 
run_child()
{
	int shmid;
	key_t key;
	char *shm, *s;
	
	
	/* Setting up shared mem */
	key = 6712;
	shmid = shmget(key, SHMSZ, IPC_CREAT | 0600);
	shm = shmat(shmid, NULL, 0);
	s = shm;

	/* Handle reload symbol table signals */
	signal (SIGUSR1, reload_daemon);
	signal (SIGUSR2, reload_daemon);
	/* Ignore some signals */
	signal(SIGTERM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	TRACE("pause \n");
	/* wait for the parent to complete his setup */ 
	while(s[1] != 0x0f)
		sleep(1);
	TRACE("unpause, s[1]=%d\n", s[1]);		
				
	processKernelEvents();
	TRACE("*** This is the end for the child\n");
	/* 
	 * Mark the first byte in the shared mem with 0x0e so that
	 * the parent knows child exits
	 */
	*shm = 0x0e;		
					
	_exit(0);		
}

/*
 * The parent infinite loop 
 */
void
run_parent(uid_t max_sysuid, int max_userconns)
{
	struct timeval tv;
	int ns_sd;			/* notification socket descriptor */
	int conf_sd;			/* Configuration socket descriptor */
	socklen_t ns_sock_len, conf_sock_len;
	struct sockaddr_un ns_sock, conf_sock;
	int buf_index = 0;
	int listen_sd, newsd;
	socklen_t listen_sock_len;
	struct sockaddr_un listen_sock;
	int conf_socket = -1;
	event_buffer_t *buf_ptr;
	/* TIMING */
	struct timeval perf_time_temp;
	perf_time.tv_sec = 0;
	perf_time.tv_usec = 0;

	/* Allocate the buffer to handle event reads and duplicates. */
	reallocBuffer(dup_array_size, FALSE, dis_dup_recs);
	buf_ptr = buf;
	
	/* Establish a connection with notify daemon */
	notify_sd = establishNfyConnection(&ns_sock);

	(void)unlink(EVLOG_CONF_SOCKET);
	if (createSocket(EVLOG_CONF_SOCKET, &conf_sd, &conf_sock_len,
					 &conf_sock, 0600, 0) < 0) {
		exit(1);
	}
	/* Listen to clients that are writing events to the log */
	(void)unlink(EVLOGD_EVTSERVER_SOCKET);
	if (createSocket(EVLOGD_EVTSERVER_SOCKET, &listen_sd, &listen_sock_len,
					 &listen_sock, 0666, MAX_CLIENTS) < 0) {
		exit(1);
	}

	FD_ZERO(&all_sds);
	if ( notify_sd > 0) {
		FD_SET(notify_sd, &all_sds);
	}
	FD_SET(conf_sd, &all_sds);		
	FD_SET(listen_sd, &all_sds);
	maxsd = MAX(notify_sd, conf_sd);
	maxsd = MAX(maxsd, listen_sd);

	/* Now the parent process is ready tell the child to continue */
	sharedData[1] = 0x0f;
	 
	for(;;) {
		int nsel, i;
		struct ucred ucred;	/* evlog client credential */
		socklen_t ucredsz = sizeof(struct ucred);	
		struct timeval tv, *ptv;
		
		checkDupEventTimeout(&tv);
		ptv = (tv.tv_sec == 0 ? NULL : &tv);

		if (notify_sd <= 0) {
			fprintf(stderr, "Trying to connect to evlnotifyd.\n");
			if ((notify_sd = establishNfyConnection(&ns_sock)) > 0) {
				FD_SET(notify_sd, &all_sds);
				maxsd = MAX(notify_sd, maxsd);
			} 
			else {
				tv.tv_sec = 2;
				tv.tv_usec = 0;
				ptv = &tv;
			}	
		}

		bcopy((char *)&all_sds, (char *)&read_sds, sizeof(all_sds));
		if ((nsel = select(maxsd + 1, &read_sds, (fd_set *) 0, 
						   (fd_set *) 0, ptv)) < 0) {
			if (errno == EINTR) {
				/*
				 * I get EINTR when the (kernel) child process exits, the parent 
				 * gets a SIGCHLD, and handle it properly. Just continue.
				 */
				TRACE("got EINTR\n");
				continue;
			} else {
				(void)fprintf(stderr, "evlogd: selection of specified file descriptors failed.\n");
				perror("select");
				if (errno != EBADF)
					exit(1);
			}
		}
		if (nsel == 0) {
			/* timeout */
			TRACE("select timeout\n");
			continue;
		}

		if (FD_ISSET(listen_sd, &read_sds)) {
			/* Client is trying to connect. */
			clientinfo_t *cl, *empty;
			char abyte;
			int userconns, idx;
			
			if ((newsd = accept(listen_sd, NULL, NULL)) < 0) {
				perror("accept");
				goto exit_new_sd;
			}
			TRACE("Client is connecting.\n");	
			if (getsockopt(newsd, SOL_SOCKET, SO_PEERCRED, &ucred, &ucredsz) < 0) {
				perror("getsockopt");
			} else {
				TRACE("uid = %d, pid = %d\n", ucred.uid, ucred.pid);
			} 

			/* Record the pid of evlogrmtd */
			if (rmtdpid == 0 && ucred.uid == 0) {
				char *cmdline;
				
				if ((cmdline = getProcessCmd(ucred.pid)) != NULL) {
					if (strstr(cmdline, "evlogrmtd")) {
						rmtdpid = ucred.pid;
					}
					free(cmdline);
				}
			}
				
			/* Look for a free client slot; while doing
			 * so we count the number of connections
			 * per this uid, as well as recomputing the
			 * max client index. */
			empty = NULL;
			cl = clients;
			userconns = 0;
			maxci = 0;
			for (idx = 0; idx < MAX_CLIENTS; idx++, cl++) {
				if (cl->sd != CLIENT_SLOT_AVAILABLE)
					maxci = idx;
				else if (empty == NULL) {
				 	empty = cl;
					maxci = idx;
				}
				if (cl->uid == ucred.uid)
					userconns ++;
			}	
			if (empty == NULL) {
				TRACE("Max number of clients reached.\n");
				close(newsd);
				goto exit_new_sd;	
			}
			/* Do not allow non-system users to
			 * have too many connections open
			 * simultaneously; otherwise they might
			 * eat all our file descriptors and
			 * disable logging entirely. */
			if (ucred.uid > max_sysuid
				&& userconns >= max_userconns) {
				TRACE("Too many simultaneous connections by uid %u\n", ucred.uid);
				close(newsd);
				goto exit_new_sd;
			}

			TRACE("sd %d in slot %d, maxci = %d\n", newsd, empty - clients, maxci);
			empty->sd = newsd;
			empty->uid = ucred.uid;
			empty->pid = ucred.pid;
			
			FD_SET(newsd, &all_sds);
			if (maxsd < newsd) {
				maxsd = newsd;
			}
		
		exit_new_sd: /* semicolon makes gcc happy */ ;
		}

		/* notification daemon  goes away */
		if (notify_sd > 0 && FD_ISSET(notify_sd, &read_sds)) {
			notify_sd = rmSocket(notify_sd);
		}
		
		if (FD_ISSET(conf_sd, &read_sds)) {
			/*
			 * Accept the connection to the evlconfig command.
			 */
			if ((newsd = accept(conf_sd, NULL, NULL)) < 0){
				(void)fprintf(stderr, "evlogd: canot accept connection to evlconfig\n");
				perror("accept");
				exit(1);
			}
			FD_SET(newsd, &all_sds);
			if (maxsd < newsd) {
				maxsd = newsd;
			}
			TRACE("evlconfig starting...\n");
			/*
			 * Got the connection from evlconfig command. Update
			 * the following values for the next select call
			 * to read config data. After read the data, these
			 * values will be reset to 0's.
			 */
			conf_socket = newsd;
			continue;
		}
		
		if ((conf_socket != -1) && FD_ISSET(conf_socket, &read_sds)) { 
			if (updateConfvalues(conf_socket) < 0) {
				exit(1);
			}
			conf_socket = rmSocket(conf_socket);
			TRACE("evlconfig done.\n");
		}

		/* TIMING */
		gettimeofday(&perf_time_start, NULL);

		/* process clients */
		for (i= 0; i <= maxci; i++) {
			TRACE("maxci=%d , i = %d\n", maxci, i);
			if ( clients[i].sd != CLIENT_SLOT_AVAILABLE) {
				if (FD_ISSET(clients[i].sd, &read_sds)) {
					/*Determine next buf_index to use. */
					buf_ptr = buf;
					buf_index = 0;
					if (dis_dup_recs) {
						for (; buf_index < buf_size; buf_index++) {
							if (!buf_ptr->in_use)
								break;
							buf_ptr++;
						}
					}
					Process_Event(&clients[i], (char *)buf_ptr->rec, buf_index);
				}
			}
		}

		/* TIMING */
		gettimeofday(&perf_time_end, NULL);
		timeval_subtract(&perf_time_temp, &perf_time_end, &perf_time_start);
		perf_time.tv_sec += perf_time_temp.tv_sec;
		perf_time.tv_usec += perf_time_temp.tv_usec;
		if (perf_time.tv_usec > 1000000) {
			perf_time.tv_sec += perf_time.tv_usec / 1000000;
			perf_time.tv_usec = perf_time.tv_usec % 1000000;
		}
	} /* end for(;;) */
}

/*
 * MAIN:
 */
int
main(int argc, char **argv)
{
	int i;          /* Loop counter */
	pid_t pid;
	int bg = 1; /* Execute as a Daemon */
	auto int c;
	uid_t max_sysuid = 99;
	int max_userconns = 32;
	int becnt;
	
	while ((c = getopt(argc, argv, "fum:x2k:p")) != EOF) {
		switch (c) {
		case 'f':
			bg = 0;
			break;
		case 'u':
			defaultPosixLog = 0;
			break;
		case 'm':
			max_sysuid = atoi(optarg);
			break;
		case 'x':
			lookup_symbol = 0;
			break;
		case '2':
			symbol_twice = 1;
			break;
		case 'k':	/* kernel symbol file to replace the default */
			system_map = optarg;
			break;
		case 'p':	/* Load symbols on oops. */
			SetParanoiaLevel(1);
			break;
		}
	}

	initClients();
	initSharedMem();
	if (_evlValidate_pid(PidFile)) {
		fprintf(stderr, "evlogd: Already running.\n");
		exit(1);
	}
	if ( lookup_symbol ) {
		/* lookup symbols turned on */
		lookup_symbol = (InitKsyms(system_map) == 1);
		lookup_symbol |= InitMsyms();
		if (lookup_symbol == 0) {
			//fprintf(stderr, "Cannot find any symbols, turning off symbol lookups\n");
		}
	}
	if (bg) {
		_daemonize();
	}
	if (defaultPosixLog) {
		initLogFiles();	
	}

	initSignals();
	
	/* Initialize backends - by default the default posix log is enable
	 * unless the daemon is start with -u option */
	becnt = be_init();

	if (becnt <= 0 && defaultPosixLog == 0) {
		fprintf(stderr, "ERROR:evlogd needs atleast one backend plug-in for it to operate.\n");
		exit(1); 
	}

	/*
	 * Now it is time to fork another process for handling kernel events
	 */
	if ((kpid = fork()) < 0) {
		perror("fork");
		exit(1);
	}

	if (kpid == 0) {
		run_child();
	}
	else {
		/* This code executes in the parent process */
		if (kpid < 0) {
			fprintf(stderr, "Fail to start child process!\n");
		}
		run_parent(max_sysuid, max_userconns);
	}
	return 0;
}

/*
 * FUNCTION	: getNextRecId
 *
 * PURPOSE	: Returns the next recid for either regular or private log 
 * 			  an odd recid for regular log and an even recid for private
 * 			  log.
 *
 * ARGS		: Log type 0 = private log ; 1 = regular log
 *
 * RETURN	: Record ID
 *
 */
posix_log_recid_t 
getNextRecId(int logType)
{
	if (logType == 0) {
		/* private log - Even recid */
		(gRecId % 2)? (gRecId = gRecId + 1): (gRecId = gRecId + 2);
	} else if (logType == 1) {
		/* Regular log - Odd recid */
		(gRecId % 2)? (gRecId = gRecId + 2): (gRecId = gRecId + 1);
	}
	return gRecId;
}

/*
 * FUNCTION	: initClients
 *
 * PURPOSE	: Initializes the clients info array to some known state
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
static void
initClients()
{
 	int i;
 	int maxci = 0;
 	for (i=0; i < MAX_CLIENTS; i++) {
 		clients[i].sd = CLIENT_SLOT_AVAILABLE;
 	}
}

/* Compute max client index */
static int 
getMaxClientIndex()
{
	int i;

	for (i= MAX_CLIENTS - 1; i >= 0; i--) {
		if (clients[i].sd != CLIENT_SLOT_AVAILABLE)
			break;
	}
	return i;
}			 
/*
 * FUNCTION	: closeClientSocket
 *
 * PURPOSE	: Closes the client socket
 *
 * ARGS		: client info structure
 *
 * RETURN	:
 *
 */
static void 
closeClientSocket(clientinfo_t *ci)
{
	int i;
	int new_maxsd = 0;
	int tbclosedsd = ci->sd;

	if (ci->pid == rmtdpid) {
		rmtdpid = 0;
	}
	FD_CLR(ci->sd, &all_sds);
	(void) close(ci->sd);
	ci->sd = CLIENT_SLOT_AVAILABLE;
	
	TRACE("closeClientSocket:sd = %d maxsd = %d\n", tbclosedsd, maxsd);
	/*
	 * If maxsd is notify socket don't compute new maxsd
	 */	
	if (maxsd == notify_sd)  {
		return;
	}
	/*
	 * Compute new maxsd
	 */
	maxci = getMaxClientIndex();	
	for (i = maxci; i >= 0 ; i--) {
		if (clients[i].sd > new_maxsd)
			new_maxsd = clients[i].sd;
	}
	/* 
	 * If new maxsd = 0 that means there is no client currently
	 * connects to evlogd, and the to be closed sd = maxsd then
	 * maxsd shoulde be maxsd-1
	 */
	if (new_maxsd == 0 && tbclosedsd == maxsd) {
		maxsd--;
	} else if (new_maxsd != 0) {
		if (new_maxsd < notify_sd)
            maxsd = notify_sd;
        else
            maxsd = new_maxsd;
	}
	
	TRACE("Done closeClientSocket: maxsd = %d\n", maxsd);
	return;
}
#ifdef _PPC_64KERN_32USER_
struct ppc64kern_log_entry {
	unsigned int            log_magic;
	posix_log_recid_t       log_recid;
	long long               log_size;
	int                     log_format;
	int                     log_event_type;
	posix_log_facility_t    log_facility;
	posix_log_severity_t    log_severity;
	uid_t                   log_uid;
	gid_t                   log_gid;
	pid_t                   log_pid;
	pid_t                   log_pgrp;
	unsigned int            log_flags;
	unsigned long long       log_thread;
	posix_log_procid_t log_processor;
	long long		log_time_tv_sec;
	long long		log_time_tv_nsec;
};
#define REC_HDR_PPC64_SIZE sizeof(struct ppc64kern_log_entry)
#endif


/* 
 * FUNCTION	:  processKernelEvents()
 * 
 * PURPOSE	: 
 * 
 * ARGS		:
 * 
 * RETURN	:
 * 
 */
static void 
processKernelEvents()
{
	int bytes_read;      /* Bytes read for the Kernel buffer */
	int j, ret, retry=0;
	uint rlen;
	char *varbuf;
	log_info_t *linfo;
	struct posix_log_entry *rhdr;
	char buf[MAX_RECSIZE];
	char *bufp;
	int sd = 0;
	struct sockaddr_un evlsock;
	size_t sock_len;
	struct sysinfo info;
	struct timezone tz;
	int rtc_is_local = 0;
	struct posix_log_entry recheader;
	size_t rechdr_size = 0;
	
	/*
	 * get tz_minuteswest, we use this value to adjust the time stamp
	 * back to utc if it is flagged as local time.
	 */
	gettimeofday(NULL, &tz);
	/* 
	 * Get the system up time so we can fill in kernel event that 
	 * has zero as the time (when system initially starts up)
	 */ 
	sysinfo(&info);

	/* find out if the hwclock is local time */
	rtc_is_local = isRTC_Local();	
	
	if ((sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0){
		(void)fprintf(stderr, "Cannot create socket.\n");
		return;
	}
	memset(&evlsock, 0, sizeof(struct sockaddr_un));
	evlsock.sun_family = PF_UNIX;
	(void)strcpy(evlsock.sun_path, EVLOGD_EVTSERVER_SOCKET);
	sock_len = sizeof(evlsock.sun_family) + strlen(evlsock.sun_path);

	while (retry < 5) {
		if (connect(sd, (struct sockaddr *)&evlsock, sock_len) < 0) {
			if (retry > 5) {
				(void)fprintf(stderr, "Cannot connect to daemon.\n");
				return;
			}
			
			retry ++;
			sleep(1);
			TRACE("kernel thread cannot connect, retry=%d.\n", retry);
		}
		break;
	}
	
	for (;;) {
		TRACE("Wait on ksyslog....\n");	
		bytes_read = ksyslog(20, buf, MAX_RECSIZE);
		if (bytes_read < 0) {
			TRACE("ksyslog returns <0, breaking up\n");
			break;
		}
		if (bytes_read == 0) {
			TRACE("ksyslog returns 0\n");
			continue;
		}
		TRACE("Got some event(s) from kernel\n");
		j = bytes_read;
		bufp = buf;
		while (j>0) {
			TRACE("Prepare to write to evlogd...\n");
			getRecHeader(bufp, &recheader, &rechdr_size);
			rlen = recheader.log_size + rechdr_size ;
			if (j < rlen) {
				/*
				 * Found a partial record. 
				 */
				TRACE("Got partial record\n");
				break;
			}
			else {
				rhdr = &recheader;
				varbuf = (char *)(bufp + rechdr_size);

				/* Adjust timestamp to local time for kernel event */
				if (rhdr->log_flags & EVL_INITIAL_BOOT_EVENT) {
					int gotTimeStamp = 0;
#ifdef _POSIX_TIMERS_1
					if (clock_gettime(CLOCK_REALTIME, &(rhdr->log_time)) == 0) {
						gotTimeStamp = 1;
						rhdr->log_time.tv_sec -= info.uptime;
					}
#endif
					if (!gotTimeStamp) {
						/* time stamp this with system startup time */
						rhdr->log_time.tv_sec = (long) time(0) - info.uptime;
						rhdr->log_time.tv_nsec = 0;
					} 
				}
				else if ((rhdr->log_flags & EVL_KERNTIME_LOCAL) && (rtc_is_local == 1)) {
					rhdr->log_time.tv_sec = rhdr->log_time.tv_sec + (tz.tz_minuteswest * 60) ;
				}
				/* mask off the time relate flags - don't need them anymore*/
				rhdr->log_flags &= 	~(EVL_INITIAL_BOOT_EVENT | EVL_KERNTIME_LOCAL);

				if ((rhdr->log_flags & EVL_EVTYCRC) &&
		    		    (rhdr->log_format == POSIX_LOG_STRING
		    		    || rhdr->log_format == POSIX_LOG_PRINTF)) {
					/* event type = CRC of msg/fmt string */
					rhdr->log_event_type =
						evl_gen_event_type_v2(varbuf);
				}
				
				TRACE("Writing a kernel event\n");
				
				if ((ret = writeKernEvt(sd, rhdr, varbuf)) == EIO) {
					close(sd);
					return;
				}
				
				bufp += rlen ;
				j -= rlen;
			}
		} /* end while */
	} /* end for(;;) */
}


#ifdef LOGREC_KVERSION
/*
 * The record header we have from the kernel may be in the form of a
 * struct kern_log_entry.  If so, copy the data from it into entry
 * and return 0.  Otherwise, return -1.
 *
 * Currently, version 1 of kern_log_entry is the only one we understand.
 */
static int
copyKernelHeader(const char *buf, struct posix_log_entry *entry,
				 size_t *hdr_size)
{
	/* All versions of struct kern_log_entry start with this info: */
	struct {
		__u16	log_kmagic;
		__u16	log_kversion;
	} k;
	memcpy(&k, buf, sizeof(k));
	if (k.log_kmagic != LOGREC_KMAGIC) {
		return -1;
	}

	switch (k.log_kversion) {
	case 1:
	    {
		struct kern_log_entry_v1 kentry;
		memcpy(&kentry, buf, sizeof(kentry));
		entry->log_magic = 0;
		entry->log_recid = 0;
		entry->log_size = (size_t) kentry.log_size;
		entry->log_format = kentry.log_format;
		entry->log_event_type = kentry.log_event_type;
		entry->log_facility =
			(posix_log_facility_t) kentry.log_facility;
		entry->log_severity =
			(posix_log_severity_t) kentry.log_severity;
		entry->log_uid = kentry.log_uid;
		entry->log_gid = kentry.log_gid;
		entry->log_pid = kentry.log_pid;
		entry->log_pgrp = kentry.log_pgrp;
		entry->log_time.tv_sec = kentry.log_time_sec;
		entry->log_time.tv_nsec = kentry.log_time_nsec;
		entry->log_flags = kentry.log_flags;
		entry->log_thread = 0;
		entry->log_processor =
			(posix_log_procid_t) kentry.log_processor;
		*hdr_size = sizeof(kentry);
		break;
	    }
	case 2:
	    {
		struct kern_log_entry_v2 kentry;
		memcpy(&kentry, buf, sizeof(kentry));
		entry->log_magic = 0;
		entry->log_recid = 0;
		entry->log_size = (size_t) kentry.log_size;
		entry->log_format = kentry.log_format;
		entry->log_event_type = kentry.log_event_type;
		entry->log_facility =
			(posix_log_facility_t) kentry.log_facility;
		entry->log_severity =
			(posix_log_severity_t) kentry.log_severity;
		entry->log_uid = kentry.log_uid;
		entry->log_gid = kentry.log_gid;
		entry->log_pid = kentry.log_pid;
		entry->log_pgrp = kentry.log_pgrp;
		entry->log_time.tv_sec = kentry.log_time_sec;
		entry->log_time.tv_nsec = kentry.log_time_nsec;
		entry->log_flags = kentry.log_flags;
		entry->log_thread = 0;
		entry->log_processor =
			(posix_log_procid_t) kentry.log_processor;
		*hdr_size = sizeof(kentry);
	    }
	case 3:
	    {
		struct kern_log_entry_v3 kentry;
		posix_log_facility_t facility;

		memcpy(&kentry, buf, sizeof(kentry));
		entry->log_magic = 0;
		entry->log_recid = 0;
		entry->log_size = (size_t) kentry.log_size;
		entry->log_format = kentry.log_format;
		entry->log_event_type = kentry.log_event_type;
		entry->log_severity =
			(posix_log_severity_t) kentry.log_severity;
		entry->log_uid = kentry.log_uid;
		entry->log_gid = kentry.log_gid;
		entry->log_pid = kentry.log_pid;
		entry->log_pgrp = kentry.log_pgrp;
		entry->log_time.tv_sec = kentry.log_time_sec;
		entry->log_time.tv_nsec = kentry.log_time_nsec;
		entry->log_flags = kentry.log_flags;
		entry->log_thread = 0;
		entry->log_processor =
			(posix_log_procid_t) kentry.log_processor;

		/* Convert facility name to facility code. */
		if (posix_log_strtofac(kentry.log_facility, &facility) != 0) {
			/* Facility not registered.  Figure code=CRC of name. */
			if (evl_gen_facility_code(kentry.log_facility,
			    &facility) != 0) {
				facility = LOG_KERN;
			}
		}
		entry->log_facility = facility;
		*hdr_size = sizeof(kentry);
		break;
	    }
	default:
		return -1;
	}
	return 0;
}
#endif	/* LOGREC_KVERSION */

/*
 * Copy the data from the kernel event's record header (pointed to by buf)
 * to entry.  Set *hdr_size to the size of the kernel's header.
 */
void
getRecHeader(const char *buf, struct posix_log_entry *entry, size_t *hdr_size)
{
#ifdef LOGREC_KVERSION
	if (copyKernelHeader(buf, entry, hdr_size) == 0) {
		return;
	}
	/*
	 * The kernel header was not in the form of a kern_log_entry --
	 * or at least not one whose format we understand.  Try to
	 * extract the info using older techniques.
	 */
	/* FALLTHROUGH */
#endif
#ifdef _PPC_64KERN_32USER_
	{
		struct ppc64kern_log_entry ppc64_entry;
		memcpy(&ppc64_entry, buf, sizeof(struct ppc64kern_log_entry)); 

		entry->log_magic = ppc64_entry.log_magic;
		entry->log_recid = ppc64_entry.log_recid;
		entry->log_size = (size_t) ppc64_entry.log_size;
		entry->log_format = ppc64_entry.log_format;
		entry->log_event_type = ppc64_entry.log_event_type;
		entry->log_facility = ppc64_entry.log_facility;
		entry->log_severity = ppc64_entry.log_severity;
		entry->log_uid = ppc64_entry.log_uid;
		entry->log_gid = ppc64_entry.log_gid;
		entry->log_pid = ppc64_entry.log_pid;
		entry->log_pgrp = ppc64_entry.log_pgrp;
		entry->log_time.tv_sec = (long) ppc64_entry.log_time_tv_sec;
		entry->log_time.tv_nsec = (long) ppc64_entry.log_time_tv_nsec;
		entry->log_flags = ppc64_entry.log_flags;
		entry->log_thread = (unsigned long) ppc64_entry.log_thread;
		entry->log_processor = ppc64_entry.log_processor;
		*hdr_size = sizeof(struct ppc64kern_log_entry);
	}
#else
	memcpy(entry, buf, sizeof(struct posix_log_entry));
	*hdr_size = sizeof(struct posix_log_entry);
#endif
}
/* 
 * FUNCTION	:  writeKernEvt()
 * 
 * PURPOSE	: write a kernel event to the log daemon 
 * 
 * ARGS		:
 * 
 * RETURN	: return 0 if succeeded otherwise EIO
 * 
 */
int 
writeKernEvt(int sd, struct posix_log_entry *entry, const char *buf)
{
	enum parse_state_enum {
		PARSING_TEXT,
		PARSING_SYMSTART,    /* at < */
		PARSING_SYMBOL,       
		PARSING_SYMEND       /* at ] */
	};  

	int n, ret=0;
	char writebuf[POSIX_LOG_ENTRY_MAXLEN];
	unsigned char c;
	sigset_t oldset;
	int sigsBlocked;
	unsigned long value;
	auto struct symbol sym;
	auto char * symbol;
	char * ptr = (char *)buf;
	char * line = writebuf;
	static enum parse_state_enum parse_state = PARSING_TEXT;
	static char *sym_start;  /* points at the '<' of a symbol */
	int len;
	int oldlog_size;
	int expanded = 0;


	/* Mask all signals so we don't get interrupted */
	sigsBlocked = (_evlBlockSignals(&oldset) == 0);

#ifdef POSIX_LOG_TRUNCATE
	if (entry->log_format == POSIX_LOG_STRING
	    && (entry->log_flags & POSIX_LOG_TRUNCATE) != 0) {

		/* First write the header */
		if ((n = write(sd, entry, REC_HDR_SIZE)) != REC_HDR_SIZE) {
			/* socket is broken */
			fprintf(stderr, "Failed to write the msg header to evlog daemon.\n");
			ret = EIO;
			goto err_exit;
		}
		/*
		 * buf contains a string that was truncated to
		 * POSIX_LOG_ENTRY_MAXLEN bytes.  Stick a null character at the
		 * end of our copy of the buffer to make a null-terminated
		 * string. We don't try to resolve kernel address to symbol
		 * because there will be always no extra space here.
		 */
		bcopy((void *)buf, (void *)writebuf, entry->log_size);
		/* then write the variable message body */
		writebuf[POSIX_LOG_ENTRY_MAXLEN - 1] = '\0';
		if ((n = write(sd, writebuf, entry->log_size)) != entry->log_size) {
			/* socket is broken */
			fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
			ret = EIO;
			goto err_exit;
		}
		/*
		 * We don't try to resolve kernel addresses to symbols because
		 * there will be always no extra space here.
		 */
		goto done_exit;
	}
#endif

	/*
	 * Kernel symbols show up in the input buffer as : "[<aaaaaa>]",
	 * where "aaaaaa" is the address.  These are replaced with
	 * "[symbolname+offset/size]" in the output line - symbolname,
	 * offset, and size come from the kernel symbol table.
	 *
	 * If a message is longer than POSIX_LOG_ENTRY_MAXLEN after we reslove
	 * the kernel address to symbol, the symbol will not be expanded.
	 * (This should never happen, since the kernel should never generate
	 * messages that long.
	 *
	 * To preserve the original addresses, lines containing kernel symbols
	 * are output twice, if symbol_twice == 1. Once with the symbols
	 * converted and again with the original text. Just in case somebody
	 * wants to run their own Oops analysis on the evlog, e.g. ksymoops.
	 */
	if ( !lookup_symbol || (entry->log_format != POSIX_LOG_STRING) )
		goto write_without_resolve;

	TRACE ("Try resolving the kernel addresses to symbols\n");
	while ( *ptr && (ptr <= buf + entry->log_size) ) {
		switch ( parse_state )
			{
			case PARSING_TEXT:
				if ( *ptr == '[' )
					parse_state = PARSING_SYMSTART;
				break;
			case PARSING_SYMSTART:
				if ( *ptr == '<' ) {
					parse_state = PARSING_SYMBOL;
					sym_start = line;
				} else
					parse_state = PARSING_TEXT;
				break;
			case PARSING_SYMBOL:
				if ( *ptr == '>' )
					parse_state = PARSING_SYMEND;
				else if ( *ptr < '0' ||
						  *ptr > 'f' ||
						  ((*ptr > '9') && (*ptr < 'A')) ||
						  ((*ptr > 'F') && (*ptr < 'a'))
						  )
					parse_state = PARSING_TEXT;
				break;
			case PARSING_SYMEND:
				if ( *ptr != ']' ) {
					parse_state = PARSING_TEXT;
					break;
				}
				*(line - 1) = '\0';
				value = strtoul(sym_start + 1, NULL, 16);
				*(line - 1) = '>';  /* put the '>' back */
				if ( (symbol = LookupSymbol(value, &sym)) == NULL ) {
					TRACE("Cannot find symbol for address [<%lx>]\n", value);
					parse_state = PARSING_TEXT;
					break;
				}
				TRACE("Find symbol [%s] for address [<%lx>]\n", symbol, value);
				if ( entry->log_size + strlen (symbol) + 10 >
					 POSIX_LOG_ENTRY_MAXLEN ){
					/*
					 * 10 is the approximate max length of the extra
					 * space. If we don't have enough space for the
					 * symbol, we do not convert it.
					 */
					parse_state = PARSING_TEXT;
					break;
				}
				len = sprintf (sym_start, "%s+%d/%d",
							   symbol, sym.offset, sym.size);
				line = sym_start + len;
				expanded = 1;
				parse_state = PARSING_TEXT;
				break;
			default:  /* Can't get here! */
				parse_state = PARSING_TEXT;
			}
		*line++ = *ptr++;
	}
	*line = '\0';

	if ( !expanded )
		goto write_without_resolve;

	/* Now write the converted message */
	/* First write the header */
	oldlog_size = entry->log_size;
	entry->log_size = strlen (writebuf) + 1;
	if ((n = write(sd, entry, REC_HDR_SIZE)) != REC_HDR_SIZE) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg header to evlog daemon.\n");
		ret = EIO;
		goto err_exit;
	}
	if ((n = write(sd, writebuf, entry->log_size)) != entry->log_size) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
		ret = EIO;
		goto err_exit;
	}
	entry->log_size = oldlog_size;

	if ( !symbol_twice )
		goto done_exit;  /* no need to log the original addresses */

 write_without_resolve:
	/* First write the header */
	if ((n = write(sd, entry, REC_HDR_SIZE)) != REC_HDR_SIZE) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg header to evlog daemon.\n");
		ret = EIO;
		goto err_exit;
	}
	if (entry->log_size <= 0)
		goto done_exit;

	/* then write the variable message body */
	if ((n = write(sd, buf, entry->log_size)) != entry->log_size) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
		ret = EIO;
		goto err_exit;
	}

 done_exit:
	/* The daemon should tell the client that he finishes reading */ 
	read(sd, &c, sizeof(char));
	if(c != 0xac) {
		ret = EIO;
	}
 err_exit:
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	return ret;
}

/*
 * When receives signal SIGUSR1 or SIGUSR2, reload the symbol tables.
 */
void reload_daemon (int sig)
{
	if ( sig == SIGUSR2 ) {
		signal (SIGUSR2, reload_daemon);
		lookup_symbol = (InitKsyms(system_map) == 1);
	} else
		signal (SIGUSR1, reload_daemon);

	lookup_symbol |= InitMsyms();

	return;
}

/*
 * When the child process exits due to the kernel is not evlog-enable.
 * The parent process gets a SIGCHLD, and it should wait for the child 
 * exit status in order to avoid a zombie process on the process list
 */
static void sigChild_handler()
{
	int status;
	if (!waitForKids) {
		return;
	}
	waitpid(kpid, &status, WNOHANG);
}

static int
createQuery(char *qexp)
{
	char errbuf[80];

	/*
	 * If the query is already created in the previous read, destroy it
	 * before create the new query.
	 */
	if (dis_query.qu_tree != NULL) {
		if (posix_log_query_destroy(&dis_query) != 0) {
			fprintf(stderr, "evlogd: posix_log_query_destroy Failed\n"); 
			return -1;
		}
		dis_query.qu_tree = NULL;
	}
	if(!strcmp(qexp, "nofilter")) {
		return 0;
	}
	if (posix_log_query_create(qexp, POSIX_LOG_PRPS_NOTIFY, &dis_query,
							   errbuf, 80) != 0) {
		fprintf(stderr, "evlogd: posix_log_query_create: %s\n", errbuf);
		return -1;
	}

	return 0;
}

/*
 * Update evlogd config values. User send these values through 'evlconfig'
 * command.
 */
static int 
updateConfvalues(int confsd) 
{
	int ctype, value;
	int qexp_defined = 0;
	char cbuf[MAX_EVL_LINE];
	
	if (read(confsd, &ctype, sizeof(int)) < 0) {
		fprintf(stderr, "evlogd: Read call for configuration failed\n");
		return -1;
	}
	/*
	 * Read for different configuration elements until end of transmissin
	 * (EVENT_SCREEN + 1) is received. Hence, it supports for multiple
	 * elements in a single 'evlconfig' command.
	 */
	while (ctype <= EVENT_SCREEN) {
		if (read(confsd, &value, sizeof(int)) < 0) {
			fprintf(stderr, 
					"evlogd: Read call for configuration failed\n");
			return -1;
		}
		if (ctype >= DIS_DUPS) {
			if (value > sizeof(cbuf)) 
				return -1;
			if (read(confsd, &cbuf, value) < 0) {
				fprintf(stderr, 
						"evlogd: Read call for configuration failed\n");
				return -1;
			}
			cbuf[value] = '\0';
		}
		switch (ctype) {
		case DIS_COUNT:
			dup_count = value;
			if (dup_count == 0) {
				/*
				 * Get some maximum value to not to
				 * consider this Dup Count value in
				 * discarding records.
				 */
				dup_count = 10 * MAX_DUP_COUNT;
			}
			break;
		case DIS_INTERVAL:
			dup_interval = value;
			if (dup_interval == 0) {
				/*
				 * Get some maximum value to not to
				 * consider this Dup Interval value in
				 * discarding records.
				 */
				dup_interval = 24 * MAX_DUP_INTERVAL;
			}
			break;
		case DIS_DUPS:
			if (dis_dup_recs == FALSE && !strcmp(cbuf, "on")) {
				reallocBuffer(dup_array_size, FALSE, TRUE);
				dis_dup_recs = TRUE;
			} else if (dis_dup_recs == TRUE && !strcmp(cbuf, "off")) {
				reallocBuffer(dup_array_size, TRUE, FALSE);
				dis_dup_recs = FALSE;
			}
			break;
		case DIS_SIZE:
			if (dup_array_size != value) {
				/* Given the current implementation an infinitely
				 * sized array is absurd. Instead we'll just go to
				 * a default value.
				 */
				if (value == 0)
					value = DEFAULT_DUP_ARRAY_SIZE;
			
				reallocBuffer(value, dis_dup_recs, dis_dup_recs);
			}
			break;
		case EVENT_SCREEN:
			strcpy(query_exp, cbuf);
			qexp_defined = 1;
			break;
		}
		if (read(confsd, &ctype, sizeof(int)) < 0) {
			fprintf(stderr, 
					"evlogd: Read call for configuration failed\n");
			return -1;
		}
	}

	if (qexp_defined) {
		/*
		 * User requested to update screen filer. Hence, destroy the 
		 * previous query and creaty a new query.
		 */
		if (createQuery(query_exp) < 0) {
			return -1;
		}
	}

	return 0;
}


static int
createSocket(char *sockname, int *sd, socklen_t *sock_len, struct sockaddr_un *sa, 
			 mode_t mode, int backlog)
{

	if ((*sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "evlogd: cannot create socket for communication with event server.\n");
		perror("socket");
		return -1;
	}

	memset(sa, 0, sizeof(struct sockaddr_un));
	sa->sun_family = PF_UNIX;
	(void)strcpy(sa->sun_path, sockname);
	*sock_len = sizeof(sa->sun_family) + strlen(sa->sun_path);
	
	/*
	 * Bind the socket to the known socket address.
	 */
	if (bind(*sd, (struct sockaddr *) sa, *sock_len) < 0) {
		(void)fprintf(stderr, "evlogd: cannot bind to socket for communication with event server.\n");
		perror("bind");
		return -1;
	}

	(void)chmod(sockname, mode);
	/* Number of connections allowed in backlog */
	if (listen(*sd, backlog) < 0) {
		(void)fprintf(stderr, "evlogd: listening to socket connection failed.\n");
		perror("listen");
		return -1;
	}

	return 0;
}

static int
openMaybeCreate(const char *path, mode_t mode, log_info_t *linfo)
{
	struct stat st;
	int create_file = 0;
	log_header_t log_hdr;
	
	st.st_size = 0;
	if (stat(path, &st) || !st.st_size) {
		create_file = 1;
	}	
	if ((linfo->fd = open(path, O_RDWR|O_CREAT, mode)) < 0) {
		fprintf(stderr, "evlogd: Cannot open EVL Log file '%s': %s\n",
				path, strerror(errno));
		return -1;
	}
	
	(void)flock(linfo->fd, LOCK_EX);
	if (create_file) {
		memset(&log_hdr, 0, sizeof(log_header_t));
		log_hdr.log_magic = LOGFILE_MAGIC;
		log_hdr.log_version = 0;

		/* 
		 * Newly created.  Write the log header structure. 
		 */
		if (write(linfo->fd, &log_hdr, sizeof(log_header_t))
			!= sizeof(log_header_t)) {
			fprintf(stdout, 
					"evlogd: Cannot write log_header_t structure: %s",
					strerror(errno));
			return -1;
		}
	}

	linfo->map_hdr = (log_header_t *)mmap(NULL, sizeof(log_header_t),
										  PROT_WRITE|PROT_READ, MAP_SHARED, linfo->fd, 0);
	if (linfo->map_hdr == (log_header_t *)-1) {
		fprintf(stdout, "evlogd: mmap failed: %s", strerror(errno));
		return -1;
	}

	/*
	 * Seek to the end of the logfile for writing the event
	 * records.
	 */
	if (lseek(linfo->fd, 0, SEEK_END) == (off_t)-1) {
		mk_evl_rec(&evl_log,LOG_LOGMGMT, EVLOG_SEEK_LOG_FAILED, LOG_CRIT,
				   "evlogd: Cannot seek to the end of EVL log file: %s",
				   strerror(errno));
		return -1;
	}

	if (st.st_size > sizeof(log_header_t)) {
		/*
		 * Log file is already exists and it has atleast one record.
		 */ 
		int recsize;
		struct posix_log_entry rec_hdr;

		/*
		 * Read the record ID present in the last event record.
		 * This rec Id will be used if the daemon got killed last time 
		 * using signal SIGKILL. I,e. this record ID will be greater 
		 * than the one in the log header structure.
		 */ 
		if (lseek(linfo->fd, -sizeof(int), SEEK_CUR) == (off_t)-1) {
			mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_SEEK_LOG_FAILED, 
					   LOG_CRIT,"evlogd: Cannot seek in the EVL log file: %s",
					   strerror(errno));
			return -1;
		}
		if (read(linfo->fd, &recsize, sizeof(int)) != sizeof(int)) {
			mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_READ_LOG_FAILED, LOG_CRIT,
					   "evlogd: Cannot read from the EVL log file: %s",
					   strerror(errno));
			return -1;
		}
		recsize = recsize + sizeof(int);
		if (lseek(linfo->fd, -recsize, SEEK_CUR) == (off_t)-1) {
			mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_SEEK_LOG_FAILED, LOG_CRIT,
					   "evlogd: Cann't seek in the EVL log file: %s",
					   strerror(errno));
			return -1;
		}
		if (read(linfo->fd, &rec_hdr, sizeof(struct posix_log_entry)) !=
			sizeof(struct posix_log_entry)) {
			mk_evl_rec(&evl_log, LOG_LOGMGMT, EVLOG_READ_LOG_FAILED, LOG_CRIT,
					   "evlogd: Cann't read from the EVL log file: %s",
					   strerror(errno));
			return -1;
		}

		linfo->recId = linfo->map_hdr->last_recId;
		if (linfo->recId < rec_hdr.log_recid) {
			/*
			 * Record ID present in the log header is not the 
			 * correct one. Read the record ID from the last
			 * record in the log file.
			 */
			linfo->recId = rec_hdr.log_recid;
		}
	} 

	(void) flock(linfo->fd, LOCK_UN);
	return 0;
}

static int
valid_integer(const char *s, int *value)
{
	char *end = '\0';

	*value = (int)strtol(s, &end, 0);
	if (end == s) {
		fprintf(stderr, "evlogd: Invalid value in evlog.conf file\n");
		return -1;
	}

	return 0;
}

/*
 * Open and Read evlog.conf file to get config values before daemon started. 
 */
static int
readEvlogConfig()
{
	int conf_fd, ret, error = 0;
	FILE *conf_f;
	char line[MAX_EVL_LINE], query_exp[MAX_EVL_LINE];
	char *qexp = (char *)query_exp;
	char *s, *p, c;

	conf_fd = open(confPath, O_RDONLY);
	if (conf_fd < 0) {
		if (errno == ENOENT) {
			return 0;
		}
		perror("open");
		return -1;
	}
	conf_f = fdopen(conf_fd, "r");
	if (conf_f == NULL) {
		perror("fdopen");
		return -1;
	}
	*qexp = '\0';
	(void) lock_routine(conf_fd, F_SETLKW, F_RDLCK);
	while (fgets(line, MAX_EVL_LINE, conf_f)) {
		p = line + strspn(line, " \t");
		if ((p[0] == '\n') || (p[0] == '#')) {
			/* Empty line  or comment*/
			continue;
		}
		if ((s = strchr(p, ':')) == NULL) {
			fprintf(stderr,
					"evlogd: '%s' file got corrupted\n", confPath);
			return -1;
		}
		*s++ = '\0';
		if (!strncmp(p, "Discard Dups", 12)) {
			p = s + strspn(s, " \t");
			if (!strncmp(p, "on", 2)) {
				dis_dup_recs = 1;
			} else if (!strncmp(p, "off", 3)) {
				dis_dup_recs = 0;
			} else {
				fprintf(stderr,
						"evlogd: Invalid string for 'Discard Dups' in the evlog.conf file\n");
				error = -1;
				goto out;
			}
			continue;
		} else if (!strncmp(p, "Discard Interval", 16)) {
			if ((error = valid_integer(s, &dup_interval)) < 0) {
				goto out;
			}
			if (dup_interval < 0 || 
				dup_interval > MAX_DUP_INTERVAL) {
				fprintf(stderr, 
						"evlogd: Invalid Discard Interval.\n");
				goto out;
			}
			continue;
		} else if (!strncmp(p, "Discard Count", 13)) {
			if ((error = valid_integer(s, &dup_count)) < 0) {
				goto out;
			}
			if (dup_count < 0 || dup_count > MAX_DUP_COUNT) {
				fprintf(stderr, 
						"evlogd: Invalid Discard Count\n");
				goto out;
			}
			continue;
		} else if (!strncmp(p, "Lookback Size", 13)) {
			if ((error = valid_integer(s, &dup_array_size)) < 0) {
				goto out;
			}
			if (dup_array_size < 0 || dup_array_size > MAX_DUP_ARRAY_SIZE) {
				fprintf(stderr,
						"evlogd: Invalid Lookback Size\n");
				goto out;
			}
		} else if (!strncmp(p, "Event Screen", 12)) {
			p = s + strspn(s, " \t");
			while (*p != '\0') {
				if (*p == '\t') {
					*p++;
					continue;
				} else if (*p == '\n') {
					break;
				}
				*qexp++ = *p++;
			}
			*qexp = '\0';
			continue;
		} else if (!strncmp(p, "Console display level", 21)) {	
			if ((error = valid_integer(s, &console_sev_level)) < 0) {
				goto out;
			}
			TRACE("Console display level = %d\n", console_sev_level);
		}	
	}

	if (!dup_count && !dup_interval && dis_dup_recs) {
		fprintf(stderr, "evlogd: Invalid evlog.conf values\n");
		goto out;
	}
	/* 
	 * Get some maximum values if either dup_count or dup_interval values
	 * are zeros. Hence, these values will not be considered to drop
	 * records.
	 */
	if (dup_count == 0) {
		dup_count = 10 * MAX_DUP_COUNT;
	}
	if (dup_interval == 0) {
		dup_interval = 24 * MAX_DUP_INTERVAL;
	}

	/* With the current implementation, an infinitely sized duplicates array
	 * is impossible. Instead, if dup_array_size is zero, we'll just go to
	 * the default value.
	 */
	if (dup_array_size == 0)
		dup_array_size = DEFAULT_DUP_ARRAY_SIZE;

	if (console_sev_level == CONSOLE_OUTPUT_DISABLE) {
		if (ksyslog(6, NULL, 0) != 0) {
			fprintf(stderr, 
					"evlogd: Fail to disable console display\n");
		}	
	} else {
		if (ksyslog(8, NULL, console_sev_level) != 0) {
			fprintf(stderr, 
					"evlogd: Fail to set console display level\n");
		}	
	}
	 
	/*
	 * If screen filter present in the evlog.conf file, create the query.
	 */
	if (query_exp[0] != '\0') {
		if (createQuery(query_exp) < 0) {
			error = -1;
		}
	} 

 out:
	(void) lock_routine(conf_fd, F_SETLKW, F_UNLCK);
	fclose(conf_f);	/* closes conf_fd */
	return error;
}
			
	
/*
 * Compare two event records.
 * Return 0 if the records are not matched; 1 for otherwise.
 */
static int
compareEventRecords(char *crec,  /* Current Record */
					char *prec	 /* Previous Record */)
{
	int pos;
	char *pdata, *cdata;
	struct posix_log_entry *phdr = (struct posix_log_entry *)prec;
	struct posix_log_entry *chdr = (struct posix_log_entry *)crec;

	if (prec == (void *)NULL) {
		/*
		 * Can not find previous record to match.
		 */
		return 0;
	}
	if (phdr->log_size != chdr->log_size) {
		return 0;
	}
	if (phdr->log_event_type != chdr->log_event_type) {
		return 0;
	}
	if (phdr->log_facility != chdr->log_facility) {
		return 0;
	}
	if (phdr->log_severity != chdr->log_severity) {
		return 0;
	}	
	if (phdr->log_pid  != chdr->log_pid) {
		return 0;
	}
	if (phdr->log_uid != chdr->log_uid) {
		return 0;
	}
	if (phdr->log_gid != chdr->log_gid) {
		return 0;
	}
	if (phdr->log_pgrp != chdr->log_pgrp) {
		return 0;
	}
	if (phdr->log_format != chdr->log_format) {
		return 0;
	}
	if (phdr->log_flags != chdr->log_flags) {
		return 0;
	}
	if (phdr->log_thread != chdr->log_thread) {
		return 0;
	}
	if (phdr->log_processor != chdr->log_processor) {
		return 0;
	}
	/* 
	 * Both record headers are same. Now compare variable data.
	 */
	cdata = (char *)(crec + REC_HDR_SIZE);
	pdata = (char *)(prec + REC_HDR_SIZE);
	if (memcmp(cdata, pdata, chdr->log_size) != 0)
		return 0;

	return 1;
}
	

static int
evlogdstopdaemon()
{
	/* 
	 * Kill child process - in this case the process that is handling
	 * the kernel events, if child does not exit yet.
	 */ 
	TRACE("sharedData[0]=%d\n", sharedData[0]);
	if (kpid > 0 && *sharedData != 0x0e) {
		TRACE("Kill child process.\n");
		kill(kpid, 9);	
	}
	/* Unload all backends */
	be_cleanup();
	if (defaultPosixLog) {
		/*
		 * Dump any extra duplicates and update record ID in the
		 * log_header_t structure for both log files.
		 */

		dumpDuplicatesBuffer(&evl_log);
		dumpDuplicatesBuffer(&pvt_log);

		evl_log.map_hdr->last_recId = gRecId;
		(void) munmap(evl_log.map_hdr, sizeof(log_header_t));
		pvt_log.map_hdr->last_recId = gRecId;
		(void) munmap(pvt_log.map_hdr, sizeof(log_header_t));

		close(evl_log.fd);
		close(pvt_log.fd);
	}

	/* TIMING */
	printf("%f\n", perf_time.tv_sec + (perf_time.tv_usec / 1000000.0));
	return 0;
}

/*
 * FUNCTION: NewSIGAction
 *
 * PURPOSE: New signal handler
 *
 * CALLED BY: Any signal
 *
 * ARGUMENTS:
 *    None
 *
 * RETURN VALUES:
 *    None
 */
static void
NewSIGAction()
{
	(void) unlink(PidFile);
	(void)evlogdstopdaemon();
	exit(1);
}

/*
 * FUNCTION     : mk_evl_rec
 * USAGE        : Form an event record and directly write it to the log file.
 *		: This function will be called if evlogd failed (not writing
 *		: to the log file error).
 */
void
mk_evl_rec(log_info_t *linfo, posix_log_facility_t facility,
		   int event_type, posix_log_severity_t severity,
		   const char *format, ...)
{
	char evbuf[REC_HDR_SIZE + POSIX_LOG_ENTRY_MAXLEN];
	struct posix_log_entry *rentry = (struct posix_log_entry *)(char *)evbuf;
	int gotTimeStamp = 0, len;
	va_list args;

	va_start(args, format);
	len = vsnprintf((char *)(evbuf + REC_HDR_SIZE), POSIX_LOG_ENTRY_MAXLEN, format, args);
	va_end(args);

#ifdef _POSIX_TIMERS_1
	if (clock_gettime(CLOCK_REALTIME, &(entry.log_time)) == 0) {
		gotTimeStamp = 1;
	}
#endif
	if (!gotTimeStamp) {
		rentry->log_time.tv_sec = (long) time(0);
		rentry->log_time.tv_nsec = 0;
	}

	rentry->log_magic = LOG_MAGIC_ARCH;

	rentry->log_flags  	= 0;
	rentry->log_format 	= POSIX_LOG_STRING;
	rentry->log_size 	= len + 1;

	rentry->log_event_type 	= event_type;
	rentry->log_facility 	= facility;
	rentry->log_severity 	= severity;

	rentry->log_uid 	= geteuid();
	rentry->log_gid 	= getegid();
	rentry->log_pid 	= getpid();
	rentry->log_pgrp 	= getpgrp();
	rentry->log_processor  	= _evlGetProcId();

#ifdef _POSIX_THREADS
	rentry->log_thread 	= pthread_self();
#else
	rentry->log_thread 	= 0;
#endif
	
	log_evl_rec(linfo, evbuf);
}

/*
 * FUNCTION     : writeDiscardEvent
 * USAGE        : After the discarded interval expired, write an event about
 *              : how many duplicate records discarded in the eventlog file. 
 */
void
writeDiscardEvent(log_info_t *linfo, posix_log_facility_t facNum,
				  int eventType,	int count)
{
	char *fname, facname[POSIX_LOG_MEMSTR_MAXLEN];
	char event[POSIX_LOG_MEMSTR_MAXLEN + 255];

	fname = _evlGetFacilityName(facNum, facname);
	if (fname == NULL) {
		snprintf(event, sizeof(event),
				 "Discarded %d duplicate events, event_type = 0x%x, facility = %d",
				 count, eventType, facNum);
	} else {
		snprintf(event, sizeof(event), 
				 "Discarded %d duplicate events, event_type = 0x%x, facility = %s",
				 count, eventType, fname);
	}

	mk_evl_rec(linfo, LOG_LOGMGMT, 1, LOG_INFO, event);
}

/*
 * FUNCTION: dropDupRecord
 *
 * PURPOSE: Drops duplicate records if dis_dup_recs flag is on.
 *
 * CALLED BY: Process_Event
 *
 * ARGUMENTS:
 * 		rhdr 	- The incoming record.
 * 		rbuf 	- rhdr's entry in the lookback buffer.
 * 		linfo 	- The currently operating on log
 * 		buf_index	- rbuf's index in the lookback buffer.
 *
 * RETURN VALUES: 1 for drop record; 0 for either the flag is 'off' or
 * 		current record is a new one.
 */
static int
dropDupRecord(struct posix_log_entry *rhdr,
			  log_info_t *linfo,
			  int buf_index)
{
	/* equality_index is for replacing elements round-robin style
	 * in case all other replacement tests fail.
	 */
	static int equality_index = -1;

	int count;
	int equality_flag = 1;
	dup_buffer_t *replace_ptr;
	dup_buffer_t *dup_ptr;
	int replace_ptr_score;
	int dup_ptr_score;
	time_t cur_time;
	
	if (!dis_dup_recs)
		return 0;
	
	cur_time = time(0);
		
	/* Ensure the array for the past n logs has been created.
	 * If it hasn't, create it.
	 */
	if (linfo->dup_recs_array == NULL)
		linfo->dup_recs_array = calloc(dup_array_size,
									   sizeof(dup_buffer_t));

	/* Compare the current record against every record in the array. */
	dup_ptr = linfo->dup_recs_array;
	replace_ptr = dup_ptr;
	replace_ptr_score = compute_score(replace_ptr, cur_time);

	/* Loop through the array to see if there are any duplicates
	 * or if not, then to determine which element to replace. If every
	 * single element is the same then the equality_flag will remain
	 * true and after exiting the loop we will replace elements
	 * incrementally (round-robin).
	 */
	for (count = 0; count < dup_array_size; count++) {

		if (count > 0) {
			dup_ptr_score = compute_score(dup_ptr, cur_time);

			if (dup_ptr_score != replace_ptr_score) {
				equality_flag = 0;
				if (dup_ptr_score < replace_ptr_score) {
					replace_ptr = dup_ptr;
					replace_ptr_score = dup_ptr_score;
				}
			}
		}
			
		/* Call compareEventRecords to determine if we are duplicating.
		 * Of course, it's pointless to compare to a timed-out event.
		 */
		if (cur_time - dup_ptr->time_start < dup_timeout &&
			compareEventRecords((char *)rhdr, dup_ptr->rec)) {
			
			/* Increment dup_ptr->dup_count & check to see if we
			 * should write a discard event.
			 */
			dup_ptr->dup_count++;
			linfo->dup_count++;
			
			if (dup_ptr->dup_count == dup_count ||
			    cur_time - dup_ptr->time_start >= dup_interval ) {
				writeDiscardEvent(linfo, rhdr->log_facility,
								  rhdr->log_event_type,
								  dup_ptr->dup_count);
				dup_ptr->time_start = 0;
				dup_ptr->rec = NULL;
				buf[dup_ptr->buf_index].in_use = 0;
				linfo->dup_count -= dup_ptr->dup_count;
				dup_ptr->dup_count = 0;
			}

			return 1;
		}
		dup_ptr++;
	}
	
	/* Once we've determined who to replace, we write out the
	 * duplicate entry event (as needed) and then put the
	 * current event into the duplicates array.
	 *
	 * If all of the elements were of equal weighting, then the equality
	 * flag will be true and we'll just replace the next element.
	 */
	if (equality_flag) {
		replace_ptr = linfo->dup_recs_array + 
			((++equality_index >= dup_array_size) ?
			 equality_index = 0 : equality_index);
	}

	/* Write out a discard event if needed. */
	if (replace_ptr->dup_count > 0) {
		struct posix_log_entry *rechdr = 
			(struct posix_log_entry *)replace_ptr->rec;
		writeDiscardEvent(linfo, rechdr->log_facility,
						  rechdr->log_event_type,
						  replace_ptr->dup_count);
		linfo->dup_count -= replace_ptr->dup_count;
	}
		
	/* Clear old index position and set the new one. */
	if (replace_ptr->rec != NULL)
		buf[replace_ptr->buf_index].in_use = 0;
	buf[buf_index].in_use = 1;
	replace_ptr->buf_index = buf_index;

	replace_ptr->dup_count = 0;
	replace_ptr->time_start = cur_time;
	replace_ptr->rec = (char *)rhdr;

	return 0;
}


/* FUNCTION: 	compute_score
 *
 * PURPOSE:	Assigns a score to various time_start and dup_count
 * 		combinations. Lowest score wins.
 * 			
 * CALLED BY: 	dropDupRecord
 *
 * ARGUMENTS:
 * 		dup_ptr	- Element being scored.
 * 		cur_time - Current time
 *
 * RETURN VALUE:
 * 		computed score.
 */
static int
compute_score(dup_buffer_t *dup_ptr, time_t cur_time) {

	if (dup_ptr->rec == NULL) {
		/* Element is NULL */
		return 0;
	}

	if (dup_ptr->time_start != 0 &&
		cur_time - dup_ptr->time_start > dup_interval) {
		/* Element has exceeded dup_interval */
		return dup_ptr->time_start + dup_ptr->dup_count;
	}
	
	if (dup_ptr->time_start != 0) {
		/* Element is not new */
		return dup_ptr->time_start + dup_interval +
			dup_ptr->dup_count + dup_count;
	}
	
	/* Element is new */
	return 2 * dup_count + cur_time + dup_interval;
}

#ifdef EVLOG_REGISTER_FAC
/* rhdr and varbuf constitute an event record that we are about to log.
 * If it's a (LOG_LOGMGMT, EVLOG_REGISTER_FAC) record, then ensure that
 * the facility named in the record is registered in the facility registry.
 * We annotate the record to indicate the outcome of the request.
 */
static void
handleFacRegEvent(struct posix_log_entry *rhdr, char *varbuf)
{
	extern long evlCheckInterval;
	long oldInterval = evlCheckInterval;
	struct evl_facreg_rq *rq;
	char *facName;
	posix_log_facility_t codeFromRegistry;
	evl_facreg_rq_status_t success;
	int status;
	int oldWaitForKids;

	if (rhdr->log_facility != LOG_LOGMGMT
	    || rhdr->log_event_type != EVLOG_REGISTER_FAC) {
		/* The usual case */
		return;
	}

	if ((rhdr->log_flags & EVL_KERNEL_EVENT) == 0) {
		/* Don't honor requests from user mode. */
		return;
	}

	rq = (struct evl_facreg_rq*) varbuf;
	facName = varbuf + sizeof(struct evl_facreg_rq);
	rq->fr_registry_fac_code = EVL_INVALID_FACILITY;	/* for now */

	if (rq->fr_rq_status == frst_kernel_failed) {
		/* Beyond our help */
		rhdr->log_severity = LOG_ERR;
		return;
	}

	/* Make sure our view of the registry stays up-to-date. */
	evlCheckInterval = 0;

	success = frst_already_registered;
	status = posix_log_strtofac(facName, &codeFromRegistry);
	if (status == 0) {
		goto facIsInRegistry;
	}

	/* facName is not in the registry.  Add it. */
	/*
	 * Clear the waitForKids flag temporarily, since
	 * _evlAddFacilityToRegistry() forks, execs, and waits for evlfacility.
	 */
	oldWaitForKids = waitForKids;
	waitForKids = 0;
	status = _evlAddFacilityToRegistry(facName, EVL_FACC_KERN, NULL);
	waitForKids = oldWaitForKids;
	if (status != 0) {
		rq->fr_rq_status = frst_registration_failed;
		rhdr->log_severity = LOG_ERR;
		goto done;
	}

	success = frst_registered_ok;
	status = posix_log_strtofac(facName, &codeFromRegistry);
	if (status == 0) {
		goto facIsInRegistry;
	}

	/*
	 * We shouldn't reach here.  _evlAddFacilityToRegistry() was
	 * successful, which means that we successfully added the facility.
	 * Since evlCheckInterval = 0, the call to posix_log_strtofac()
	 * should have noticed that the registry file has changed, reread
	 * it, and found the new entry.
	 *
	 * No big deal, really.  We accept all events from the kernel,
	 * even if the facility code is unrecognized.  Here we'll just
	 * say we failed to register it.
	 */
	rq->fr_rq_status = frst_registration_failed;
	rhdr->log_severity = LOG_ERR;
	goto done;

 facIsInRegistry:
	/*
	 * The facility named facName is in the registry.  Does the code
	 * match what the kernel is using?
	 */
	rq->fr_registry_fac_code = codeFromRegistry;
	if (codeFromRegistry == rq->fr_kernel_fac_code) {
		rq->fr_rq_status = success;
		rhdr->log_severity = LOG_INFO;
	} else {
		rq->fr_rq_status = frst_faccode_mismatch;
		rhdr->log_severity = LOG_ERR;
	}
 done:
	evlCheckInterval = oldInterval;
	return;
}
#endif

/* The record header would looks like this for 32 bit app under x86_64 */
struct rec_hdr_32 {
	unsigned int            log_magic;
	posix_log_recid_t       log_recid;
	unsigned int		log_size;
	int                     log_format;
	int                     log_event_type;
	posix_log_facility_t    log_facility;
	posix_log_severity_t    log_severity;
	uid_t                   log_uid;
	gid_t                   log_gid;
	pid_t                   log_pid;
	pid_t                   log_pgrp;
	unsigned int 		log_time_tv_sec;
	unsigned int		log_time_tv_nsec;
	unsigned int            log_flags;
	unsigned int		log_thread;
	posix_log_procid_t log_processor;
};
/* Annotate record comes from 32 bit apps under x86_64 */ 
#define LOGREC_ARCH_X86_32	0xf000

static int
populateRecHdr(clientinfo_t *ci, struct posix_log_entry * rhdr)
{
	unsigned int daemon_arch = LOCAL_ARCH;
	unsigned int client_arch;
	struct rec_hdr_32 rhdr32;
	char *tmp;
	int log_magic_size = sizeof(unsigned int);
	int rec_hdr_32_size = sizeof(struct rec_hdr_32);
	int nbytes;

	nbytes = read (ci->sd, &client_arch, log_magic_size);
	if (nbytes != log_magic_size) {
		closeClientSocket(ci);
		return -1;
	}
	TRACE("client_arch = %x\n", client_arch);
	if (daemon_arch == LOGREC_ARCH_X86_64 && 
		client_arch == LOGREC_ARCH_I386) {
		/* 
		 * 32 bit app writes this record - the header size 
		 * is different (smaller)
		 */
		tmp = (char *) &rhdr32;
		tmp += log_magic_size;
		nbytes = read(ci->sd, tmp,
					  rec_hdr_32_size - log_magic_size);
		if (nbytes != rec_hdr_32_size - log_magic_size) {
			closeClientSocket(ci);
			return -1;
		}
		
		rhdr->log_magic = LOGREC_ARCH_X86_32; 
		rhdr->log_recid = 0;
		rhdr->log_size = (size_t) rhdr32.log_size;
		rhdr->log_format = rhdr32.log_format;
		rhdr->log_event_type = rhdr32.log_event_type;
		rhdr->log_facility = rhdr32.log_facility;
		rhdr->log_severity = rhdr32.log_severity;
		rhdr->log_uid = rhdr32.log_uid;
		rhdr->log_gid = rhdr32.log_gid;
		rhdr->log_pid = rhdr32.log_pid;
		rhdr->log_pgrp = rhdr32.log_pgrp;
		rhdr->log_time.tv_sec = (long) rhdr32.log_time_tv_sec;
		rhdr->log_time.tv_nsec = (long) rhdr32.log_time_tv_nsec;
		rhdr->log_thread = rhdr32.log_thread;
		rhdr->log_processor = (long) rhdr32.log_processor;
		

	} else if (daemon_arch == LOGREC_ARCH_I386 &&
			   client_arch == LOGREC_ARCH_X86_64) {
		/* Hmm, would some one wants this */
		closeClientSocket(ci);
		return -1;
	}
	else {
		TRACE("REC_HDR_SIZE=%d\n", REC_HDR_SIZE);	
		tmp = (char *) rhdr;
		tmp += log_magic_size;
		nbytes = read(ci->sd, tmp,
					  REC_HDR_SIZE - log_magic_size);
		if (nbytes != REC_HDR_SIZE - log_magic_size) {
			closeClientSocket(ci);
			return -1;
		}
		rhdr->log_magic = client_arch;
	}
	TRACE("Read %d bytes into rhdr, log_magic = %x, facility=%d.\n", nbytes, rhdr->log_magic, rhdr->log_facility);	
	return nbytes;
}
	
/*
 * FUNCTION: Process_Event
 *
 * PURPOSE: Process input data directly from the posix_log_write call. 
 *
 * CALLED BY: Main
 *
 * RETURN VALUES: 0 : Success, -1 : otherwise
 *    		 
 */
static int
Process_Event(clientinfo_t *ci, char *bufp, int buf_index)
{
	int nbytes;
	posix_log_query_t *query;
	int acc_flags;
	char *varbuf;
	log_info_t *linfo;
	struct posix_log_entry *rhdr;
 	struct ucred ucred;
 	socklen_t ucredsz = sizeof(struct ucred);
	
	rhdr = (struct posix_log_entry *)bufp;
	varbuf = (char *)(bufp + REC_HDR_SIZE);	
	/* read an event from the socket */
	TRACE("Got an event arrive on sd %d\n", ci->sd);
	/* first read the record header */
	if (populateRecHdr(ci, rhdr) == -1 ) {
		return -1;
	}
	/*
	 * This check make sure that the event is not from a malicious user,
	 * anything comes from the kernel thread is ok.
	 */
	if (ci->pid != kpid && ci->pid != rmtdpid) {
	
		if (ci->uid != rhdr->log_uid || ci->pid != rhdr->log_pid) {
			TRACE("bad user uid=%u, pid=%u, facility=%d\n", rhdr->log_uid, rhdr->log_pid, rhdr->log_facility);
			closeClientSocket(ci);
			return 0;
		}
		/*
		 * An event did not come from the kernel thread, but claimed that it is 
		 * a kernel event
		 */
		if (rhdr->log_flags & EVL_KERNEL_EVENT) {
			TRACE("Fake kernel event\n");
			closeClientSocket(ci);
			return 0;
		}
	}
	/* Don't set architecture if it is remote events */
	if (ci->pid != rmtdpid) {
		if (rhdr->log_magic == LOGREC_ARCH_X86_32) {
			/* 
 			 * This record appears to come from 32 bit app
			 * under x86_64 environment - 
			 */
			rhdr->log_magic = (LOGREC_MAGIC & 0xffff0000) | LOGREC_ARCH_I386;
		} else {
			rhdr->log_magic = LOG_MAGIC_ARCH;
			TRACE("local event - log_magic = %x\n", rhdr->log_magic);
		}
		if (rhdr->log_flags & EVL_KERNEL_EVENT) {
#if defined(_PPC_64KERN_32USER_)
			rhdr->log_magic =  (LOGREC_MAGIC & 0xffff0000) | 
				LOGREC_ARCH_PPC64;	
#endif
		} 
	} else {
		/* 
		 * If came from evlogrmtd, it either from remote host
		 * or evlogrmtd itself. If it comes from the remote host
		 * it should have the log_magic and arch properly
		 * setup. If log_magic is not set yet then just set it with
		 * local arch. 
		 */
		TRACE("Record from rmt host\n");
		if ((rhdr->log_magic & 0xffff0000) ==
			(LOGREC_MAGIC & 0xffff0000)) {
			/* nop */
			TRACE("rmt event - log_magic = %x\n", rhdr->log_magic);
		} else {	
			rhdr->log_magic = LOG_MAGIC_ARCH;
		}
	}

	if (rhdr->log_size <= 0) {
		varbuf = NULL;	
	}
	else {
		if((nbytes = _evlReadEx(ci->sd, varbuf, rhdr->log_size)) < 0) {
			closeClientSocket(ci);
			return -1;
		}
		TRACE("Read %d bytes into rec_buf.\n", nbytes);	
	}
	/* 
	 * Just write one byte back to the client to let him know that
	 * we are done reading. 
	 */ 
	{
		unsigned char c=0xac;
		write(ci->sd, &c, sizeof(char));
	}
	

	/* Default posix log */
	if (defaultPosixLog) {
		if ((dis_query.qu_tree != NULL) && 
			_evlEvaluateQuery(&dis_query, rhdr, varbuf)) {
			return 0;
		}
		
		query = _evlGetFacilityAccessQuery(rhdr->log_facility, &acc_flags);
		if ((acc_flags & EVL_FACC_KERN) && !(rhdr->log_flags & EVL_KERNEL_EVENT)) {
			/*
			 * Kernel facility, but it does not have EVL_KERNEL_EVENT set.
			 * - don't log the event
			 */
			TRACE("Kernel facility, but it does not have EVL_KERNEL_EVENT set.\n");
			return 0;
		}				
		if ((query != NULL)  &&
			(_evlEvaluateQuery(query, rhdr, varbuf) != 1)) {
			/* not match the restricted query - don't log the event */
			return 0;
		}
	}
#ifdef EVLOG_REGISTER_FAC
	handleFacRegEvent(rhdr, varbuf);
#endif
	if (defaultPosixLog) {
		/*
		 * If the access for the facility is defined as private 
		 * in the registry, This event has to be in the 
		 * privatelog. Otherwise, the event has to be in the
		 * eventlog even the facility is not present in the 
		 * registry. The above function returns -1 if the 
		 * facility is not present.  Since the user level 
		 * events will be checked for valid facility before
		 * write in to the kernel buffer, this problem arises 
		 * only for kernel level facilities. Need to modify 
		 * this code to return here if decided not to log
		 * an event which has an invalid facility.
		 */
		if (acc_flags != -1 && acc_flags & EVL_FACC_PRIVATE) {
			linfo = &pvt_log;
		} else {
			linfo = &evl_log;
		}
		if (!dropDupRecord(rhdr, linfo, buf_index)) {
			if (log_evl_rec(linfo, bufp) < 0) {
				return -1;
			}
			TRACE("Write an event to the log, log_size=%d\n", 
				  rhdr->log_size);		
		}
	}
	/* Event is logged now - excecute all backends */
	be_run((char *) rhdr, NULL, evl_callback);
	return 0;
}


int 
log_dropped_evtcnt_msg(log_info_t *linfo, int numevt_dropped)
{
	char evbuf[REC_HDR_SIZE + POSIX_LOG_ENTRY_MAXLEN + sizeof(evlrecsize_t)];
	struct posix_log_entry *rentry = (struct posix_log_entry *)(char *)evbuf;
	int gotTimeStamp = 0, len;
	uint recsize;
	
	len = snprintf((char*)(evbuf + REC_HDR_SIZE), POSIX_LOG_ENTRY_MAXLEN, 
				   "%d events dropped due to secondary user buffer overflow.\n", numevt_dropped);
	
#ifdef _POSIX_TIMERS_1
	if (clock_gettime(CLOCK_REALTIME, &(entry.log_time)) == 0) {
		gotTimeStamp = 1;
	}
#endif
	if (!gotTimeStamp) {
		rentry->log_time.tv_sec = (long) time(0);
		rentry->log_time.tv_nsec = 0;
	}

	rentry->log_flags  	= 0;
	rentry->log_format 	= POSIX_LOG_STRING;
	rentry->log_size 	= len + 1;

	rentry->log_event_type 	= 1;
	rentry->log_facility 	= LOG_LOGMGMT;
	rentry->log_severity 	= LOG_INFO;

	rentry->log_uid 	= geteuid();
	rentry->log_gid 	= getegid();
	rentry->log_pid 	= getpid();
	rentry->log_pgrp 	= getpgrp();
	rentry->log_processor  	= _evlGetProcId();

#ifdef _POSIX_THREADS
	rentry->log_thread 	= pthread_self();
#else
	rentry->log_thread 	= 0;
#endif
	
	rentry->log_magic = LOG_MAGIC_ARCH;
	rentry->log_recid = getNextRecId(linfo->recId);
	recsize = rentry->log_size + REC_HDR_SIZE;	

	memcpy((void *)(evbuf + recsize), &recsize , sizeof(evlrecsize_t));
	
	if (lseek(linfo->fd, 0, SEEK_END) == (off_t)-1) {
		fprintf(stderr,
				"evlogd: Cannot seek in the EVL logfile\n");
		return(-1);
	}
		
	if (write(linfo->fd, (void *)evbuf, recsize + sizeof(evlrecsize_t)) != (recsize + sizeof(evlrecsize_t))) {
		fprintf(stdout,
				"evlogd: Cannot write an EVL record to the log file: %s\n",
				strerror(errno));
		return(-1);
	}

	writeEvtToNfyDaemon(evbuf);
	return 0;
}

/*
 * FUNCTION	:  writeEvtToNfyDaemon
 * ARGUMENTS	:  
 *				:  evlbuf  - Pointer to the buffer to be logged.
 * 				
 * RETURN	:  
 *			:  
 * USAGE	:  This function writes rec to notify daemon
 */

static void 
writeEvtToNfyDaemon(char * evlbuf)
{
	struct posix_log_entry *rentry = (struct posix_log_entry *)(char *)evlbuf;

	if (notify_sd <= 0)
		return;		/* Notification socket is not openned - return */
		
	if (writeEx(notify_sd, evlbuf,REC_HDR_SIZE) 
		<= 0)  {
		fprintf(stdout, "Fail to write rec header to notification socket. "
						"Connection maybe broken.\n");
		notify_sd = rmSocket(notify_sd);
		return;
	}
	if (rentry->log_size <= 0)
		return;	/* nothing to write - return */
		
	if (writeEx(notify_sd, evlbuf + REC_HDR_SIZE,
				rentry->log_size) <= 0) {
		fprintf(stdout, "Fail to write record to notification socket. "
						"Connection maybe broken.\n");
		/* 
		 * Notification daemon may already exit - reset
		 * notification socket descriptor
		 */
		notify_sd = rmSocket(notify_sd);
	}	
}	 
/*
 * FUNCTION	:  write_rec_to_buf
 * ARGUMENTS	:  
 *				:  rbuf  - Pointer to the buffer to be logged.
 * 				:  rlen  - Buffer length.
 * RETURN	:  0 - Success
 *			:  -1  failed.
 * USAGE	:  This function writes log rec to memory.
 */
int
write_rec_to_buf(log_info_t *linfo, char *rbuf, size_t rlen)
{
	userbuf_info_t * tmpPtr_evlbuf;
	size_t bufsize;
	if (linfo->fd == user_evlbuf.fd) {
		tmpPtr_evlbuf = &user_evlbuf;
		bufsize =  MAX_USER_EVLBUF;
	} else if ( linfo->fd == userpriv_evlbuf.fd) {
		tmpPtr_evlbuf = &userpriv_evlbuf;
		bufsize = MAX_USERPRIV_EVLBUF;
	} else {
		/* should never happen */
		return -1;
	}
	if (tmpPtr_evlbuf->buf == NULL) {
		
		tmpPtr_evlbuf->buf = malloc(bufsize);
		if (!tmpPtr_evlbuf->buf) {
			perror("malloc failed.\n");
			return -1;
		}
		tmpPtr_evlbuf->watermark = tmpPtr_evlbuf->buf;
		tmpPtr_evlbuf->dropped_evt = 0;
	}
	if ((tmpPtr_evlbuf->watermark + rlen) <= (tmpPtr_evlbuf->buf + bufsize)) {
		/* still fit in the buffer */
		memcpy(tmpPtr_evlbuf->watermark, rbuf, rlen);
		tmpPtr_evlbuf->watermark += rlen;
		TRACE("Total bytes used in buf = %d\n", tmpPtr_evlbuf->watermark - tmpPtr_evlbuf->buf); 
	} else {
		/* keep track of how many dropped events */
		tmpPtr_evlbuf->dropped_evt++;
		TRACE("Dropped %d events\n", tmpPtr_evlbuf->dropped_evt);
	}
	return 0;
}

/*
 * Drain all events in the buffer to log file. 
 *
 */
int
drain_buf_to_log(log_info_t *linfo)
{
	char * tmp;
	size_t rlen;
	int i = 0;
	userbuf_info_t * tmpPtr_evlbuf;
	
	if (linfo->fd == user_evlbuf.fd) {
		tmpPtr_evlbuf = &user_evlbuf;
	} else if ( linfo->fd == userpriv_evlbuf.fd) {
		tmpPtr_evlbuf = &userpriv_evlbuf;
	} else {
		/* should never happen */
		return -1;
	}
	assert(tmpPtr_evlbuf->buf != 0);
	
	TRACE("Drain buffer to log\n", i);
#if 0
	tmp = tmpPtr_evlbuf->buf;
	while (tmp < tmpPtr_evlbuf->watermark) {
		rlen = ((evl_buf_rec_t *)tmp)->rechdr.log_size + REC_HDR_SIZE + sizeof(evlrecsize_t);	
		if (write(linfo->fd, (void *)tmp, rlen) != rlen) {
			fprintf(stdout,
					"evlogd: Cannot write an EVL record to the log file: %s\n",
					strerror(errno));
			return(-1);
		}
		i++;
		tmp += rlen;	
	}
	TRACE("Drain %d records\n", i);
#endif

	if (lseek(linfo->fd, 0, SEEK_END) == (off_t)-1) {
		fprintf(stderr,
				"evlogd: Cannot seek in the EVL logfile\n");
		return(-1);
	}
	
	rlen = tmpPtr_evlbuf->watermark - tmpPtr_evlbuf->buf;
	if (write(linfo->fd, (void *) tmpPtr_evlbuf->buf, rlen) != rlen) {
		fprintf(stdout,
				"evlogd: Cannot write an EVL records to the log file: %s\n",
				strerror(errno));
		return(-1);
	}

	/* 
	 * If there are any dropped messages - log an event 
	 * indicates the number of dropped messages.
	 */
	if (tmpPtr_evlbuf->dropped_evt > 0) {		
		log_dropped_evtcnt_msg(linfo, tmpPtr_evlbuf->dropped_evt);
	}
	/* buffer now is drained - free it */
	free(tmpPtr_evlbuf->buf);
	tmpPtr_evlbuf->buf = tmpPtr_evlbuf->watermark = NULL;	
	TRACE("free buf, set buf pointer to NULL.\n");
	return 0;
} 

/*
 * FUNCTION	:  write_rec_to_log
 * ARGUMENTS	:  linfo - EVL log or private log information. 
 *		:  rbuf  - Pointer to the buffer to be logged.
 * 		:  rlen  - Buffer length.
 * RETURN	:  0 - Success
 *		:  -1 - if lseek or write calls are failed.
 * USAGE	:  This function checks if the write offset is changed in the
 *		:  log file by the other process (Ex: Log management daemon).
 *	 	:  If so, it adjust to the correct position. Write this record
 *		:  at the correct write offset in the log.
 */
int
write_rec_to_log(log_info_t *linfo, char *rbuf, size_t rlen) 
{
	if (lock_routine(linfo->fd, F_SETLK, F_WRLCK) == -1 )
		{
			/* Fail to get the lock */
			TRACE("Failed to get the lock, write event to buffer instead.\n");
			//if (errno == EAGAIN)	
			write_rec_to_buf(linfo, rbuf, rlen);		
		
			return 0;
		}

	/* If the userbuf has event(s) then drain it */
	if (linfo->fd == user_evlbuf.fd && user_evlbuf.buf != NULL) {
		if (drain_buf_to_log(linfo) == -1) {
			return -1;
		}
	} 

	if (linfo->fd == userpriv_evlbuf.fd && userpriv_evlbuf.buf != NULL) {
		if (drain_buf_to_log(linfo) == -1) {
			return -1;
		}
	} 	

	if (lseek(linfo->fd, 0, SEEK_END) == (off_t)-1) {
		fprintf(stderr,
				"evlogd: Cannot seek in the EVL logfile\n");
		return(-1);
	}
	TRACE("Write event rec to log, rlen=%d\n", rlen);
	if (write(linfo->fd, (void *)rbuf, rlen) != rlen) {
		fprintf(stdout,
				"evlogd: Cannot write an EVL record to the log file: %s\n",
				strerror(errno));
		return(-1);
	}

	lock_routine(linfo->fd, F_SETLK, F_UNLCK);
	return 0;
}


/*
 * FUNCTION     : log_evl_rec
 * ARGUMENTS    : recbuf - Pointer to the buffer to be logged
 * RETURN       : 0 - Success
 *              : -1 - if write to the log failed.
 * USAGE        : Write the buffer to different log file depends on the
 *              : facility registry.
 *
 * Note: The REC_HDR_SIZE bytes of recbuf are the record's header, and we
 * set logrec = recbuf accordingly.  This code assumes that recbuf is aligned
 * such that references to logrec's members will be aligned.  Given that
 * recbuf is one of the elements of buf[], which is an array of long char
 * buffers, it's not clear that this will always be the case.
 */
static int 
log_evl_rec(log_info_t *linfo, char *recbuf)
{
	struct posix_log_entry *logrec = (struct posix_log_entry*) recbuf;
	char lbuf[REC_HDR_SIZE + POSIX_LOG_ENTRY_MAXLEN + sizeof(evlrecsize_t)];
	uint recsize = logrec->log_size + REC_HDR_SIZE;
	int facAccess; 
	struct timezone tz;
#if 0	
	/*
	 * get tz_minuteswest, we use this value to adjust the time stamp
	 * back to utc if it is flagged as local time.
	 */
	gettimeofday(NULL, &tz);
	/* Adjust timestamp */
	if (logrec->log_flags & EVL_KERNEL_EVENT) {
		logrec->log_time.tv_sec = logrec->log_time.tv_sec + (tz.tz_minuteswest * 60) ;
	}
#endif
	//	logrec->log_magic = LOG_MAGIC_ARCH;
	logrec->log_recid = getNextRecId(linfo->recId);
	memcpy((void *)lbuf, (void *)recbuf, recsize);
	memcpy((void *)(lbuf + recsize), &recsize , sizeof(evlrecsize_t));

	if (write_rec_to_log(linfo, lbuf, recsize + sizeof(evlrecsize_t)) < 0) {
		lock_routine(linfo->fd, F_SETLK, F_UNLCK);
		return -1;
	}
	if ((logrec->log_severity & (LOG_EMERG | LOG_CRIT | LOG_ALERT)) ||
		logrec->log_facility == LOG_AUTHPRIV) {
#ifdef _POSIX_SYNCHRONIZED_ID
		fdatasync(linfo->fd);
#endif
	}
	writeEvtToNfyDaemon(lbuf);	
	return 0;
}
	
static int
rmSocket(int soc)
{
	FD_CLR(soc, &all_sds);
	if (soc == maxsd) {
		maxsd--;
	}
	(void) close(soc);
	return 0;
}

int writeEx(int sd, void * buf, size_t len)
{
    int n = 0;
#ifdef _NONBLOCKING_SOCKET_
	int moreBytes = len;
	void * tmp;
	tmp = buf;
	while (moreBytes > 0) {
		n = write(sd, tmp, moreBytes);
		if (n < 0) {
			if(errno != EWOULDBLOCK) {
				fprintf(stdout, "write error on sd %d\n", sd);
				return -1;
			}
		} else {
			if (n == 0) {
				fprintf(stdout, "Socket sd %d closed\n", sd);
				return -1;
			}
			moreBytes -= n;
			tmp += n;
		}
	}

	return len - moreBytes;
#else
    if ((n = write(sd, buf, len)) != len) {
        return -1;
    }
    return n;
#endif
}

/* 
 * cmd is F_GETLK see if lock exists on a file descriptor fd,
 * or F_SETLK set a lock on file descriptor fd,
 * or F_SETLKW, the blocking version of F_SETLK.
 * Process sleeps until lock can be obtained. 
 * 
 */
int lock_routine(int fd, int cmd, int type)
{

	struct flock lock;
	lock.l_type=type;					/*F_RDLCK, F_WRLCK, or F_UNLOCK */
	lock.l_len= lock.l_start= 0;		/* byte offset relative to lock.l_whence */
	lock.l_whence=SEEK_SET;			/* or SEEK_CUR or SEEK_END*/
	return( fcntl(fd,cmd, &lock) );
}

/* Does the work of checkDupEventTimeout() for event log linfo. */
static void
checkDupEventTimeoutForLog(log_info_t *linfo, struct timeval *tv)
{
	/*
	 * timeout is the number of seconds we need to wait before 
	 * logging the discarded-dups event we have pending for this log.
	 */
	long timeout;
	time_t expireTime;
	time_t now = time(0);
	dup_buffer_t *dup_ptr = linfo->dup_recs_array;
	int count = 0;

	if (linfo->dup_count == 0 || linfo->dup_recs_array == NULL) {
		/* No dups in the entire array pending -- the usual case */
		return;
	}

	for (count = 0; count < dup_array_size; count++) {

		if (dup_ptr->dup_count > 0) {
			/* This record has pending dups. */
			
			expireTime = dup_ptr->time_start + dup_interval;
			timeout = expireTime - now;

			if (timeout <= 0) {
				/* Time to log the discarded-dups event. */
				struct posix_log_entry *rechdr = 
					(struct posix_log_entry *)dup_ptr->rec;
				writeDiscardEvent(linfo, rechdr->log_facility,
								  rechdr->log_event_type, dup_ptr->dup_count);
				linfo->dup_count -= dup_ptr->dup_count;
				dup_ptr->dup_count = 0;
				dup_ptr->time_start = 0;
				dup_ptr->rec = NULL;
				buf[dup_ptr->buf_index].in_use = 0;
			} else {
				/* Not yet time to log the discarded-dups event. */
				if (tv->tv_sec == 0 || tv->tv_sec > timeout) {
					tv->tv_sec = timeout;
				}
			}
		}
		
		dup_ptr++;
	}
}

/* Writes any pending duplicate discard events. Ie, the duplicate count
 * is greater than 0. We also free up any memory used to hold the info
 * about the duplicates.
 */
static void
dumpDuplicatesBuffer(log_info_t *linfo)
{
	int count;
	dup_buffer_t *dup_ptr = linfo->dup_recs_array;

	if (dup_ptr == NULL)
		return;

	for (count = 0; count < dup_array_size; count++) {
		if (dup_ptr->dup_count > 0) {
			struct posix_log_entry *rechdr =
				(struct posix_log_entry *)dup_ptr->rec;
			writeDiscardEvent(linfo, rechdr->log_facility,
							  rechdr->log_event_type,
							  dup_ptr->dup_count);
			dup_ptr->dup_count = 0;
			dup_ptr->time_start = 0;
			dup_ptr->rec = NULL;
			buf[dup_ptr->buf_index].in_use = 0;
		}
		dup_ptr++;
	}

	linfo->dup_count = 0;
	free(linfo->dup_recs_array);
	linfo->dup_recs_array = NULL;
}


/* FUNCTION: reallocBuffer
 *
 * PURPOSE: Reallocates the lookback buffer. This generally needs to occur
 * 	    when the size of the buffer has changed. If the prev_on is true,
 * 	    then prior to calling this we were discarding duplicates, so
 * 	    we'll need to dump the duplicates buffer. The size of the new
 * 	    buffer will depend on cur_on, which if true means we are currently
 * 	    discarding duplicates.
 *
 * CALLED BY: updateConf, main
 *
 * ARGUMENTS:
 * 		newsize	- Size to set dup_array_size to. If cur_on is false,
 * 			  this value is ignored for resizing the buffer,
 * 			  otherwise the actual buffer size will be two times
 * 			  this value plus one.
 * 		
 * 		prev_on - True if dis_dup_recs was true before calling this
 * 			  function.
 * 		cur_on	- True if dis_dup_recs will be true after this
 * 			  function.
 * 		
 * RETURN VALUES:
 * 		None
 */
static void
reallocBuffer(size_t newsize, int prev_on, int cur_on) {
	if (prev_on) {
		dumpDuplicatesBuffer(&evl_log);
		dumpDuplicatesBuffer(&pvt_log);
	}
	free(buf);
	dup_array_size = newsize;
	if (cur_on) {
		buf_size = 2 * dup_array_size + 1;
	} else {
		buf_size = 1;
	}
	
	buf = calloc(buf_size, sizeof(event_buffer_t));
}


/*
 * This function ensures that a "Discarded %d duplicate events" event always
 * gets logged in a timely manner, even if there is no subsequent event to
 * trigger its creation.
 * 
 * If there is a discarded-dups event pending, and it's time to log it (i.e.,
 * the interval set by "evlconfig -i" has expired), we log it.  If there
 * is a discarded-dups event pending, but it's not yet time to log it, we
 * set *tv to the appropriate timeout to ensure that the caller's select()
 * returns in time for us to log it.
 * 
 * If, after we're done, there is no discarded-dups event pending, we set
 * *tv to {0,0} (interpreted by the caller as no timeout) because in that
 * case, select() doesn't need to return until a new event or some such
 * happens.
 */
static void
checkDupEventTimeout(struct timeval *tv)
{
	tv->tv_sec = 0;
	tv->tv_usec = 0;

	checkDupEventTimeoutForLog(&evl_log, tv);
	checkDupEventTimeoutForLog(&pvt_log, tv);
}

/* 
 * This funtion TRIES to determine the hardware clock is set as local time
 * or UTC. The hwclock program store the UTC or LOCAL information in the 
 * /etc/adjtime (line 3). Basically, if /etc/adjtime does not exist hwclock 
 * is LOCAL, if it exists then line 3 would indicate hwclock is LOCAL or UTC.
 *  
 * See 'hwclock' man page for detail. 
 * 
 */

int
isRTC_Local()
{
	char line[256];
	FILE *f;

	if ((f = fopen("/etc/adjtime", "r")) == NULL) {
		/* file does not exist, it is local time */
		return 1;
	}
	/* get first line */
	if (fgets(line, 256, f) != NULL) {
		/* get second line */
		if (fgets(line, 256, f) != NULL) {
			/* get the third line - this is where the GMT info is*/
			if (fgets(line, 256, f) != NULL) {
				if (!strncasecmp(line, "UTC", 3)) {
					fclose(f);
					return 0;
				}
			}
		}
	}
	fclose(f);
	return 1;
}

/*
 * This function will be invoke by the backend plug-ins. The plug-ins should
 * pass back the cmd (cmd is pre-defined in callback.h) and some data.
 */ 	
int
evl_callback(int cmd, const char * data1, const char * data2)
{
	switch(cmd) {
	case CB_WR_NOTIFY:
		writeEvtToNfyDaemon((char *)data1);
		break;
	case CB_MK_EVL_REC:
		mk_evl_rec(&evl_log,  LOG_LOGMGMT, 1, LOG_NOTICE, "%s", data1);
		break;
	case CB_PLUGIN_FD_SET:
	{
		int sd = atoi(data1);	
		if (sd > 0) {
			FD_SET(sd, &all_sds);
			maxsd = MAX(sd, maxsd);
		}
		break;
	}
	case CB_PLUGIN_FD_UNSET:
	{
		int sd = atoi(data1);
		if (sd > 0) {
			rmSocket(sd);
		}
		break;
	}
	case CB_UNLOAD_PLUGIN:
	default:
		/* Don't do anything */
		fprintf(stderr, "Called back, but do nothing here.\n");
		break;
	}
	return 0;
}

char * 
getProcessCmd(pid_t pid)
{	
	char buf[256 + 1];
	char *cmd;
	char path[40];
	int fd;
	size_t	r;

	snprintf(path, sizeof(path), "/proc/%u/cmdline", pid);
	if((fd = open(path, O_RDONLY)) == -1) {
		return NULL;
	}
	
	if (( r = read(fd, buf, 256)) != -1) {
		buf[r] = '\0';
	}

	if((cmd = malloc(strlen(buf)+ 1)) == NULL) {
		close(fd);
		return NULL;
	}
	strcpy(cmd, buf);
	close(fd);
	return r == -1? NULL : cmd;
}

int
establishNfyConnection(struct sockaddr *sa)
{
	int sd, retry=0;
	size_t sock_len;
	if ((sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0){
		(void)fprintf(stderr, "evlnotifyd: Server cannot create a connection to the evlogd daemon.\n");
		return -1;
	}
	sa->sa_family = PF_UNIX;
	(void) strcpy(sa->sa_data, EVLOGD_SERVER_SOCKET);
	sock_len = sizeof(sa->sa_family) + strlen(sa->sa_data);
	
	while (connect(sd, sa, sock_len) < 0) {
		retry++;
		if (retry==5) {
			fprintf(stderr, "evlogd: can't connect to notify daemon\n");
			close(sd);
			return -1;
		}
	}
	return sd;
}
	
