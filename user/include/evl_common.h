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

#ifndef _EVL_COMMON_
#define _EVL_COMMON_
#include <sys/un.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int _evlCreateListenSocket(struct sockaddr_un *sa,
				const char *sock_path_name, int backlog);
extern int _evlReadEx(int sd, void * buf, size_t len);
extern int _evlGetUserGroups(uid_t uid, size_t size, gid_t list[]);
int _evlVerifyUserCredential(uid_t euid, gid_t egid, int (*sec_verify_func)());
size_t _evlSplitCmd(char * cmdbuf, size_t n, char **argv);
void _evlTrim(const char *str);
int _evlValidate_pid(char *pfile);
int _evlUpdate_pid(char *pfile);
extern void _evlLockMutex(pthread_mutex_t *mutex);
extern void _evlUnlockMutex(pthread_mutex_t *mutex);
extern void _evlLockFd(int fd, int type);
extern void _evlUnlockFd(int fd);

#ifdef __cplusplus
}
#endif

#endif /* _EVL_COMMON_*/

