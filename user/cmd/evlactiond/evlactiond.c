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


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>    	
#include <fcntl.h>
#include <sys/klog.h>
#include <signal.h>           	
#include <errno.h>
#include <assert.h>
#include <grp.h>
#include <pwd.h>
#include <sys/wait.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_list.h"
#include "evl_common.h"
#include "evlactiond.h"
#include "evl_template.h"

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stdout, fmt, ##args)
#endif

#ifndef DEBUG2
#define TRACE(fmt, args...)		/* fprintf(stdout, fmt, ##args) */
#endif


#define max(a,b) (((a)>(b))?(a):(b))

#define LOGERROR(evt_type, fmt, args...)	posix_log_printf(LOG_LOGMGMT, evt_type, LOG_CRIT, 0, fmt, ##args)


#define MAX_CLIENTS				32
#define CLIENT_SLOT_AVAILABLE	-100

#define QUERY_FLAGS_NOT_SET	999

typedef struct clientinfo {
	int sd;
	uid_t uid;
} clientinfo_t;

/* Functions prototype */
void _daemonize();
static void SIGTERM_handler();
HANDLE notifyAdd(char *query_string, int query_flags, int notifyId, int flags);
int processNotifyCmd(clientinfo_t *ci);
void closeSocket(int sd);
static int getMaxClientIndex();
static void closeClientSocket(clientinfo_t *ci);
static void actions_handler(int signo, siginfo_t *info, void *context);
static void sigChild_handler();

int registerAction(nfy_action_hdr_t * act_hdr, char *querystr, char *cmd, 
		char *name, char *anystr);
int removeAction(int id, uid_t uid);
int modifyAction(int id, char * action_cmd, uid_t uid);
int loadActionsFromFile();
int writeActionToFile(int fd, nfy_action_t *act);
int refreshActionRegistry(void);
void listActions(int sd, uid_t uid);

void setNextNfyId(int id);
int getNextNfyId();
void freeActionData(void * data);
nfy_action_t * findAction(int id);
nfy_action_t * findActionByName(const char *name);
void execute(nfy_action_t * action, size_t recid);
int security_test();
int checkAllowDenyAccess(uid_t uid);
int setExecEnvironment();
char * expandRecId(char * cmdbuf, char *expandbuf, size_t recid);

void freeAnyNode(void *data);
void freeHistoryNode(void *data);
any_t * addNewAny(evl_listnode_t *any_list, const char *ident);
any_t * lookUpAnyIdent(evl_listnode_t *any_list, const char *ident);
void addNfyHistory(evl_listnode_t *hist, posix_log_recid_t recid);
void resetNfyHistoryList(evl_listnode_t *hist);
void buildHistoryWindow(evl_listnode_t *hist, unsigned interval);
int exceedThreshold(evl_listnode_t *hist, int thres);
void listActionByName(int sd, uid_t uid, const char *name);
char * getAnyIdentFromLog(int recid, const char * any_str, char *ident, size_t isize);
static int getRec(int recid, struct posix_log_entry *entry, char *buf, size_t size);

/* Global variables */
static evl_listnode_t *pActionList;			/* List of registered actions */
static int maxsd = 0;                       /* Largest socket descriptor used by this server */
fd_set all_sds;
static int next_nfy_id = 0;

static clientinfo_t clients[MAX_CLIENTS];
static int maxcl;
static int maxci = 0;
static char *PidFile = "/var/run/evlactiond.pid";

static char *logPath = LOG_CURLOG_PATH;

int 
main(int argc, char *argv[]) 
{
	auto int c;
	int listen_sd;					/* Unix Domain (UD) socket descriptor for local clients */
	struct sockaddr_un listen_sock;	/* Generic UD socket address */
	fd_set read_sds;
	int newsd;                      /* New socket descriptor */

	static struct sigaction SigTERMAction;  				/* Signal handler to terminate gracefully */
	static struct sigaction act;
	void SIGTERM_handler();
	int daemonize = 1;
	struct sigevent reg_nfy_sigevent, ret_nfy_sigevent;		/* Hold the register sigevent
															 * when calling posix_log_notify_add
															 * and the return sigevent from 
															 * posix_log_notify_get
															 */
	static struct sigaction SigRTAction; 
	static struct sigaction SigChild; 	  				/* Signal handler SIGCHLD */
	void actions_handler();
	void sigChild_handler();
	int i;
	pid_t pid;
	

	while((c=getopt(argc, argv, "f")) != EOF) {
		switch (c) {
			case 'f':
				daemonize = 0;
			break;     
			default:     
			break;
		}
	}

	/* Daemonize */
	if (daemonize) {
		_daemonize();
	}
	/* Init clients array */
	for (i = 0; i < MAX_CLIENTS; i++) {
		clients[i].sd = CLIENT_SLOT_AVAILABLE;
	}
	/* Reuse template once it is created */
	evltemplate_initmgr(TMPL_REUSE1CLONE);

	/*
   	 * Create the list with the first node is empty (NULL).
   	 * This first node will always exist from now on.
   	 */
   	pActionList =_evlMkListNode(NULL);

   	/* read the action registry file */
	TRACE("Loading action registry.. \n");
   	if (loadActionsFromFile() != 0) {
   		fprintf(stderr, "Failed to load action registry.\n");
   	}

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
/*
 * sigChild_handler can't handle a high rate of death childs, that leaves zombie
 * processes around -  if 0 the code below
 *
 * Just ignore SIGCHLD would fix this problem
 */
#if 0
	/*
	 * Create signal handler to handle SIGCHILD - 
	 */
	(void)memset(&SigChild, 0, sizeof(struct sigaction));
	SigChild.sa_handler =sigChild_handler;
	SigChild.sa_flags = 0;
	if (sigaction(SIGCHLD, &SigChild, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGCHLD.\n", argv[0]);
		perror("sigaction");
		exit(1); 
	}
#endif
    /* Ignore SIGCHLD */	
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
	 * the server may go away, we want to reconnect to the server if possible.
	 */
	if (sigaction(SIGPIPE, &act, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGPIPE.\n", argv[0]);
		perror("sigaction");
		exit(1);
	}

	/*
	 * Setup notification signal handler
	 */

	(void) memset(&SigRTAction, 0, sizeof(SigRTAction));
	SigRTAction.sa_handler = (void *) actions_handler;
	SigRTAction.sa_flags = SA_SIGINFO;			/* This flag is set so that we 
												 * get the siginfo pass back
												 * to the signal handler
												 */
	if (sigaction(SIGRTMIN+1, &SigRTAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGRTMIN.\n", argv[0]);
		perror("sigaction");
	}

	(void) unlink(EVLACTIOND_SERVER_SOCKET);

   	listen_sd = _evlCreateListenSocket(&listen_sock, EVLACTIOND_SERVER_SOCKET, MAX_CLIENTS);
    	
   	FD_ZERO(&all_sds);
   	FD_SET(listen_sd, &all_sds);
   	maxsd = listen_sd;
	TRACE("listen_sd=%i\n", listen_sd);

   	for(;;) {
   		int sd;
   		int nsel;
   		struct ucred ucred;							/* notification       */
   		socklen_t ucredsz = sizeof(struct ucred);	/* clients credential */
		sigset_t oldset, newset;
		int status;
		int sigIsBlocked;
		
		bcopy((char *)&all_sds, (char *)&read_sds, sizeof(all_sds));
        TRACE("Waiting for an event..blocking on select call\n");
		if ((nsel = select(maxsd + 1, &read_sds, 0, 0, 0)) < 0) {
			if(errno == EINTR ) {
				/*
		 		* I got an interrupt here every time I got a rt-signal
		 		* Just ignore it here.
		 		*/
		 		TRACE("got EINTR\n");
				continue;
			} else {
				perror("select");
				exit(1);
			}
		}
            		
		/*
		 * Temporary block notification signal
 		 * during the time I have socket activity.
		 * For example: User is executing evlnotify
		 */
		status = sigaddset(&newset, SIGRTMIN+1);
		status = sigaddset(&newset, SIGCHLD);	
        status = sigprocmask(SIG_BLOCK, &newset, &oldset);
    	if (status != 0) {
    		perror("sigprocmask");
    		/* Fail to block, but probably the best course is just to continue */
    		sigIsBlocked = 0;
    	} else {
    		sigIsBlocked = 1;
    	}
    	TRACE("Something to read came ...\n");
   		if (FD_ISSET(listen_sd, &read_sds)) {
   			/* Client is trying to connect. */
   			int newsd;
			char abyte;	
			clientinfo_t *cl, *empty;
			int idx;
    		
   			if ((newsd = accept(listen_sd, NULL, NULL)) < 0) {
   				perror("accept");
   				goto exit_new_sd;
   			}
            TRACE("****************** New sd=%u  *************************\n", newsd);
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
   				 * First check if the action.allow file exist, if it is
   				 * then check if user is allowed. If the action.allow
   				 * does not exists but there is action.deny file, check
   				 * user is denied access.
   				 */
   				if (checkAllowDenyAccess(ucred.uid) != 0) {
   				 	abyte = NFY_ACCESS_DENIED;
   					write(newsd, &abyte, sizeof(char));
   					TRACE("Failed user allow deny check.\n");
#if 0
   					LOGERROR(EVLOG_ACCESS_DENIED,"evlactiond: User %u failed user credential check.", ucred.uid);
#endif
   					close(newsd);
   					goto exit_new_sd;
   				}
   				
   				/* then check to see if user has access to the log file */
   				if (_evlVerifyUserCredential(ucred.uid, ucred.gid, security_test) != 0) {
   					abyte = NFY_ACCESS_DENIED;
   					write(newsd, &abyte, sizeof(char));
   					TRACE("Failed user credential check.\n");
#if 0
   					LOGERROR(EVLOG_ACCESS_DENIED,"evlactiond: User %u failed user credential check.", ucred.uid);
#endif
   					close(newsd);
   					goto exit_new_sd;
   				}
   				abyte = NFY_ACCESS_GRANTED;
   				write(newsd, &abyte, sizeof(char));
   				
				empty->sd = newsd;
				empty->uid = ucred.uid;

				FD_SET(newsd, &all_sds);
   			
				if (newsd > maxsd) {
					 maxsd = newsd;
				}
   			}
            exit_new_sd:  /* make gcc happy */ ;
   		}
   		
   		/* prosess clients */
		for (i= 0; i <= maxci ; i++) {    	
			if (clients[i].sd != CLIENT_SLOT_AVAILABLE) {
				TRACE("Process clients %i, maxci=%i, maxsd=%i\n", i, maxci, maxsd);
				if (FD_ISSET(clients[i].sd, &read_sds)) {
					processNotifyCmd(&clients[i]);
				}
			}
		}

   		/* unblock signal */
   		if (sigIsBlocked == 1) {
			status = sigprocmask(SIG_SETMASK, &oldset, NULL);
			if (status != 0) {
				perror("sigprocmask (unblock)");
			}
		}
  	}/* end of for */

}

/* 
 * FUNCTION	: _daemonize
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
 void 
 _daemonize()
 {
 	pid_t pid;
 	int num_fds, i;

	if (_evlValidate_pid(PidFile)) {
		fprintf(stderr, "evlactiond: Already running.\n");
		exit(1);
	}
	/*
	 * Fork a child and let the parent exit. This guarentees that
	 * the first child is not a process group leader.
	 */
	if ((pid = fork()) < 0) {
		fprintf(stderr, 
				"evlactiond: Cannot fork child process. Check system process usage.\n"); 
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
				"evlactiond: Cannot fork child process. Check system process usage.\n"); 
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
		LOGERROR(EVLOG_WRITE_PID, "evlactiond: Cannot write 'evlactiond' PID to '%s' file\n", 
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
 * FUNCTION	: closeSocket
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
	close(sd);
	FD_CLR(sd, &all_sds);
	if(sd == maxsd) {
		maxsd--;
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
	
	FD_CLR(ci->sd, &all_sds);
	(void) close(ci->sd);
	ci->sd = CLIENT_SLOT_AVAILABLE;
	
	TRACE("closeClientSocket:sd = %d maxsd = %d\n", tbclosedsd, maxsd);
	
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
	 * connects to evlactiond, and the to be closed sd = maxsd then
	 * maxsd shoulde be maxsd-1
	 */
	if (new_maxsd == 0 && tbclosedsd == maxsd) {
		maxsd--;
	} else if (new_maxsd != 0) {
		maxsd = new_maxsd;
	}
	
	TRACE("Done closeClientSocket: maxsd = %d\n", maxsd);
	return;
}

static void sigChild_handler()
{
	int status;
	TRACE("Got a sigchild!\n");
	waitpid(0, &status, WNOHANG | WUNTRACED);
}

/*
 * FUNCTION	: SIGTERM_handler
 *
 * PURPOSE	: Save the action registry, clean up before exit
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
static void
SIGTERM_handler()
{
	void (* freedata_fptr);
	(void) unlink(PidFile);
	refreshActionRegistry();
	TRACE("Done refreshActionRegisstry\n");
   	freedata_fptr = freeActionData;
   	removeAllNodes(pActionList, freedata_fptr);
   	free(pActionList);
	exit(0);
}
/* 
 * FUNCTION	: processNotifyCmd
 * 
 * PURPOSE	: processes the client cmd (add, remove, modify, list)
 * 
 * ARGS		:	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */ 
int 
processNotifyCmd(clientinfo_t *ci)
{
	nfy_action_t *act;
	nfy_action_hdr_t *act_hdr;
	char buf[sizeof(nfy_action_hdr_t)];
	int n, status;
	char *qustr, *action_cmd, *action_name, *any_str;

	/* read the message header */
	if (_evlReadEx(ci->sd, buf, sizeof(nfy_action_hdr_t)) == -1) {
		closeClientSocket(ci);
		return -1;
	}
	act_hdr = (nfy_action_hdr_t *) buf;

	switch (act_hdr->cmd_type) {
		case nfyCmdAdd:
		{
			if ((qustr = malloc(act_hdr->qu_strLen + 1)) == NULL ) {
				closeClientSocket(ci);
				return -1;
			}  
			if ((action_cmd = malloc(act_hdr->action_cmdLen + 1)) == NULL ) {
				closeClientSocket(ci);
				free(qustr);
				return -1;
			}
			if ((action_name = malloc(act_hdr->name_len + 1)) == NULL) {
				closeClientSocket(ci);
				free(qustr);
				free(action_cmd);
				return -1;
			}
			if ((any_str = malloc(act_hdr->any_str_len + 1)) == NULL) {
				closeClientSocket(ci);
				free(qustr);
				free(action_cmd);
				free(action_name);
				return -1;
			}
			if ((n = _evlReadEx(ci->sd, qustr, act_hdr->qu_strLen)) == -1) {
				closeClientSocket(ci);
				free(qustr);
				free(action_cmd);
				free(action_name);
				free(any_str);
				return -1;
			}
			qustr[n] = '\0';
		
			if ((n = _evlReadEx(ci->sd, action_cmd, act_hdr->action_cmdLen)) == -1) {
				closeClientSocket(ci);
				free(qustr);
				free(action_cmd);
				free(action_name);
				free(any_str);
				return -1;
			}
			action_cmd[n] = '\0';
			
			if ((n = _evlReadEx(ci->sd, action_name, act_hdr->name_len)) == -1) {
				closeClientSocket(ci);
				free(qustr);
				free(action_cmd);
				free(action_name);
				free(any_str);
				return -1;
			}
			action_name[n] = '\0';

			if ((n = _evlReadEx(ci->sd, any_str, act_hdr->any_str_len)) == -1) {
				closeClientSocket(ci);
				free(qustr);
				free(action_cmd);
				free(action_name);
				free(any_str);
				return -1;
			}
			any_str[n] = '\0';
			/*
			 * If success status should contain the notify id, otherwise
			 * a negative number. The qustr and action_cmd will be freed
			 * with removeNode.
			 */
			status = registerAction(act_hdr, qustr, action_cmd, 
					action_name, any_str); 
			
			write(ci->sd, &status, sizeof(int));
			
			break;
		}
		case nfyCmdRemove:
		{
		  	status = removeAction(act_hdr->nfy_id, ci->uid);
		  	write(ci->sd, &status, sizeof(int));
			
			break;
		}
		case nfyCmdChange:
		{
			
			if ((action_cmd = malloc(act_hdr->action_cmdLen + 1)) == NULL ) {
				closeClientSocket(ci);
				return -1;
			}
			if ((n = _evlReadEx(ci->sd, action_cmd, act_hdr->action_cmdLen)) == -1) {
				closeClientSocket(ci);
				free(action_cmd);
				return -1;
			}
			action_cmd[n] = '\0';
			status = modifyAction(act_hdr->nfy_id, action_cmd, ci->uid);
			write(ci->sd, &status, sizeof(int));
			
			if (action_cmd)
				free(action_cmd);
			break;
		}
		case nfyCmdListAll:
		{
			listActions(ci->sd, ci->uid);
			
			break;
		}
		case nfyListByName:
		{
			if ((action_name = malloc(act_hdr->name_len + 1)) == NULL ) {
				closeClientSocket(ci);
				return -1;
			}
			if ((n = _evlReadEx(ci->sd, action_name, act_hdr->name_len)) == -1) {
				closeClientSocket(ci);
				free(action_name);
				return -1;
			}
			action_name[n] = '\0';
			
			listActionByName(ci->sd, ci->uid, action_name);
			break;
		}
		
		default:
		break;
	}
	
	return 0;
}

/* 
 * FUNCTION	: registerAction
 * 
 * PURPOSE	: request a notify request to evlnotifyd  and create the action
 *			  object, also add the action object tothe list
 * 
 * ARGS		:	
 * 
 * RETURN	: return the action id if succeeded, otherwise -1
 * 
 */
int 
registerAction(nfy_action_hdr_t * act_hdr, char *querystr, char *cmd, 
		char *name, char *any_str)
{
	nfy_action_t * action = NULL;
	HANDLE handle;
	int id = getNextNfyId();

	TRACE("queryStr=%s\n", querystr);

	if((handle = notifyAdd(querystr, act_hdr->query_flags, id, act_hdr->flags)) == -1) {
		setNextNfyId(--id);		/* dont want to waste an id */
		free(querystr);
		free(cmd);
		return -1;
	}
	
	action = (nfy_action_t *) malloc(sizeof(nfy_action_t));
	if (action == NULL) {
		perror("malloc");
		free(querystr);
		free(cmd);
		return -1;
	}
	
	action->handle = handle;
	act_hdr->nfy_id = id;

	/* 
	 * If the threshold and interval are set, we need
	 * to intialize the any list (the any list is a list of 
	 * unique device, we maintain a seperate count for each
	 * device)
	 */
	if (act_hdr->thres > 0 && act_hdr->interval > 0) {
		action->any_list = _evlMkListNode(NULL);
	} else {
		action->any_list = NULL;
	}

	memcpy(&action->hdr, act_hdr, sizeof(nfy_action_hdr_t));
	action->qu_str = querystr;
	action->cmd = cmd;
	action->name = name;
	action->any_str = any_str;
	pActionList =_evlAppendToList(pActionList, action);
	assert(pActionList != NULL);
	if (action->hdr.flags & POSIX_LOG_ACTION_PERSIST) {
		refreshActionRegistry();
	}

	TRACE("registerAction succeeded\n");
	return id;
}

/*
 * FUNCTION	: removeAction
 *
 * PURPOSE	: remove an action object off the list
 *
 * ARGS		:	
 *
 * RETURN	: return 0 if succeeded, otherwise -1
 *
 */
int
removeAction(int id, uid_t uid)
{
	nfy_action_t *act;
	
	evl_listnode_t *p = pActionList->li_next;	/* since the first is empty   */
							/* we start with the next one */
	evl_listnode_t *prev, *next;
	void (* freedata_fptr);
	int refreshActReg=0;
	static list_dirty_ctr;

	freedata_fptr = freeActionData;
	if (pActionList == p) {
		/* Point to itself - it is the first empty node, list is empty */
		return -1;
	}

	do {
		act = (nfy_action_t *) p->li_data;
		next = p->li_next;
		if (act->hdr.nfy_id == id && (act->hdr.uid == uid || uid == 0)) {
			if(act->hdr.flags & POSIX_LOG_ACTION_PERSIST) {
				refreshActReg = 1;
			}
			removeNode(p, freedata_fptr);
			
			/*
			 * For performance reason, we delay writing the change in the list
			 * until it is really dirty :-)
			 */
			list_dirty_ctr++;
			if(refreshActReg && list_dirty_ctr >= 20 ) {
				refreshActionRegistry();
				list_dirty_ctr=0;
			}

			return 0;
		}
		p = next;
	} while (p != pActionList);
	return -1;	
}

/*
 * FUNCTION	: modifyAction
 *
 * PURPOSE	: modify the action cmd in the action object
 *
 * ARGS		:	
 *
 * RETURN	: return 0 if succeeded, otherwise -1
 *
 */
int
modifyAction(int id, char * action_cmd, uid_t uid)
{
	nfy_action_t *act;
	
	if ((act = findAction(id)) == NULL) {
		return -1;
	}
 	if (act->hdr.uid != uid && uid != 0) {
 		return -1;
 	}
 	act->cmd = realloc(act->cmd, strlen(action_cmd) + 1);

	strcpy(act->cmd, action_cmd);
	if(act->hdr.flags & POSIX_LOG_ACTION_PERSIST) {
		refreshActionRegistry();
	}

	return 0;
}
/*
 * FUNCTION	: refreshActionRegistry
 *
 * PURPOSE	: write action list to actionregistry
 *
 * ARGS		:	
 *
 * RETURN	: return 0 if succeeded, otherwise -1
 *
 */
int
refreshActionRegistry(void)
{
	int fd;
	nfy_action_t *act;
	evl_listnode_t *p;
	char tempfile[PATH_MAX];
	sigset_t oldset;
	int sigsBlocked;

	/*
	 * Block signals while we do this.
	 */

	sigsBlocked = (_evlBlockSignals(&oldset) == 0);

	/*
	 * Create a temporary file.
	 */

	(void) sprintf(tempfile, "%s_XXXXXX", EVLACTIOND_REG_FILE);

	fd = open(tempfile, O_CREAT|O_TRUNC|O_RDWR, S_IRWXU);
	if (fd < 0) {
		perror("create temporary");
		return -1;
	}

	/* since the first is empty we start with the next one */

	p = pActionList->li_next;

	/*
	 * Write out the list of persistent actions (if any).
	 */

	while (p != pActionList) {
		act = (nfy_action_t *) p->li_data;

		if (act->hdr.flags & POSIX_LOG_ACTION_PERSIST) {
    			if (writeActionToFile(fd, act) == -1) {
				(void) close(fd);
   				return -1;
			}
		}

		p = p->li_next;
	}

	(void) close(fd);

	/*
	 * Attempt to update the file via atomic rename of the temporary
	 * file to the target file name.
	 */

	if (rename(tempfile, EVLACTIOND_REG_FILE) == -1) {
		perror("rename temp to target");
		return -1;
	}

	/*
	 * Unblock signals.
	 */

	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}

	/*
	 * Success!
	 */

	return 0;
}

/*
 * Send action data to evlnotify command
 */ 
int
writeAction(int sd, nfy_action_t *act)
{
	/* write header */
	act->hdr.qu_strLen = strlen(act->qu_str);
	act->hdr.action_cmdLen = strlen(act->cmd);
	act->hdr.name_len = strlen(act->name);
	act->hdr.any_str_len = strlen(act->any_str);
	if (write(sd, &act->hdr, sizeof(nfy_action_hdr_t)) != sizeof(nfy_action_hdr_t)) {
		return -1;
	}
	/* write filter */
	if (write(sd, act->qu_str, strlen(act->qu_str)) != strlen(act->qu_str)) {
		return -1;
	}
	/* write cmd */
	if (write(sd, act->cmd, strlen(act->cmd)) != strlen(act->cmd)) {
		return -1;
	}	
	TRACE("Write -name=%s\n", act->name);
	/* write name */
	if (write(sd, act->name, strlen(act->name)) != strlen(act->name)) {
		return -1;
	}
	if (write(sd, act->any_str, strlen(act->any_str)) != strlen(act->any_str)) {
		return -1;
	}
	return 0;
}	
/*
 * FUNCTION	: listActions
 *
 * PURPOSE	: writing back the action(s) one by one over the wire to client
 *			  evlnotify -l
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
void
listActions(int sd, uid_t uid)
{
	int fd, ret = 0;
	nfy_action_hdr_t hdr;
	nfy_action_t *act;
	
	evl_listnode_t *p = pActionList->li_next;		/* since the first is empty   */
													/* we start with the next one */
	hdr.nfy_id = 0 ;		/* tell client we done */
	hdr.flags  = 0 ;		/* 0 = OK , -1 = failed */ 													
													
	
	if (pActionList == p) {
		/* Point to itself - it is the first empty node, list is empty */
		/* Tell the client that nothing to read */
		hdr.flags  = 0 ;
		write(sd, &hdr, sizeof(nfy_action_hdr_t));
		return;
	}
	
	do {
		act = (nfy_action_t *) p->li_data;
		if (act->hdr.uid == uid || uid == 0) {
			if (writeAction(sd, act) != 0) {
				hdr.flags  = -1 ;
				goto doneList;
			}
			
		}	
		p = p->li_next;
	} while (p != pActionList);
	
doneList:
	/* Tell the client that we done */
	write(sd, &hdr, sizeof(nfy_action_hdr_t));
	return;
}
	
void 
listActionByName(int sd, uid_t uid, const char *name)
{
	int fd, ret = 0;
	nfy_action_hdr_t hdr;
	nfy_action_t *act;

	act = (nfy_action_t *) findActionByName(name);

	if (act == NULL || (act->hdr.uid != uid && uid != 0)) {
		hdr.flags  = -1 ;
		write(sd, &hdr, sizeof(nfy_action_hdr_t));
		return;
	}
	if (writeAction(sd, act) != 0) {
		return;
	} 
	return;	
}
/*
 * FUNCTION	: writeActionToFile
 *
 * PURPOSE	:
 *
 * ARGS		:	
 *
 * RETURN	: return 0 if succeeded, otherwise -1
 *
 */
 int
 writeActionToFile(int fd,  nfy_action_t *act)
 {
 	char line[SMALL_BUF_SIZE];	
	
	snprintf(line, sizeof(line),
		"%u:%s:%s:%u:%u:%u:%u:%u:%s:%s\n", 
		act->hdr.nfy_id, 
		act->qu_str, 
		act->cmd,
		act->hdr.uid,
		act->hdr.gid,
		act->hdr.flags & POSIX_LOG_ONCE_ONLY,
	//	act->hdr.query_flags, 
		act->hdr.thres,
		act->hdr.interval,
		act->name,
		act->any_str);
	if (write(fd, line, strlen(line)) == -1) {
		perror("write");
#if 0  	
		LOGERROR(EVLOG_WRITE_REQF_FAILED, "evlactiond: Failed to write action into action registry");
#endif
		return -1;
	}
  	return 0;
 }

/* 
 * FUNCTION	: getNextNfyId
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int 
getNextNfyId() 
{
	return ++next_nfy_id;
}

/* 
 * FUNCTION	: setNextNfyId
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
void 
setNextNfyId(int id) 
{
	next_nfy_id = id;
}

/* 
 * FUNCTION	: security_test
 * 
 * PURPOSE	:  This function tests to see if user can open the log file.
 *			   To effectively do this security test user need to pass the uid,
 *			   gid, the pointer of this function to the verifyUserCredential.
 *			   The verifyUserCredential function switches to the appropriate uid
 *			   gid then calls this function, when it is done the verifyUserCredential
 *			   will switch its identity back to previous ID
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int security_test()
{
	int fd;
	
	if (geteuid() == 0) {
		/* he is root - let him go */
		return 0;
	}
	
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
 * FUNCTION	: checkAllowDenyAccess
 *
 * PURPOSE	: check the .allow and .deny for user access privilege
 *
 * ARGS		:	
 *
 * RETURN	: return 0 if user is clear, other wise -1
 *
 */
int
checkAllowDenyAccess(uid_t uid) 
{
	FILE *f;
	char username[25];
	struct passwd *pw;
	int	fd;
	
	if (uid == 0) {
		/* he is root - let him go */
		return 0;
	}
	
	if ((pw = getpwuid(uid)) == NULL) {
#if 0
		LOGERROR(EVLOG_PWFILE_LOOKUP_FAILED, "Failed to look up the user name");
#endif
    	fprintf(stderr, "Failed to look up the user name\n");
    	return -1;
    }
	/*
	 * If the actions.allow exists then verify that user
	 * is on the list
	 */

	if ((fd = open(ACTION_DOT_ALLOW, O_RDONLY)) != -1 ) {
		if ((f = fdopen(fd, "r")) == NULL) {
			perror("fdopen");
			close(fd);
			return -1;
		}
		fcntl(fd, F_RDLCK);
	
		while (fgets(username, 25, f) != NULL) {
			if (username[0] == '#' || username[0] == '\n') {
				continue;
			}
			if (username[strlen(username) -1] == '\n') {
				username[strlen(username) -1] = '\0';
			}
			if(!strcmp(username, pw->pw_name)) {
				fclose(f); /* closes fd; releases lock */
				return 0;
			}
		}
		fclose(f);	/* closes fd; releases lock */
		return -1;
	}

	/*
	 * If the actions.deny exists then verify that user
	 * is not on the list
	 */
	if (( fd = open(ACTION_DOT_DENY, O_RDONLY)) != -1) {
		if((f = fdopen(fd, "r")) == NULL) {
			perror("fdopen");
			close(fd);
			return -1;
		}
		(void) fcntl(fd, F_RDLCK);
		while(fgets(username, 25, f) != NULL) {
			
			if (username[0] == '#' || username[0] == '\n') {
				continue;
			}
			if (username[strlen(username) -1] == '\n') {
				username[strlen(username) -1] = '\0';
			}
			if(!strcmp(username, pw->pw_name)) {
				fclose(f);	/* closes fd; releases lock */
				return -1;
			}
		}
		fclose(f);	/* closes fd; releases lock */
	}
	return 0;
}



/* 
 * FUNCTION	: actions_handler
 * 
 * PURPOSE	: handle the signal from the evlnotifyd, retrieve the action id
 *			  find the action in the list and execute that action on behalf
 *			  of user.
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
static void 
actions_handler(int signo, siginfo_t *info, void *context) 
{
	posix_log_recid_t recid;
	char recid_str[20];
	nfy_action_t *act;
	any_t *any;
	struct posix_log_entry entry;
	char buf[POSIX_LOG_ENTRY_MAXLEN];
	char ident[256];
	int status;
	
	int nfyId = info->_sifields._rt.si_sigval.sival_int;

	posix_log_siginfo_getrecid(info, context, &recid);
	TRACE("\n\n****************** actions_handler activated  ****************\n");
	TRACE("sig_no=%d, recid=%d, nfy_id=%d\n", signo, recid, nfyId );
	
	act = findAction(nfyId);
	if (act == NULL) {
		return;
	}
	if (act->hdr.thres > 0 && act->hdr.interval > 0) {
		/*
		 * The threshold and interval are set - 
		 * We need to check the any_str if it is not
		 * empty then we need more information from the 
		 * log.
		 */
		if (strcmp(act->any_str, "no_any_str") == 0) {
			any = lookUpAnyIdent(act->any_list, "no_any_str");
			if (any == NULL) {
				any = addNewAny(act->any_list, "no_any_str");
			}
			TRACE("addNfyHistory...\n");
			addNfyHistory(any->hist, recid);
			TRACE("no_any_str-Buildwindow..\n");
			buildHistoryWindow(any->hist, act->hdr.interval);
			
		}
		else {	
			status = getRec(recid, &entry, buf, sizeof(buf));
			if ( status != 0) {
				return;
			}
			TRACE("act->any_str=%s\n", act->any_str);
			if( getAnyIdentFromLog(recid, act->any_str, ident, sizeof(ident)) == NULL) {
				return;
			}
			any = lookUpAnyIdent(act->any_list, ident);
			if (any == NULL) {
				TRACE("New ident - add %s to any_list\n", ident);
				any = addNewAny(act->any_list, ident);
			}
			
			TRACE("%s-addNfyHistory...\n", ident);
			addNfyHistory(any->hist, recid);
			TRACE("%s-Buildwindow..\n", ident);
			
			buildHistoryWindow(any->hist, act->hdr.interval);
		}
		
		if (exceedThreshold(any->hist, act->hdr.thres)) {
			/* Reset history */
			resetNfyHistoryList(any->hist);
			TRACE("execute..\n");
			execute(act, recid);
		}
	}
	else {
		execute(act, recid);
	}
	return;
}
			
/* 
 * FUNCTION	: execute
 * 
 * PURPOSE	: spawn a child to execute the action
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
void 
execute(nfy_action_t * action, size_t recid)
{
	pid_t pid;
	sigset_t mask, savemask;
	struct sigaction sa, ignore, istat, qstat;
	
/*
 * if 0 these, the call will handle the sigchild
 * when the child exit.
 */
#if 0
	/*
	 * Block SIGCHLD 
	 */
	(void)sigemptyset(&mask);
	(void)sigaddset(&mask, SIGCHLD);
	(void)sigprocmask(SIG_BLOCK, &mask, &savemask);
#endif	
	/*
	 * Ignore SIGINT and SIGQUIT.
	 */
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	(void)sigaction(SIGINT, &sa, &istat);
	(void)sigaction(SIGQUIT, &sa, &qstat);


	/* Create a child process - */ 
	if (( pid = fork()) < 0) {
		perror("fork");
		return;
	}
   	/* The parent return here, the child will go out and do its task */
	/* The child process */
	if (pid == 0) {
		int status;
		char exeCmd[SMALL_BUF_SIZE];
		struct passwd *pw;

		TRACE("Child process for nfyID=%d started\n", action->hdr.nfy_id);
		
		/*
		 * Restore previous handler for SIGINT and SIGQUIT
		 */
	 	sigaction(SIGINT, &istat, 0);
		sigaction(SIGQUIT, &qstat, 0);
		sigprocmask(SIG_SETMASK, &savemask, 0);


		if (setgid(action->hdr.gid) == -1) {
#if 0
			LOGERROR(EVLOG_GID_OP_FAILED, "evlactiond: setegid failed errno=%d", errno);
#endif
			fprintf(stderr, "Failed to change group ID to %d.\n", action->hdr.gid);
		}
		
		pw = getpwuid(action->hdr.uid);
		if (pw == NULL) {
			_exit(1);
		}
		if (initgroups(pw->pw_name, action->hdr.gid) == -1) {
			perror("initgroups");
			_exit(1);
		}
		
		if (setuid(action->hdr.uid) == -1) {
#if 0
			LOGERROR(EVLOG_UID_OP_FAILED, "evlactiond: seteuid failed errno=%d", errno);
#endif
			fprintf(stderr, "Failed to change user ID to %d.\n", action->hdr.uid);
			_exit(1);
		}
			
		TRACE ("uid=%u gid=%u euid=%u egid=%u\n", getuid(), getgid(), geteuid(), getegid());

		/* Setting user environment variable */
		setExecEnvironment();
		
		expandRecId(action->cmd, exeCmd, recid);
		
		/* Execute command */
		
		execl("/bin/sh", "sh", "-c", exeCmd, 0);
		/* It should never get here if execl goes well */
		perror("execl");
		_exit(127);
	} else {

	/* Restore signal handlers */
	sigaction(SIGINT, &istat, 0);
	sigaction(SIGQUIT, &qstat, 0);
	sigprocmask(SIG_SETMASK, &savemask, 0);
	}
	return;
}

/* 
 * FUNCTION	: setExecEnvironment
 * 
 * PURPOSE	: seting the apporpriate executed environment
 * 
 * ARGS		:	
 * 
 * RETURN	: return 0 if succeeded, otherwise -1
 * 
 */
int 
setExecEnvironment()
{
	char *pwd, *tmp;
	char line[SMALL_BUF_SIZE];
	FILE *f;
	
	if((f = fopen(EVLACTIOND_CONF_FILE, "r")) == NULL) {
		perror(EVLACTIOND_CONF_FILE);
		return -1;
	}	

	while (fgets(line, SMALL_BUF_SIZE, f) != NULL) {
		if (line [0] == '#' || line[0]=='\n' || line[0]==' ' || line[0]=='\t') {
			continue;
		} 
		if (strchr(line,'=') == NULL) {
			continue;
		}
		/* replace newline char with null */
		if (line[strlen(line) -1] == '\n') {
			line[strlen(line) -1] = '\0';
		}
		TRACE("%s\n", line);
		if(putenv(line) != 0) {
			perror("putenv");
		}
	}
	if((pwd = getenv("PWD")) == NULL) {
		(void)chdir("/tmp");
	} else {
		TRACE("PWD=%s. len=%d\n", pwd, strlen(pwd));
		if(chdir(pwd)!=0) {
			perror("chdir");
			fclose(f);
			return -1;
		}
	}
	fclose(f);
	return 0;
}

/* 
 * FUNCTION	: loadActionsFromFile
 * 
 * PURPOSE	: populate the action list with action object in the registry
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int 
loadActionsFromFile()
{
 	char line[SMALL_BUF_SIZE];
	char *ptr;
	int fd;
	FILE *f;
	nfy_action_t *action;
	int flags = 0, ret = 0;
	sigset_t oldset;
	int sigsBlocked;
	struct group *grp;
	struct passwd *pw;
	evl_listnode_t *p;
	
	fd = open(EVLACTIOND_REG_FILE, O_RDONLY|O_CREAT);
	if ( fd < 0) {
		perror(EVLACTIOND_REG_FILE);
		return -1;
	}
	if((f = fdopen(fd, "r")) == NULL) {
		perror("fdopen");
		close(fd);
		return -1;
	}

	sigsBlocked = (_evlBlockSignals(&oldset) == 0);
	
	while (fgets(line, SMALL_BUF_SIZE, f) != NULL) {
		char *s;
		
		/* skip line start with # or blank line */
		if(line[0] == '#' || line[0] == '\n') {
			continue;
		}
		
		ptr = (char *) strtok(line,":");
		
		action = (nfy_action_t *) malloc(sizeof(nfy_action_t));
		if(action == NULL) {
			fprintf(stderr, "malloc failed\n");
			ret = -1;
			goto exit_loadaction;
		}
		action->hdr.nfy_id = strtoul(ptr, &s, 10);
		if (ptr == s) {
			/* it is not what we intend to read */
			free(action);
			continue;
		}
		
		setNextNfyId(action->hdr.nfy_id);
			
		ptr = (char *) strtok(NULL, ":");
		action->qu_str = strdup(ptr);
		if (strlen(ptr) == 0) {
			/* bad  */
			free(action);
			continue;
		}
		
		ptr = (char *) strtok(NULL, ":");
		action->cmd = strdup(ptr);
		if (strlen(ptr) == 0) {
			/* bad  */
			free(action);
			continue;
		}
		
		ptr = (char *) strtok(NULL, ":");	
		action->hdr.uid = strtoul(ptr, &s, 10);
		if (ptr == s) {
			/* it is not what we intend to read, process next line */
			free(action);
			continue;
		}
		
		if ((pw = getpwuid(action->hdr.uid)) == NULL) {
			fprintf(stderr, "Failed to look up the user name\n");
			free(action);
			continue;
		}
    	
		ptr = (char *) strtok(NULL, ":");	
		action->hdr.gid = strtoul(ptr, &s, 10);
		if (ptr == s) {
			/* it is not what we intend to read, process next line */
			free(action);
			continue;
		}

		ptr = (char *) strtok(NULL, ":");	
		action->hdr.flags = strtoul(ptr, &s, 10);
		if (ptr == s) {
			/* it is not what we intend to read, process next line */
			free(action);
			continue;
		}
		/* This action was persisted */
		action->hdr.flags |= POSIX_LOG_ACTION_PERSIST;
		
		if (action->hdr.flags & POSIX_LOG_ONCE_ONLY) {
			flags |= POSIX_LOG_ONCE_ONLY;
		}
#if 0
		/* setup the query flags */
		ptr = (char *) strtok(NULL, ":");
		action->hdr.query_flags = strtoul(ptr, &s, 10);
		if (ptr == s) {
			free(action);
			continue;
		}
#endif
		/* setup threshold */
		ptr = (char *) strtok(NULL, ":");
		action->hdr.thres =  strtoul(ptr, &s, 10); 
		TRACE("loadActionsFromFile- thres=%d\n", action->hdr.thres);
		if (ptr == s) {
			/* it is not what we intend to read, process next line */
			free(action);
			continue;
		}
		/* setup interval */
		ptr = (char *) strtok(NULL, ":");
		action->hdr.interval =  strtoul(ptr, &s, 10); 
		TRACE("loadActionsFromFile- interval=%d\n", action->hdr.interval);
		if (ptr == s) {
			/* it is not what we intend to read, process next line */
			free(action);
			continue;
		}
		/* the action name */
		ptr = (char *) strtok(NULL, ": \n");
		if (ptr == NULL) {
			action->name = strdup("noname");
		} else {
			action->name = strdup(ptr);
			TRACE("name=%s-\n", action->name);
		}
		/* the action any_str */
		ptr = (char *) strtok(NULL, ": \n");
		if (ptr == NULL) {
			action->any_str = strdup("no_any_str");
		} else {
			action->any_str = strdup(ptr);
			TRACE("name=%s-\n", action->any_str);
		}
		/* 
	 	 * If the threshold and interval are set, we need
	 	 * to intialize the any list.
	 	 */
		if (action->hdr.thres > 0 && action->hdr.interval > 0) {
			action->any_list = _evlMkListNode(NULL);
		} else {
			action->any_list = NULL;
		}

		if ((pActionList =_evlAppendToList(pActionList,
							action)) == NULL) {
			fprintf(stderr,"Failed append action to list\n");
			ret = -1;
			goto exit_loadaction;
		}
		TRACE("loadfromFile:action->cmd=%s\n", action->cmd);
	}

	(void) fclose(f); /* closes fd */

	/*
	 * Attempt to send all of the actions to evlnotify.
	 */

	p = pActionList->li_next;

	while (p != pActionList) {
		action = (nfy_action_t *) p->li_data;

		if ((action->handle = notifyAdd(action->qu_str,
				QUERY_FLAGS_NOT_SET, action->hdr.nfy_id,
				flags)) == -1) {
			fprintf(stderr, "Failed to register a notifiaction\n"); 
			ret = -1;
			goto exit_loadaction;
		}

		p = p->li_next;
	}

exit_loadaction:
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	if(ret == -1) {
	 	if(action != NULL) {
			free(action);
		}
	}
	return ret;
}

/* 
 * FUNCTION	: notifyAdd
 * 
 * PURPOSE	: requests a notify request from the evlnotifyd
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
HANDLE 
notifyAdd(char *query_string, int query_flags, int notifyId, int flags)
{
	struct sigevent reg_nfy_sigevent; /* Hold the register sigevent
					   * when calling posix_log_notify_add
					   */
	char error_str[SMALL_BUF_SIZE];								
	posix_log_query_t query;
	
	HANDLE reqHandle = 0;
	
	/*
	 * Initialize the sigevent object
	 * sigev_notify should be SIGEV_SIGNAL
	 * sigev_signo should be a value between SIGRTMIN to SIGRTMAX
	 * (a range of 32 values). When a match occures this app will receive
	 * a signal with signo = this sigev_signo.
	 */
	reg_nfy_sigevent.sigev_notify=SIGEV_SIGNAL;
	reg_nfy_sigevent.sigev_signo=SIGRTMIN+1;
	reg_nfy_sigevent.sigev_value.sival_int=notifyId;	/* sub signal code - optional */
	flags &= POSIX_LOG_NFY_MASK; 	

	/* Check the query string for its basic syntax req. */
	if(!strcmp(query_string, "<null>")) {
		if(posix_log_notify_add(NULL, &reg_nfy_sigevent, flags, &reqHandle)!=0) {
			fprintf(stderr, "evlactiond: posix_log_notify_add() failed.\n");
			return -1;
		}
		return reqHandle;
	}	
	if (query_flags == QUERY_FLAGS_NOT_SET) {
		if (posix_log_query_create(query_string, POSIX_LOG_PRPS_NOTIFY,
             		&query, error_str, 200) == 0) {
			/* nop */
		} else if (posix_log_query_create(query_string, 
			POSIX_LOG_PRPS_GENERAL, &query, error_str, 200) == 0) {
			/* nop */
		} else if (posix_log_query_create(query_string,
			EVL_PRPS_TEMPLATE, &query, error_str, 200) == 0) {
			/* nop */
		} else {	
			fprintf(stderr, "evlactiond: could not create query string! Error message: \n   %s.\n", error_str);
			return -1;
		}
	} else {	
		if(posix_log_query_create(query_string, query_flags,
			&query, error_str, SMALL_BUF_SIZE) != 0) {
			fprintf(stderr, "evlactiond: could not create query string! Error message: \n   %s.\n", error_str);
			return -1;
		}
	}

												
	if(posix_log_notify_add(&query, &reg_nfy_sigevent, flags, &reqHandle)!=0) {
		fprintf(stderr, "evlactiond: posix_log_notify_add() failed.\n");
		posix_log_query_destroy(&query);
		return -1;
	}
	posix_log_query_destroy(&query);
	return reqHandle;
}

/* 
 * FUNCTION	: freeActionData
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
void 
freeActionData(void * data)
{
	nfy_action_t *act;	
	if(data) {	
		act = (nfy_action_t*) data;
		TRACE("\nRemove the request with handle=%d. ", act->handle);
		if(posix_log_notify_remove(act->handle) != 0) {
			TRACE("Failed\n");
		} else {
			TRACE("Succeeded\n");
		}
		if (act->any_list != NULL) {
			TRACE("Remove act->any_list...\n");
			removeAllNodes(act->any_list, freeAnyNode);
			free(act->any_list);
		}			
		if(act->qu_str)
			free(act->qu_str);
		if(act->cmd)
			free(act->cmd);
		if (act->name)
			free(act->name);
		if (act->any_str)
			free(act->any_str);
		free(act);
	}
}
		
/* 
 * FUNCTION	: findAction
 * 
 * PURPOSE	: looks for action object id in the list
 * 
 * ARGS		:	
 * 
 * RETURN	: returns the action object if found, NULL otherwise
 * 
 */
nfy_action_t * 
findAction(int id)
{
	nfy_action_t * action;
	/* since the first is empty 
	 * we start with the next one */
	evl_listnode_t *p = pActionList->li_next;

	if (pActionList == p) {
	/* Point to itself - it is the first empty node, list is empty */
		return NULL;
	}

	do {
		action = (nfy_action_t *) p->li_data;
		if (action->hdr.nfy_id == id) {
			return action;
		}
		p = p->li_next;
	} while (p != pActionList);
	return NULL;
}

any_t *
lookUpAnyIdent(evl_listnode_t *any_list, const char *ident)
{
	any_t * any;
	evl_listnode_t *p = any_list->li_next;

	if (any_list == p) {
		return NULL;
	}
	
	do {
		any = (any_t *) p->li_data;
		if (!strcmp(any->ident, ident)) {
			return any;
		}
		p = p->li_next;
	} while (p != any_list);
	return NULL;
}

nfy_action_t *
findActionByName(const char *name)
{
	nfy_action_t * action;
	
	/* since the first is empty 
	 * we start with the next one */
	evl_listnode_t *p = pActionList->li_next;

	if (pActionList == p) {
	/* Point to itself - it is the first empty node, list is empty */
		return NULL;
	}

	do {
		action = (nfy_action_t *) p->li_data;
		if (!strcmp(action->name, name)) {
			return action;
		}
		p = p->li_next;
	} while (p != pActionList);
	return NULL;
}

/*
 * FUNCTION	: expandRecId
 *
 * PURPOSE	: expands the %recid% in the action command.
 *
 * ARGS		:	
 *
 * RETURN	: return the pointer to the expanded string
 *
 */
char *
expandRecId(char * cmdbuf, char *expandbuf, size_t recid)
{	
	char recidStr[20];
	char *ptmp1, *ptmp2;
	char *ptmp3;
	ptmp1 = cmdbuf;
	ptmp3 = expandbuf;
	while ((ptmp2 = strstr(ptmp1, "%recid%")) != NULL) {
		memcpy(ptmp3, ptmp1, ptmp2-ptmp1);
		ptmp3 += (ptmp2 -ptmp1);
		snprintf(recidStr, sizeof(recidStr), "%u", recid);
		memcpy(ptmp3, recidStr, strlen(recidStr));
		ptmp3 += strlen(recidStr);
		ptmp1 = ptmp2 + 7;
	}
		
    if(ptmp3 == expandbuf) {
		strcpy(expandbuf, cmdbuf);
	}else{
		if(*ptmp1 != '\0') {
			memcpy(ptmp3, ptmp1, strlen(ptmp1));
			ptmp3 += strlen(ptmp1);
			*ptmp3 = '\0';
		} else {
			*ptmp3 = '\0';
		}
	}	
	return expandbuf;
}

/***
 * Code to support threshold and interval.
 ***/

void freeAnyNode(void *data)
{
	any_t * any = (any_t *) data;
	evl_listnode_t *l = (evl_listnode_t *) any->hist;

	/* If there is any hist, free them */
	if ( l != NULL) {
		TRACE("Free hist for %s node\n", any->ident); 
		removeAllNodes(l, freeHistoryNode);	
		free(l);
	}
	TRACE("freeAnyNode:%s\n", any->ident);
	free((any_t *)any->ident);
	free((any_t *)data);
}

void freeHistoryNode(void *data)
{
	free((nfy_info_t *)data);
}

any_t * 
addNewAny(evl_listnode_t *any_list, const char *ident)
{
	any_t *newany;
	newany = malloc(sizeof(any_t));
	if (newany == NULL) {
		perror("malloc");
		return;
	}
	newany->ident = strdup(ident);
	newany->hist = _evlMkListNode(NULL);
	
	any_list = _evlAppendToList(any_list, newany); 	
	return newany;
}

void
addNfyHistory(evl_listnode_t *hist, posix_log_recid_t recid)
{
	nfy_info_t *nif;

	nif = malloc(sizeof(nfy_info_t));
	if (nif == NULL) {
		perror("malloc");
		return;
	}
	nif->recid = recid;
	nif->timestamp = time(0);
		
	hist = _evlAppendToList(hist, nif);
	assert(hist != NULL);
}

/**
 * This function removes the notification history, but the list 
 * pointer and the first empty node.
 */
void resetNfyHistoryList(evl_listnode_t *hist)
{
	removeAllNodes(hist, freeHistoryNode);
}

/**
 * Take a snap shot of all the notification in a 'n seconds windowed'.
 * When this function returns the list only contains the notification
 * with timestamp with in 'n seconds windowed'
 */
void buildHistoryWindow(evl_listnode_t *hist, unsigned interval)
{
	nfy_info_t *nif;
	evl_listnode_t *next, *p = hist->li_next;
	time_t now;

	if (p == hist) {
		/* Point to itself - This is the first empty node
		 * List is empty.
		 */
		return;
	}
			
	now = time(0);
	do {
		nif = (nfy_info_t *) p->li_data;
		next = p->li_next;
		if ((now - nif->timestamp) > interval) {
			TRACE("%d sec window expired for this nif=%d\n",
					interval, nif);
			removeNode(p, freeHistoryNode);
		}
		p = next;
	} while (p != hist);
}

/**
 * Returns true if exeeds threshold
 */
int exceedThreshold(evl_listnode_t *hist, int thres)
{
	int n=0;
	evl_listnode_t *node, *pEvtList = hist;
	
	
	for ( node = pEvtList->li_next; node != pEvtList; node = node->li_next) {
		n++;
	}	
	return (n >= thres);
}
	
static evlattribute_t *
getAtt(template_t *t, const char *attName)
{
	evlattribute_t *att;
	int status = evltemplate_getatt(t, attName, &att);
	if (status != 0) {
		 fprintf(stderr,
			"Can't find attribute %s for event type %#x\n",
			attName, t->tm_header.u.u_evl.evl_event_type);
		 return NULL;
	}
	return att;
}

static evltemplate_t *
getPopulatedTemplate(struct posix_log_entry *entry, const char *buf)
{
	int status;
	evltemplate_t *tmpl;

	status = evl_gettemplate(entry, buf, &tmpl);
	if (status == ENOENT) {
		/* No such template. */
		fprintf(stderr, "No such template!\n");
		return NULL;
	} else if (status != 0) {
		fprintf(stderr, "evl_gettemplate error, status=%i\n", status);
		return NULL; 
	}

	/* tmpl is a clone of the template for this facility and event type.  */
	if (tmpl->tm_recid != entry->log_recid
		|| tmpl->tm_entry != entry
		|| tmpl->tm_data != buf) {
		status = evl_populatetemplate(tmpl, entry, buf);
		if (status != 0) {
			fprintf(stderr, "evl_populatetemplate error, status=%i\n", status);
			return NULL;
		}
	}
	return tmpl;
}
			
static int
getRec(int recid, struct posix_log_entry *entry, char *buf, size_t size)
{
	posix_logd_t logdesc;
	posix_log_query_t query;
	char qstring[40];
	int status;
	
	snprintf(qstring, sizeof(qstring), "recid = %u", recid);
	status = posix_log_query_create(qstring, POSIX_LOG_PRPS_SEEK,
		&query, NULL, 0);
	assert(status == 0);
	
	status = posix_log_open(&logdesc, logPath);
	if (status != 0) {
	       fprintf(stderr, "Failed to open log. status= %i\n", status);
       		return status;
 	}		
	status = posix_log_seek(logdesc, &query, POSIX_LOG_SEEK_LAST);	
	if (status == 0) {
		status = posix_log_read(logdesc, entry, buf, size);
		if (status != 0) {
			fprintf(stderr, "Failed to read record. status = %i\n", status);
			posix_log_close(logdesc);
			return status;
		}
	}

	posix_log_close(logdesc);
	return status;
}

void
buildIdentString(evlattribute_t *att, int append, char * buf, size_t size)
{
	char att_value[128];
	tmpl_base_type_t att_type = att->ta_type->tt_base_type;
	
	switch(att_type) {
		case TY_STRING:
			strcpy(att_value,  evl_getStringAttVal(att));
			break;
		case TY_INT:
		case TY_LONG:
			snprintf(att_value, sizeof(att_value), "%d", 
					evl_getLongAttVal(att));
			break;
		default:
			snprintf(att_value, sizeof(att_value), "unsuport_type");
			break;
	}
	TRACE("buildIdentString: att_value=%s.\n", att_value);
	if (append) {
		strcat(buf, "_");
		strcat(buf, evl_getStringAttVal(att));
	} else {
		strcpy(buf, att_value);
		TRACE("buildIdentString: buf=%s.\n", buf);
	}			
}
/*
 * From the any_str, get the attribute values in the log
 * and build the any ident
 */
char *
getAnyIdentFromLog(int recid, const char * any_str, char * ident, size_t isize)
{
	struct posix_log_entry entry;
	char buf[POSIX_LOG_ENTRY_MAXLEN];
	evltemplate_t *tmpl;
	evlattribute_t *att;
	
	int status;
	char *tok;
	char attname[80];
	char *new_any_str_p = (char *) malloc(strlen(any_str) + 1);

	if (!new_any_str_p) {
		return NULL;
	}
	
	status = getRec(recid, &entry, buf, sizeof(buf));
	if (status != 0) {
		free(new_any_str_p);
		return NULL;
	}
	tmpl = getPopulatedTemplate(&entry, buf);
	if(tmpl == NULL) {
		free(new_any_str_p);
		return NULL;
	}
	TRACE("getAnyIdentFromLog: template populated\n");
	strcpy(new_any_str_p, any_str);
	tok = strtok(new_any_str_p, ",");
	if (tok == NULL) {
		free(new_any_str_p);
		return NULL;
	}	 
	strcpy(attname, tok);
	if (!strcmp(attname, "host")) {
		strcpy(ident, (char *)_evlGetHostNameEx(entry.log_processor >> 16));
	} else {
		att = getAtt(tmpl, attname);
		if (!att) {
			free(new_any_str_p);
			return NULL;
		}
		/*
	 	 * Copy the att value
	 	 */
		buildIdentString(att, /*append=*/ 0, ident, isize); 
	}
	
	
	while ((tok = strtok(NULL, ",")) != NULL) {
		strcpy(attname, tok);
		if (!strcmp(attname, "host")) {
			strcat(ident, "_");
			strcat(ident, (char *) _evlGetHostNameEx(entry.log_processor >> 16));
		} else {
			att = getAtt(tmpl, attname);
			if (!att) {
				free(new_any_str_p);
				return NULL;
			}
			buildIdentString(att, /*append=*/ 1, ident, isize);
		}
	}
	evl_releasetemplate(tmpl);
	return ident;
}
			
