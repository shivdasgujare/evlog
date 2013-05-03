/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2002
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

#include <stdio.h>            	
#include <string.h>           	
#include <fcntl.h>            	
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sys/syscall.h>
#include <syslog.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_list.h"
#include "evl_common.h"
#include "evlnotifyd.h"


#ifdef __ia64__
#define __ia64_syscall syscall
#endif

/*_syscall3(int, rt_sigqueueinfo, pid_t, pid, int, sig_no, siginfo_t *, si)*/


evl_listnode_t *pNfyReqList;		/* List of notify request */
static int evlogd_sd = 0;				/* EVLOGD socket descriptor */
static int maxsd;                   /* Largest socket descriptor used by this server */
fd_set all_sds;                  	/* The set of all socket descriptors known to this server */
static size_t list_dirty_ctr = 0;	/* Keep track how many requests that are removed/disabled 
									 * in the list we will kick off the flushList routine when 
									 * it reaches certain limit.
			 						 */	
static char notifyd_connect_str[] = "Notification daemon is trying to connect to evlogd.";

clientinfo_t clients[MAX_CLIENTS];
static int maxci;
static char *PidFile = "/var/run/evlnotifyd.pid";
int
main(int argc, char **argv)
{
	struct sigaction act;			/* Needed to disable signal SIGCHLD with SIG_IGN */ 
	struct sockaddr_un sock;			/* Generic UD socket address */
	
	int listen_sd;					/* Unix Domain (UD) socket descriptor for local clients */
	int listen_evlogd_sd;
	struct sockaddr_un listen_sock;	/* Generic UD socket address */

	int nfyreqs_fd;
	int optval;						/* setsockopt() var */
	size_t optlen;					/* setsockopt() var */
	int newsd;                      /* New socket descriptor */
		
	fd_set read_sds;                /* The set of socket descriptors ready to be read */
	int daemonize = 1;				/* run this app as a daemon */
	auto int c;						/* Command line option */

	static struct sigaction SigTERMAction;  /* Signal handler to terminate gracefully */
	void SIGTERM_handler();

	/* Process command line options */
	while ((c = getopt(argc, argv, "f")) != EOF) {
        switch (c) {
        	case 'f':
            	daemonize = 0;
               	break;
            default:
		break;
        }
    }
	TRACE("evlnotifyd starting ...\n");
	/* Daemonize */
	if (daemonize) {
		_daemonize();
	}
	
	/* Init clients info array */
	initClients();
	
	/*
	 * Create the list with the first node is empty (NULL). 
	 * This first node will always exist from now on. 
	 * removeAlRequests will remove all the nodes but this node.
	 */
	pNfyReqList =_evlMkListNode(NULL);
	
	/*
	 * Reloading the notification request from file
	 */
	 
	if (loadReqFromFile() < 0) {
		(void)fprintf(stderr, "Failed to re-load request.\n");
	} else {
		TRACE("Loading requests from file succeeded\n");
	}

	/*
	 * Create new signal handler for SIGTERM. This will do the cleanup of
	 * all sockets and terminate gracefully.
	 */

	(void) memset(&SigTERMAction, 0, sizeof(SigTERMAction));
	SigTERMAction.sa_handler = SIGTERM_handler;
	SigTERMAction.sa_flags = 0;

	if (sigaction(SIGTERM, &SigTERMAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGTERM.\n", argv[0]);
		perror("sigaction");
	}
	if (sigaction(SIGHUP, &SigTERMAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGHUP.\n", argv[0]);
		perror("sigaction");
	}
	if (sigaction(SIGINT, &SigTERMAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGINT.\n", argv[0]);
		perror("sigaction");
	}
#if 0
	if (sigaction(SIGSEGV, &SigTERMAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGSEGV.\n", argv[0]);
		perror("sigaction");
	}
#endif

	/*
	 * Ignore SIGCHLD
	 */
	(void) memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;

	if (sigaction(SIGCHLD, &act, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGCHLD.\n", argv[0]);
		perror("sigaction");
		exit(1);
	}
	
	/*
	 * Ignore any SIGPIPE signal.  A broken pipe is a normal occurrance as
	 * clients come and go.  We don't want the server to stop because of one.
	 */
	if (sigaction(SIGPIPE, &act, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGPIPE.\n", argv[0]);
		perror("sigaction");
		exit(1);
	}
	
	
	/* unlink notify server address */
	(void) unlink(EVLOGD_SERVER_SOCKET);
	listen_evlogd_sd = _evlCreateListenSocket(&sock, EVLOGD_SERVER_SOCKET, 0);
	(void) unlink(EVLNOTIFYD_SERVER_SOCKET);
	listen_sd = _evlCreateListenSocket(&listen_sock, EVLNOTIFYD_SERVER_SOCKET, MAX_CLIENTS);
	
	
	FD_ZERO(&all_sds);
	FD_SET(listen_evlogd_sd, &all_sds);
	FD_SET(listen_sd, &all_sds);
	maxsd = max(listen_evlogd_sd, listen_sd);
	
	
	for (;;) {
		int newsd;
		int nsel, i;
		struct ucred ucred;			/* notification */
		socklen_t ucredsz = sizeof(struct ucred);	/* clients credential */
		
		bcopy((char *)&all_sds, (char *)&read_sds, sizeof(all_sds));
		
		if ((nsel = select(maxsd + 1, &read_sds, 0, 0, 0)) < 0) {
			if (errno == EINTR) {		
				continue;
			} else if (errno != EBADF) {
				exit(1);
			}
		}

		if (nsel == 0) {
			/* timeout */
			/*TRACE("select timeout\n");*/
			continue;
		}
		if (FD_ISSET(listen_evlogd_sd, &read_sds)) {
			/* evlogd is trying to connect */
			if ((newsd = accept(listen_evlogd_sd, NULL, NULL)) < 0) {
				(void)fprintf(stderr, "evlnotifyd: can't accept connection from evlogd\n");
				perror("accept");
				exit(1);
			}
			FD_SET(newsd, &all_sds);
			if (maxsd < newsd) {
				maxsd = newsd;
			}
			evlogd_sd = newsd;
			TRACE("evlogd daemon connected. evlogd_sd = %d, maxsd=%d\n", newsd, maxsd);
		}
		if (FD_ISSET(listen_sd, &read_sds)) {
			/* Client is trying to connect. */
			char abyte;
            		clientinfo_t *cl, *empty;
            		int idx;
			
			if ((newsd = accept(listen_sd, NULL, NULL)) < 0) {
				perror("accept");
				goto exit_new_sd;
			}
			/* Look for a free client slot; while doing
   			 * so we count the number of connections
   			 * per this uid, as well as recomputing the
   			 * max client index. */
   			empty = NULL;
   			cl = clients;
   			maxci = 0;
   			for (idx = 0; idx < MAX_CLIENTS; idx++, cl++) {
   				if (cl->sd != CLIENT_SLOT_AVAILABLE)
   					maxci = idx;
   				else if (empty == NULL) {
   				 	empty = cl;
   					maxci = idx;
   				}
   			}	
   			if (empty == NULL) {
   				TRACE("Max number of clients reached.\n");
				abyte = NFY_MAX_CLIENTS;
				write(newsd, &abyte, sizeof(char));
   				close(newsd);
   				goto exit_new_sd;	
   			}
			
			if (getsockopt(newsd, SOL_SOCKET, SO_PEERCRED, &ucred, &ucredsz) < 0) {
			    perror("getsockopt");
                close(newsd);
			} else {
				int idx;	
				/* 
				 * security_test is the pointer to a function that actually
				 * doing the test
				 */
				
				if (_evlVerifyUserCredential(ucred.uid, ucred.gid, security_test) != 0) {
					TRACE("Failed user credential check.\n");
					abyte = NFY_ACCESS_DENIED;
					write(newsd, &abyte, sizeof(char));
					close(newsd);
					goto exit_new_sd;
				}
				
				abyte = NFY_ACCESS_GRANTED;
				write(newsd, &abyte, sizeof(char));
				
                empty->sd = newsd;
				empty->uid = ucred.uid;
				empty->pid = ucred.pid;
                FD_SET(newsd, &all_sds);
			    if (maxsd < newsd) {
				    maxsd = newsd;
			    }
                TRACE("newsd = %d, maxsd=%d\n", newsd, maxsd);
			}
		exit_new_sd: /*make gcc happy */;
		}

		/* process incomming event */	
		if (evlogd_sd > 0 && FD_ISSET(evlogd_sd, &read_sds)) {
			if ( -1 == processEvent(evlogd_sd)) {
				evlogd_sd = 0;
			}
		}
		
		/* prosess clients */
        for (i= 0; i <= maxci; i++) {
        	if ( clients[i].sd != CLIENT_SLOT_AVAILABLE) {
        		if (FD_ISSET(clients[i].sd, &read_sds)) {
        			processClientRequest(&clients[i]);
				}
			}
		}
	}	/* end for(;;) */
	
} /* END OF MAIN */

/* 
 * FUNCTION	: _daemonize
 * 
 * PURPOSE	: daemonizes this process
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
 void _daemonize()
 {
 	pid_t pid;
 	int num_fds, i;

	if (_evlValidate_pid(PidFile)) {
		fprintf(stderr, "evlnotifyd: Already running.\n");
		exit(1);
	}
	/*
	 * Fork a child and let the parent exit. This guarentees that
	 * the first child is not a process group leader.
	 */
	if ((pid = fork()) < 0) {
		fprintf(stderr, 
				"evlnotifyd: Cannot fork child process. Check system process usage.\n"); 
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
				"evlnotifyd: Cannot fork child process. Check system process usage.\n"); 
		exit(1);
	} else if (pid > 0) {
		exit(0);
	}

	/* 
	 * Save this pid to to a file, if it is not running yet.
	 * We allow only one instance of this process at a time.
	 * 
	 */ 
	if (!_evlUpdate_pid(PidFile)) {
#if 0
		LOGERROR(EVLOG_WRITE_PID, "evlnotifyd: Cannot write 'evlnotifyd' PID to '%s' file\n",
				PidFile);
#endif
			exit(1);
	}
	/*  Redirect fd 0 1 2 to /dev/null */
	{
	int devnull;
	devnull = open("/dev/null", O_WRONLY);
	if (devnull != -1) {
	num_fds = 2;
	for (i=0; i <= num_fds; i++) {
		(void)close(i);
		dup2(devnull, i);
	} 
	close(devnull);
	}
	}
	
	/*
	 * Clear any inherited file mode creation mask.
	 */
	(void)umask(0);
	(void)mkdir(LOG_EVLOG_DIR, 755);
	(void)chdir(LOG_EVLOG_DIR);

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
 void
 initClients()
 {
 	int i;
 	maxci = 0;
 	for (i=0; i < MAX_CLIENTS; i++) {
 		clients[i].sd = CLIENT_SLOT_AVAILABLE;
 		clients[i].numofreg = 0;
 	}
 }
 		

/* 
 * FUNCTION	: SIGTERM_handler
 * 
 * PURPOSE	: Handling terminate signal, save request list to file
 *			  and clean up before exit.
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
static void
SIGTERM_handler()
{
	int fd;	
	void (* freedata_fptr);

	freedata_fptr = freeNfyData;
	TRACE("evl_notifyd: SIGTERM handler called\n");
	(void) unlink(EVLNOTIFYD_SERVER_SOCKET);
	(void) unlink(PidFile);
	/*
	 * The notification daemon exits upon receiving a SIGTERM signal.
	 * It is good time to flush the nfy request list to file and
	 * clean up the mess.
	 */
	flushRequestList();
	/* Clean up memory - too defensive? */
	removeAllNodes(pNfyReqList, freedata_fptr);
	free(pNfyReqList);
	exit(0);
}

/*
 * FUNCTION	: connected
 *
 * PURPOSE	: Exchanges hand shake with client to verify connection
 *
 * ARGS		:	
 *
 * RETURN	: returns 0 if succeeded, otherwise -1
 *
 */
int 
connected(int sd)
{
	unsigned char c=0x0;
	
	if (read(sd, &c, sizeof(char)) != sizeof(char)) {
		return -1;
	}
	if ((write(sd, &c, sizeof(char)) != sizeof(char)) && (c != 0xac )) {
		return -1;
	}
	
	return 0;
}


/* Compute max client index */
int 
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
 * PURPOSE	: Closes the client socket and remove client's request
 *
 * ARGS		: client info structure, remove request flag	
 *
 * RETURN	:
 *
 */
void 
closeClientSocket(clientinfo_t *ci, int removeRequests)
{
	int i;
	int new_maxsd = 0;
	int tbclosedsd = ci->sd;
	
	FD_CLR(ci->sd, &all_sds);
	(void) close(ci->sd);
	ci->sd = CLIENT_SLOT_AVAILABLE;

	if(removeRequests == RM_REQUESTS ) {
		removeRequestsWithSD(ci->sd);
	}
	TRACE("closeClientSocket:sd = %d maxsd = %d\n", tbclosedsd, maxsd);

	/* 
	 * If maxsd = evlogd_sd, we don't need to compute the new 
	 * maxsd
	 */ 
	if (maxsd == evlogd_sd) {
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
	 * If new_maxsd = 0 that means there is no client currently
	 * connects to evlnotifyd, and the to be closed sd = maxsd then
	 * maxsd shoulde be maxsd-1
	 */
	if (new_maxsd == 0 && tbclosedsd == maxsd) {
		maxsd--;
	} else if (new_maxsd != 0) {
        if (new_maxsd < evlogd_sd)
            maxsd = evlogd_sd;
        else
		    maxsd = new_maxsd;
	}
	
	TRACE("Done closeClientSocket: maxsd = %d\n", maxsd);
	return;
}

/*
 * FUNCTION	:  closeSocket()
 *
 * PURPOSE	:
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
void
closeSocket(int sd)
{
	FD_CLR(sd, &all_sds);
	if (sd == maxsd) {
		maxsd--;
	}
	(void) close(sd);
	
	TRACE( "Done closing socket sd %d\n", sd);
	return;
}
/* 
 * FUNCTION	:  security_test()
 * 
 * PURPOSE	:  This function tests to see if user can open the log file.
 *		   To effectively do this security test user need to pass 
 *		   the uid, gid, the pointer of this function to the 
 *		   verifyUserCredential.
 *		   The verifyUserCredential function switches to the 
 *		   appropriate uid gid then calls this function, when it is 
 *		   done the verifyUserCredential will switch its identity 
 *		   back to previous ID
 *
 * ARGS		:	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */
int 
security_test()
{
	int fd;

	if ((fd = open(LOG_PRIVATE_PATH, O_RDONLY))!= -1) {
		/* user can open this privatelog - yahoo */
		close(fd);
		return 0;
	}
	
	if ((fd = open(LOG_CURLOG_PATH, O_RDONLY)) == -1) {
		return -1;
	}
	close(fd);
	return 0;
}

/* 
 * FUNCTION	: processEvent
 * 
 * PURPOSE	: Process the incomming event from evlogd
 *			1. Get the event
 *			2. Look for a match in the list
 *			3. Fire up a signal vi rt_sigqueueinfo if match found
 *
 * 
 * ARGS		: the socket descriptor for the connection to evlogd	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */
int 
processEvent(int sd) 
{
	unsigned char buf[POSIX_LOG_ENTRY_MAXLEN];
	unsigned char *recbuf;
	int nbytes;
	struct posix_log_entry rec_hdr;

	nfyReq_t *req;
	evl_listnode_t *node, *startnode;

	TRACE("Got an event arrive on sd %d\n", sd);
	/* first read the record header */
	if ((nbytes = _evlReadEx(sd, &rec_hdr, REC_HDR_SIZE)) == -1) {
		closeSocket(sd);
		return -1;
	}
		
	if (rec_hdr.log_size <= 0) {
		recbuf = NULL;	
	}
	else {
		recbuf = buf;
		if((nbytes = _evlReadEx(sd, recbuf, rec_hdr.log_size)) < 0) {
			closeSocket(sd);
			return -1;
		}
	}
	/* 
	 * Check if there are any request - Search the entire list
	 */
	for ( node = pNfyReqList->li_next; node != pNfyReqList; node = node->li_next) {
		req = (nfyReq_t *) node->li_data;
		/*
		 * If request is enable, check the incomming event to see
		 * if it is matching with the registered request
		 */
		if((req->nfyhdr.flags & POSIX_LOG_NFY_DISABLED) == 0 ) {
			int match;
			siginfo_t si;
			union sigval val;
			int ret;
			if (req->nfy_query.qu_tree != NULL) {
				if (posix_log_query_match(&req->nfy_query, &rec_hdr, recbuf, &match) != 0) {
		 			fprintf(stderr, "posix_log_query_match: Internal error: \n");
		 			continue;
		 		} else if (match) {
		 			TRACE( "Match found.\n");
		 		
		 		} else {
		 			continue;
		 		}
		 	} else {
		 		TRACE( "NULL query: match everything.\n");
		 	}
			/*
			 * Verify that the client for this request is still around
			 */
			if (validateProc(req->nfyhdr.pid, req->procCmd) == -1) {
			/*
			 * Process is not around - disable the request for now.
			 * We dont want to modify the list in this routine,
			 * we will kick in some gc later.
			 */
				TRACE("Process %d not found - disable the request handle %d\n",
						 req->nfyhdr.pid, req->nfyhdr.handle);
				req->nfyhdr.flags |= (POSIX_LOG_NFY_DISABLED | POSIX_LOG_NFY_PENDING_REMOVE);
				continue;
			}

			TRACE( "Enabled! Sending signal now, pid=%d, signo=%d...\n",
		 					req->nfyhdr.pid, req->nfy_sigevent.sigev_signo);
			 	
		 	si.si_code = SI_EVLOG;
		 	si.si_signo = req->nfy_sigevent.sigev_signo;
		 	si.si_errno = rec_hdr.log_recid;

		 	memcpy(&si._sifields._rt.si_sigval, &req->nfy_sigevent.sigev_value, sizeof(sigval_t));
			 	
		 	if ((ret = syscall(SYS_rt_sigqueueinfo, req->nfyhdr.pid, si.si_signo, &si)) != 0 ) {
				if(ret == -ESRCH) {
		 			/* process had gone away */
		 			TRACE("sigqueue return=%d\n", ret);
		 			req->nfyhdr.flags |= (POSIX_LOG_NFY_DISABLED | POSIX_LOG_NFY_PENDING_REMOVE);
				}
		 	}
			 	
		 	if (req->nfyhdr.flags & POSIX_LOG_ONCE_ONLY) {
				req->nfyhdr.flags |= POSIX_LOG_NFY_DISABLED;
			}

		}
	} /* end for */
	return 0; 
}

/* 
 * FUNCTION	: processClientRequest
 * 
 * PURPOSE	: processing client request (creat new request, remove, 
 * 		  get info)
 * 
 * ARGS		:	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */
int 
processClientRequest(clientinfo_t *ci)
{
	HANDLE handle;
	nfyNewRqMsg_t * reqMsg;
	char msgBuf[EVL_NFY_MSGMAXSIZE];
	int status, nbytesRead;
	char *queryStr;
	nfyMsgHdr_t resp;
	int fd;
	int query_flags;

#if 0
	/*
	 * Trading the handsake-byte with the client to
	 * confirm the connection.
	 */
	if(connected(ci->sd) == -1){
		closeClientSocket(ci, RM_REQUESTS);
		return -1;
	}
#endif	
	/* First read the msg header */
	if (_evlReadEx(ci->sd, msgBuf, sizeof(nfyNewRqMsg_t)) == -1) {
 		closeClientSocket(ci, RM_REQUESTS);
		return -1;
	}
	reqMsg = (nfyNewRqMsg_t *) msgBuf;
	
	switch(reqMsg->nwHeader.nhMsgType) {
		case nmtNewRequest:
		{
			if((queryStr = (char *) malloc(reqMsg->nwQueryLength + 1)) == NULL ) {
				return -1;
			}
			if ((nbytesRead = _evlReadEx(ci->sd, queryStr, reqMsg->nwQueryLength)) == -1) {
				free(queryStr);
				closeClientSocket(ci, RM_REQUESTS);
				return -1;
			}
			queryStr[nbytesRead] = '\0';
			/* get the query flags */
			if ((nbytesRead = _evlReadEx(ci->sd, &query_flags, sizeof(query_flags))) != sizeof(query_flags)) {
				free(queryStr);
				closeClientSocket(ci, RM_REQUESTS);
				return -1;
			}
			TRACE("query_flags=%d\n", query_flags);
	
			if ((handle = registerRequest(ci, reqMsg, queryStr, query_flags)) < 0) {
				/* Failed registering the request */
				status = handle;		
			} else {
				ci->numofreg++;
				status = nrsEnabled;
			}
	
			if (writeRequestStatus(ci->sd, handle, status) != 0 ) {
				free(queryStr);
				closeClientSocket(ci, RM_REQUESTS);
				return -1;
			}	
			free(queryStr);
			break;
		}
		case nmtRmRequest:
		{
			handle = reqMsg->nwHeader.nhReqHandle;
			
			if(removeRequestWithHandle(handle) == -1) {
				status = nrsNoRequest;
			} else {
				status = nrsRemoved;
				ci->numofreg--;
			}
			if (writeRequestStatus(ci->sd, handle, status) != 0 ) {
				closeClientSocket(ci, RM_REQUESTS);
			}
			break;
		}
		case nmtGetRequestStatus:
		{
			nfyReq_t *req;
			nfyNewRqMsg_t returnMsg;
			handle = reqMsg->nwHeader.nhReqHandle;
			/* 
			 * We are going to report back if the request is still
			 * in the system and whether it is enable or disable
			 */
			if((req = findRequest(handle)) == NULL) {
				status = nrsNoRequest;
			} else {
				if ((req->nfyhdr.flags & POSIX_LOG_NFY_DISABLED) == 0) {
					status = nrsEnabled;
				} else {
					status = nrsDisabled;
				}
			}
			if (writeRequestStatus(ci->sd, handle, status) != 0 ) {
				closeClientSocket(ci, RM_REQUESTS);
			}
			/*
			 * If the request is still in the system
			 */
			if(status != nrsNoRequest) {
				returnMsg.nwHeader.nhReqHandle = handle;
				returnMsg.nwHeader.nhStatus = status;
				returnMsg.nwFlags = req->nfyhdr.flags;
				if (req->nfy_query.qu_tree) {
					returnMsg.nwQueryLength = strlen(req->nfy_query.qu_expr);
				} else {
					returnMsg.nwQueryLength = 0;
				}
				
				memcpy(&returnMsg.nwSigevent, &req->nfy_sigevent, sizeof(sigevent_t));
				
				if (write(ci->sd, &returnMsg, sizeof(nfyNewRqMsg_t)) <= 0) {
					fprintf(stderr, "Failed response to client, socket maybe closed\n");
					closeClientSocket(ci, RM_REQUESTS);
				}
				/* If query is not a null query - send back the query string */
				if (returnMsg.nwQueryLength > 0) {
					if (write(ci->sd, req->nfy_query.qu_expr, returnMsg.nwQueryLength) <= 0) {
						fprintf(stderr, "Failed response to client, socket maybe closed\n");
						closeClientSocket(ci, RM_REQUESTS);
					}
				}
			}
			break;
		}
		case nmtRequestStatus:
			/* reserved */
			break;

		default:
			/* something really wrong here */
			return -1;
			break;
	}
	
	return 0;
}

/* 
 * FUNCTION	: registerRequest
 * 
 * PURPOSE	: create a new notify request object and add it into the list
 * 
 * ARGS		:	
 * 
 * RETURN	: returns the handle to that object if succeeded, otherwise
 *			  a negative error code
 * 
 */
HANDLE 
registerRequest(clientinfo_t *ci, nfyNewRqMsg_t * req_msg, char * queryStr,
		int query_flags)
{
	char errbuf[200];
	int status;
	int fd;
	nfyReq_t *nfyReq = NULL;
	

	nfyReq = (nfyReq_t *) malloc(sizeof(nfyReq_t));	
	if (!nfyReq) {
		perror("malloc:");
	 	return -ENOMEM;
	}
	
	TRACE("queryStr=%s\n", queryStr);
	if (!strcmp(queryStr, "<null>")) {
		TRACE("Init qu_tree to NULL\n");
	    nfyReq->nfy_query.qu_tree = NULL;
	    status =0;
	} else {
		TRACE("Create query object\n");
		status = posix_log_query_create(queryStr, query_flags,
				&nfyReq->nfy_query, errbuf, 200);
		/*
		status = posix_log_query_create(queryStr, POSIX_LOG_PRPS_NOTIFY,
					&nfyReq->nfy_query, errbuf, 200);
		*/
	}
	
	if (status == EINVAL) {
		fprintf(stderr, "query error: %s\n", errbuf);
		free(nfyReq);
		return -EINVAL;
	} else if (status != 0) {
		fprintf(stderr, "Internal error: errno %d.\n", status);
		free(nfyReq);
		return -EINVAL;
	}

	/* Validate the sigevent object */
	if (req_msg->nwSigevent.sigev_notify != SIGEV_SIGNAL) {
		fprintf(stderr, "sigev_notify is not SIGEV_SIGNAL.\n");
		if (nfyReq->nfy_query.qu_tree != NULL) {
			TRACE("Destroy query\n");
			posix_log_query_destroy(&nfyReq->nfy_query);
		}
		free(nfyReq);
		return -EINVAL;
	}
	memcpy(&nfyReq->nfy_sigevent, &req_msg->nwSigevent, sizeof(sigevent_t));

    TRACE("pid = %u\n", ci->pid);
	nfyReq->nfyhdr.handle = createNewHandle();
	nfyReq->nfyhdr.pid = ci->pid;
	nfyReq->procCmd = getProcessCmd(ci->pid);
	TRACE("procCmd = %s\n", nfyReq->procCmd);
	nfyReq->nfyhdr.flags = req_msg->nwFlags;
	nfyReq->nfyhdr.sd = ci->sd;

	pNfyReqList =_evlAppendToList(pNfyReqList, nfyReq);
    assert(pNfyReqList != NULL);
	/*
	 * Write this request to file
	 */
	appendReqToFile(nfyReq);
	
	return notifyReqHandle;
}

/* 
 * FUNCTION	: writeRequestStatus
 * 
 * PURPOSE	: send the status of an operation back to the client
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int 
writeRequestStatus(int sd, HANDLE handle, int status)
{
	nfyMsgHdr_t resp;
	
	resp.nhMsgType = nmtRequestStatus;
	resp.nhReqHandle = handle;
	resp.nhStatus = status;

	if (write(sd, &resp, sizeof(nfyMsgHdr_t)) <= 0) {
		fprintf(stderr, "Failed to reply to the client, socket maybe closed\n");
		return -1;
	}
	return 0;
}

/* 
 * FUNCTION	: findRequest
 * 
 * PURPOSE	: Iterate through the list and look for request with matching 
 * 		  HANDLE
 * 
 * ARGS		:	
 * 
 * RETURN	: returns the notify request object if it is found, NULL 
 *		  otherwise
 * 
 */
nfyReq_t * 
findRequest(HANDLE handle)
{
	nfyReq_t * req;
	evl_listnode_t *p = pNfyReqList->li_next; 		/* since the first is empty   */
													/* we start with the next one */

	if (pNfyReqList == p) {
		/* Point to itself- it is the first empty node, list is empty */
		return NULL;
	}

	do {
		req = (nfyReq_t *) p->li_data;
		if (req->nfyhdr.handle == handle) {
			return req;
		}
		p = p->li_next;
	} while (p != pNfyReqList);
	return NULL;
}

/* 
 * FUNCTION	: removeRequestWithHandle
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int 
removeRequestWithHandle(HANDLE handle)
{
	nfyReq_t *req;
	evl_listnode_t *p = pNfyReqList->li_next;		/* since the first is empty   */
													/* we start with the next one */
	evl_listnode_t *prev, *next;
	void (* freedata_fptr);

	freedata_fptr = freeNfyData;
	if (pNfyReqList == p) {
		/* Point to itself- it is the first empty node, list is empty */
		return -1;
	}

	do {
		req = (nfyReq_t *) p->li_data;
		next = p->li_next;
		if (req->nfyhdr.handle == handle) {
			removeNode(p, freedata_fptr);
			/*
			 * Keep track how many requests that are removed in the list 
	 		 * we will kick off the compact routine when it reaches certain
			 * limit
			 */
			list_dirty_ctr++;
			if(list_dirty_ctr >= MAX_LIST_DIRTY_COUNT) {
				/* save the list to file now */
				flushRequestList();
				list_dirty_ctr = 0;
			} 		
			return 0;
		}
		p = next;
	} while (p != pNfyReqList);
	return -1;	
}

/* 
 * FUNCTION	: removeRequestsWithSD
 * 
 * PURPOSE	: remove all requests when client with this socket descriptor goes away
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
void 
removeRequestsWithSD(int sd)
{
	nfyReq_t *req;
	evl_listnode_t *p = pNfyReqList->li_next;		/* since the first is empty */
													/* we start with the next one */
	evl_listnode_t *next;
	int remove;
	void (* freedata_fptr);

	freedata_fptr = freeNfyData;
	
	if (pNfyReqList == p) {
		/* Point to itself- it is the first empty node, list is empty */
		return;
	}
	
	do {
		
		req = (nfyReq_t *) p->li_data;
		next = p->li_next;
		if (req->nfyhdr.sd == sd) {
			TRACE("Remove request handle(%d)\n", req->nfyhdr.handle);
			//removeRequest(p);
			
			removeNode(p, freedata_fptr); 	
		} 
		p = next;
	} while(p != pNfyReqList);
}

/* 
 * FUNCTION	: freeNfyData
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
void 
freeNfyData(void *data)
{
	nfyReq_t *req;
	
	if (data) {
//		TRACE("Free data in the node.\n");
		req = (nfyReq_t *) data;
		if ( req->nfy_query.qu_tree) {
			posix_log_query_destroy(&req->nfy_query);
		}
		if(req->procCmd) {
//			TRACE("Free procCmd in the node.\n");
			free(req->procCmd);
		}
		free(req);
	}
	TRACE("List has %d nodes now\n",_evlGetListSize(pNfyReqList));
}

/* 
 * FUNCTION	: disableRequest
 * 
 * PURPOSE	: mark the request disable
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int 
disableRequest(HANDLE handle)
{
	nfyReq_t *req= findRequest(handle);
	if (!req) {
		return -1;
	}
	req->nfyhdr.flags |= POSIX_LOG_NFY_DISABLED;
	/*
	 * Keep track how many requests that are removed in the list 
 	 * we will kick off the compact routine when it reaches certain
	 * limit
	 */
	list_dirty_ctr++;
	if(list_dirty_ctr >= MAX_LIST_DIRTY_COUNT) {
		/* persist the list now */
		flushRequestList();
		list_dirty_ctr = 0;
	}
	return 0;
}

/* 
 * FUNCTION	: flushRequestList
 * 
 * PURPOSE	: write the notify req list to file
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int 
flushRequestList()
{
	TRACE("Flushing requests to file ...\n");
	
	removeZombieRequestsInList();	
	return writeRequestsToFile();
}

/* 
 * FUNCTION	: removeZombieRequestsInList
 * 
 * PURPOSE	: Remove notify requests if the process that requests
 *			  for a notification no longer exist
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
void 
removeZombieRequestsInList()
{
	evl_listnode_t *next, *p = pNfyReqList->li_next;	/* since the first is empty */
														/* we start with the next one */ 
	nfyReq_t *req;
	void (* freedata_fptr);

	freedata_fptr = freeNfyData;

	if (p == pNfyReqList) {
		/* Point to itself - it is the first empty node, list is empty */
		return;
	}
	do {
		req = (nfyReq_t *) p->li_data;
		next = p->li_next;
		if ((req->nfyhdr.flags & POSIX_LOG_NFY_PENDING_REMOVE) == POSIX_LOG_NFY_PENDING_REMOVE ) {
			TRACE("Remove pending-remove request handle(%d), %u\n", req->nfyhdr.handle, req->nfyhdr.flags);
			removeNode(p, freedata_fptr);
		}
		else if(validateProc(req->nfyhdr.pid, req->procCmd) == -1) {
			removeNode(p, freedata_fptr);
		}	
		p = next;
	} while (p != pNfyReqList);
	return;
}

/* 
 * FUNCTION	: createNewHandle
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
HANDLE 
createNewHandle()
{
	return ++notifyReqHandle;
#if 0
	static int overflow;
	++notifyReqHandle;
	if (notifyReqHandle >= 0xfffffffe) {		
		TRACE("notifyReqHandle is overflow\n");
		overflow = 1;
		notifyReqHandle = 1;
	}
	if (overflow == 1) {	
		while (findRequest(notifyReqHandle)) {
			notifyReqHandle++;
		}
	}	
	return notifyReqHandle;
#endif
}

/* 
 * FUNCTION	: setNotifyHandle
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
void 
setNotifyHandle(HANDLE h)
{
	notifyReqHandle = h;
}

/* 
 * FUNCTION	: getProcessCmd
 * 
 * PURPOSE	: get the program name for this pid, caller is responsible
 *			  for freeing the returned string.
 * 
 * ARGS		:	
 * 
 * RETURN	: returns a null terminated string if found, otherwise NULL
 * 
 */
char * 
getProcessCmd(pid_t pid)
{	
	char buf[MAX_PROGRAM_NAME + 1];
	char *cmd;
	char path[40];
	int fd;
	size_t	r;

	snprintf(path, sizeof(path), "/proc/%u/cmdline", pid);
	if((fd = open(path, O_RDONLY)) == -1) {
		return NULL;
	}
	
	if (( r = read(fd, buf, MAX_PROGRAM_NAME)) != -1) {
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



  
 

 	
