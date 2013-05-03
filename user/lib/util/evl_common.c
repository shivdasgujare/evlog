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
#include <sys/socket.h>       	
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include "config.h"
#include "posix_evlog.h" 
#include "evlog.h"

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stdout, fmt, ##args)
#endif

#ifndef DEBUG2
#define TRACE(fmt, args...)		/* fprintf(stdout, fmt, ##args) */
#endif

#define LOGERROR(evt_type, fmt, args...)	posix_log_printf(LOG_LOGMGMT, evt_type, LOG_CRIT, 0, fmt, ##args)

/* 
 * FUNCTION	: _evlCreateListenSocket
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int _evlCreateListenSocket(struct sockaddr_un *sa, const char *sock_path_name, int backlog)
{
	int sd;
	size_t listen_sock_len;           /* Size of generic UD address structure */
	
	if ((sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket: listen_sd");
		exit(1);
	}
    memset(sa, 0, sizeof(struct sockaddr_un));
	sa->sun_family = PF_UNIX;
	(void)strcpy(sa->sun_path, sock_path_name);
	listen_sock_len = sizeof(sa->sun_family) + strlen(sa->sun_path);

	if (bind(sd, (struct sockaddr*)sa, listen_sock_len) < 0) {
		perror("bind: listen_sd");
		exit(1);
	}
	/* Allow everyone to connect now,
	 * we will verify user credential later.
	 */
	(void) chmod(sock_path_name, 0666);
	if (listen(sd, backlog) < 0) {
		perror("listen: sd");
		exit(1);
	}
	
	return sd;
}

/*
 * FUNCTION	: _evlReadEx
 *
 * PURPOSE	:
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */

int _evlReadEx(int sd, void * buf, size_t len)
{
	int n = 0;
	int moreBytes = len;
	void *tmp;
	tmp = buf;
	while (moreBytes > 0) {
		n = read(sd, tmp, moreBytes);
		if (n < 0) {
				fprintf(stderr, "read error on sd %d\n", sd);
				return -1;
		} else {
			if (n == 0) {
				//fprintf(stderr, "Socket sd %d closed\n", sd);
				return -1;
			}
			moreBytes -= n;
			tmp += n;
		}
	}
	return len - moreBytes;
}

/* 
 * FUNCTION	: getusergroups
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int _evlGetUserGroups(uid_t uid, size_t size, gid_t list[])
{
	struct group *grp;
	struct passwd *pw;
	int n = 0;
	char *p;

	setgrent();
	while((grp = getgrent()) != NULL) {
		int i=0;
		while((p = grp->gr_mem[i++]) != NULL)  {
			pw = getpwnam(p);
			if(pw->pw_uid == uid) {
				list[n++] = grp->gr_gid;
			}	
		}
	}
	endgrent();
	return n;
}

/* 
 * FUNCTION	: _evlVerifyUserCredential
 * 
 * PURPOSE	: This function provides the switching uid, gid and groups capability
 * 			  (to callers and back to the original id). 
 * 			  Caller provides the uid, gid and a pointer to the function that's
 * 			  actually doing to security check. 
 * 
 * ARGS		:	
 * 
 * RETURN	: return the return code of the security test function
 * 
 */
int _evlVerifyUserCredential(uid_t euid, gid_t egid, int (*sec_verify_func)())
{
	int ret = 0;
	uid_t ruid;
	gid_t rgid;
	struct passwd *pw;

	/*
	 * Save the original uid, gid
	 */
	ruid = getuid();
	rgid = getgid();
	TRACE("ruid=%d : rgid=%d --- euid=%d : egid=%d\n", 
			getuid(), getgid(), geteuid(), getegid());

	/*
	 * Temporary set the uid and gid and groups to connecting user
	 */
	if (setegid(egid) == -1) {
		LOGERROR(EVLOG_GID_OP_FAILED, "setegid failed errno=%d", errno);
		fprintf(stderr, "Failed to change group ID to %d.\n", egid);
		return -1;
	}

	pw = getpwuid(euid);
	if (pw == NULL) {
		return -1;
	}
	if (initgroups(pw->pw_name, egid) == -1) {
		return -1;
	}
	

	if (seteuid(euid) == -1) {
		LOGERROR(EVLOG_UID_OP_FAILED, "seteuid failed errno=%d", errno);
		fprintf(stderr, "Failed to change user ID to %d.\n", euid);
	}
	TRACE("ruid=%d : rgid=%d --- euid=%d : egid=%d\n", 
			getuid(), getgid(), geteuid(), getegid());
			
	/*
	 * Callers plug in their own security test function,
	 * I am going to pass the return code from the security test func
	 * back to the callers  
	 */

	ret = (*sec_verify_func)();

exit_credential_check:
	if (seteuid(ruid) == -1) {
		LOGERROR(EVLOG_UID_OP_FAILED, "seteuid failed errno=%d", errno);
		fprintf(stderr, "Failed to change user ID back to %d.\n", ruid);
	}
	if (setegid(rgid) == -1) {
		LOGERROR(EVLOG_GID_OP_FAILED, "setegid failed errno=%d", errno);
		fprintf(stderr, "Failed to change group ID back to %d.\n", rgid);
	}
	TRACE("ruid=%d : rgid=%d --- euid=%d : egid=%d\n", 
			getuid(), getgid(), geteuid(), getegid());
			
	pw = getpwuid(ruid);
	if (pw == NULL) {
		return -1;
	}
	/* Just put back the groups that root guy belongs to */
	if (initgroups(pw->pw_name, rgid) == -1) {
		return -1;
	}
	return ret;
}

/*
 * FUNCTION	: _elvSplitCmd
 *
 * PURPOSE	: split string to array of words
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
size_t
_evlSplitCmd(char * cmdbuf, size_t n, char **argv)
{
	int nsplit;
    char c;
    char *tmp;
	unsigned int dbquote_open = 0x0;
	unsigned int squote_open = 0x0;
	nsplit = 0;

	tmp = cmdbuf;
	do {
		if (nsplit >=  n) {
			break;
		}
		c = *cmdbuf;
		if ((c == ' ' || c == '\t' || c == '\0') && dbquote_open == 0 && squote_open == 0) {
			*cmdbuf = '\0';
			if ((*tmp == '\'' || *tmp == '\"') &&
				(tmp[strlen(tmp)-1] == '\'' || tmp[strlen(tmp)-1] == '\"')) {
				/* try to ged rid of the quote */
				tmp[strlen(tmp)-1] = '\0';
				argv[nsplit++] = tmp+1;
			} else {
				argv[nsplit++] = tmp;
			}
			/* point to the next token */
			tmp = cmdbuf + sizeof(char);
			/* eat all white space */
			while (*tmp == ' ' || *tmp == '\t') {
				tmp++;
			}
			cmdbuf = tmp;
			continue;
			
		}
		if (c == '\"' && !squote_open) {
			dbquote_open ^= 0x1;
		}
		if (c == '\'' && !dbquote_open) {
			squote_open ^= 0x1;
		}
		cmdbuf++;
	} while (c != '\0') ;

	return nsplit;
}

/*
 * FUNCTION	: _elvTrim
 *
 * PURPOSE	: Remove leading and trailing white spaces in a string
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
void _evlTrim (const char * str)
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

/*
 * If the PID present in the file is running, return 0. Otherwise return 1.
 */
int
_evlValidate_pid(char *pfile)
{
	int f_pid;
	FILE *f;

	if ((f = fopen(pfile, "r")) == NULL) {
		return 0;
	}
	fscanf(f, "%d", &f_pid);
	fclose(f);
	if (kill(f_pid, 0) && errno == ESRCH) {
		return 0;
	}
    
	return 1;
}
	
/*
 * Update it's PID in the 'pfile'.
 */
int 
_evlUpdate_pid(char *pfile)
{
	int pid;
	FILE *f;
	int fd;

	if ((fd = open(pfile, O_RDWR|O_CREAT, 0644)) == -1) {
		return 0;
	}
   	if ((f = fdopen(fd, "r+")) == NULL) {
		close(fd);
		return 0;
	}
	pid = getpid();
	flock(fd, LOCK_EX);
	fprintf(f, "%d\n", pid);
	fclose(f);	/* closes fd; releases locks */

   	return 1;	
}

void
_evlLockMutex(pthread_mutex_t *mutex)
{
#ifdef _POSIX_THREADS
	int status = pthread_mutex_lock(mutex);
	assert(status == 0);
#endif
}

void
_evlUnlockMutex(pthread_mutex_t *mutex)
{
#ifdef _POSIX_THREADS
	int status = pthread_mutex_unlock(mutex);
	assert(status == 0);
#endif
}

/*
 * Assert a file lock on the entire file whose file descriptor is fd.
 * type is either F_RDLCK (shared lock) or F_WRLCK (exclusive/write lock).
 * If somebody else has the lock, we sleep until it's released.
 */
void
_evlLockFd(int fd, int type)
{
	struct flock lock;
	lock.l_type = type;
	lock.l_len = 0;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	(void) fcntl(fd, F_SETLKW, &lock);
}

/* Unlock a file lock previously set by _evlLockFd(). */
void
_evlUnlockFd(int fd)
{
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_len = 0;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	(void) fcntl(fd, F_SETLKW, &lock);
}
