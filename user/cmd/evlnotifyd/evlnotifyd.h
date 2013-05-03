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


#ifndef _EVL_NOTIFYD_H_
#define _EVL_NOTIFYD_H_

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stdout, fmt, ##args)
#endif

#ifndef DEBUG2
#define TRACE(fmt, args...)		/* fprintf(stdout, fmt, ##args) */
#endif


#define LOGERROR(evt_type, fmt, args...)posix_log_printf(LOG_LOGMGMT, evt_type, LOG_CRIT, 0, fmt, ##args)


#define max(a,b) (((a)>(b))?(a):(b))
/* The notification request header */
typedef struct _nfy_req_hdr {
	HANDLE handle;
	int pid;
	int sd;
	int flags;
} nfy_req_hdr_t;

/* Notification request object */
typedef struct _nfyReq {
	nfy_req_hdr_t 		nfyhdr;		/* the header */
	sigevent_t 		nfy_sigevent;	
	posix_log_query_t 	nfy_query;	/* the query string */
	char *procCmd;				/* the process's command line */					
} nfyReq_t;

/*
 * The notification request record header
 */
typedef struct _req_rec_hdr {
	int recMagic;
	nfy_req_hdr_t 	nfyhdr;
	sigevent_t 	sig_evt;
	int queryLen;
	int cmdLineLen;
} req_rec_hdr_t;


typedef struct clientinfo {
	int sd;
	uid_t uid;
	pid_t pid;
	int numofreg; 			/* number of registered request per this client */
} clientinfo_t;

#define MAX_CLIENTS				128

#define CLIENT_SLOT_AVAILABLE	-100
#define SMALL_BUF_SIZE			1024	
#define MAX_PROGRAM_NAME		64
#define MAX_LIST_DIRTY_COUNT	100

#define RM_REQUESTS				1



extern evl_listnode_t *pNfyReqList;	/* List of notify request */
static 	HANDLE notifyReqHandle = 0; 

/* Prototype functions */	
void _daemonize();
void initClients();
static void SIGTERM_handler();
int establishEvlogdConnection(struct sockaddr *sa);
int processEvent(int sd);
int processClientRequest(clientinfo_t *ci);
HANDLE registerRequest(clientinfo_t *ci, nfyNewRqMsg_t * req_msg, char * queryStr, int query_flags);
int writeRequestStatus(int sd, HANDLE handle, int status);
void closeSocket(int sd);
int getMaxClientIndex();
void closeClientSocket(clientinfo_t *ci, int removeRequests);
int connected(int sd);
int security_test();

/* Notify list related functions */
nfyReq_t * findRequest(HANDLE handle);
int removeRequestWithHandle(HANDLE handle);
void removeRequestsWithSD(int sd);
void freeNfyData(void *req);
int disableRequest(HANDLE handle);
int flushRequestList();
void removeZombieRequestsInList();

HANDLE createNewHandle();
void setNotifyHandle(HANDLE h);
char * getProcessCmd(pid_t pid);

/* Serialization functions */
int writeRequestsToFile();
int appendReqToFile(nfyReq_t *req);
int writeReqToFile(int fd, nfyReq_t *req);
int readNextReqHdrFromFile(int fd, req_rec_hdr_t *rec);
int readNextCmdStrFromFile(int fd, char *cmd_str, int len);
int readNextQueryStrFromFile(int fd, char *qu_str, int len);
int loadReqFromFile();
int openReqFile(int flags);
void closeReqFile(int fd, int fileop_success);
int validateProc(pid_t pid, char *cmd_str);

#endif     /* _EVL_NOTIFYD_H_ */
