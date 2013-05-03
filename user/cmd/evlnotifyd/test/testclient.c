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
#include <fcntl.h>
#include <sys/klog.h>
#include <signal.h>           	/* Defines struct sigaction */
#include <errno.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"

static void NewSIGTERM();
static void signal_handler(int signo, siginfo_t *info, void *context);
static void signal_handler2(int signo, siginfo_t *info, void *context);


#define BUFSIZE	1024


char query_string[]="data contains \"Hello world\"";

posix_log_query_t query;

HANDLE reqHandle=-1;


int main(int argc, char *argv[]) {

	int c;
	
	int flags=0;
	size_t reqlen;
	int ret_flags;
	char queryStrBuf[BUFSIZE];
	int queryStrLen = BUFSIZE;
	char error_str[BUFSIZE];
	
	static struct sigaction SigTERMAction;  				/* Signal handler to terminate gracefully */
	static struct sigaction SigPipeAct;
	void NewSIGTERM();
	
	struct sigevent reg_nfy_sigevent, ret_nfy_sigevent;		/* Hold the register sigevent
															 * when calling posix_log_notify_add
															 * and the return sigevent from 
															 * posix_log_notify_get
															 */
	static struct sigaction SigRTAction; 
	static struct sigaction SigRTAction2; 
	void signal_handler();
	void signal_handler2();
	
	
	while((c=getopt(argc, argv, "1")) != EOF) {
		switch (c) {
			case '1':
				flags|=POSIX_LOG_ONCE_ONLY;
			break;          
		}
	}
	
	(void) memset(&SigTERMAction, 0, sizeof(SigTERMAction));
	SigTERMAction.sa_handler = NewSIGTERM;
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
	
	(void) memset(&SigPipeAct, 0, sizeof(SigPipeAct));
	SigPipeAct.sa_handler = SIG_IGN;
	SigPipeAct.sa_flags = 0;

	
	/*
	 * Ignore any SIGPIPE signal.  A broken pipe is a normal occurrance as
	 * the server may go away, we want to reconnect to the server if possible.
	 */
	if (sigaction(SIGPIPE, &SigPipeAct, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGPIPE.\n", argv[0]);
		perror("sigaction");
		exit(1);
	}
	/* setup signal handle 2 */
	(void) memset(&SigRTAction, 0, sizeof(SigRTAction));
	SigRTAction.sa_handler = (void *) signal_handler;
	SigRTAction.sa_flags = SA_SIGINFO;			/* This flag is set so that we 
												 * get the siginfo pass back
												 * to the signal handler
												 */
	if (sigaction(SIGRTMIN+1, &SigRTAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGRTMIN+1.\n", argv[0]);
		perror("sigaction");
	}

	/* setup signal handle 2 */
	(void) memset(&SigRTAction2, 0, sizeof(SigRTAction));
	SigRTAction2.sa_handler = (void *) signal_handler2;
	SigRTAction2.sa_flags = SA_SIGINFO;			/* This flag is set so that we 
												 * get the siginfo pass back
												 * to the signal handler
												 */
	if (sigaction(SIGRTMIN+2, &SigRTAction2, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGRTMIN+2.\n", argv[0]);
		perror("sigaction");
	}

	
	/* Check the query string for its basic syntax req. */
	if(posix_log_query_create(query_string, POSIX_LOG_PRPS_NOTIFY,
             &query, error_str, BUFSIZE) != 0) {
		fprintf(stderr, "ERROR: could not create query string! Error message: \n   %s.\n", error_str);
		exit(1);
	}

	/* 
	 * Initialize the sigevent object 
	 * sigev_notify sshould be SIGEV_SIGNAL
	 * sigev_signo should be a value between SIGRTMIN to SIGRTMAX
	 * (a range of 32 values). When a match occures this app will receive
	 * a signal with signo = this sigev_signo.
	 */
	reg_nfy_sigevent.sigev_notify=SIGEV_SIGNAL;
	reg_nfy_sigevent.sigev_signo=SIGRTMIN+1;
	reg_nfy_sigevent.sigev_value.sival_int=101;		/* sub signal code - optional */
													
	printf("# Register the notification request with posix_notify_add #\n");
	printf("-----------------------------------------------------------\n");
	

	if(posix_log_notify_add(&query, &reg_nfy_sigevent, flags, &reqHandle)!=0) {
		fprintf(stderr, "ERROR: posix_log_notify_add() failed.\n");
		exit(1);
	} else {
		printf("Registered with POSIX Event logger (handle=%d)\n\n",
			reqHandle);
	}
	
	printf("#    Retrieve the request status with posix_notify_get    #\n");
	printf("-----------------------------------------------------------\n");
	/*
	 * Retrieve request status
	 * 
	 */
	if(posix_log_notify_get(reqHandle, &ret_nfy_sigevent, &ret_flags,
        					queryStrBuf, queryStrLen, &reqlen) != 0) {
        fprintf(stderr, "ERROR: posix_log_notify_get() failed. handle=%d\n", 
        			reqHandle);
				
	} else {
		printf("handle(%d):signo=%d:sigval_int=%d:ret_flags=%d:\nquery=%s\nreqlen=%d\n\n",
			 		reqHandle, ret_nfy_sigevent.sigev_signo, ret_nfy_sigevent.sigev_value.sival_int,
			 		ret_flags, queryStrBuf, reqlen);
	}

	printf("Waiting for an event..");
	fflush(stdout);
	for(;;) {
		sleep(1);
	}
}

/*
 *
 *
 */
static void NewSIGTERM() {
	
	printf("\nRemove the request with handle=%d. ", reqHandle);
	if(posix_log_notify_remove(reqHandle) != 0) {
		printf("Failed\n");
	} else {
		printf("Succeeded\n");
	}
	/* don't forget to remove the query object */
	posix_log_query_destroy(&query);	
	printf("\n");
    exit(0);
}

/*
 * This signal handler will handle that the notification that this app 
 * register to the service.
 *
 */
static void signal_handler(int signo, siginfo_t *info, void *context) 
{
	posix_log_recid_t recid;

	size_t reqlen;
	int ret_flags;
	char queryStrBuf[BUFSIZE];
	int queryStrLen = BUFSIZE;
	sigevent_t ret_nfy_sigevent;
	/* 
	 * Since we registered this notification request with the 
	 * sigevent.sigev_value.sival_int with some value, we get it
	 * back here.
	 */
	int subSignalCode = info->_sifields._rt.si_sigval.sival_int;

	posix_log_siginfo_getrecid(info, context, &recid);
	printf("\n\n****************** signal_handler activated  ****************\n");
	printf("sig_no=%d, recid=%d, subcode=%d\n", signo, recid, 
				subSignalCode );

	if(posix_log_notify_get(reqHandle, &ret_nfy_sigevent, &ret_flags,
        					queryStrBuf, queryStrLen, &reqlen) != 0) {
        fprintf(stderr, "ERROR: posix_log_notify_get() failed. handle=%d\n", 
        			reqHandle);
				
	} else {
		printf("handle(%d):signo=%d:sigval_int=%d:ret_flags=%d: disabled=%d\nquery=%s\nreqlen=%d\n\n",
			 		reqHandle, ret_nfy_sigevent.sigev_signo, ret_nfy_sigevent.sigev_value.sival_int,
			 		ret_flags, (ret_flags & POSIX_LOG_NFY_DISABLED), queryStrBuf, reqlen);
	}
				
	/* An example of sub signal code could be used for */
	switch(subSignalCode) {
		case 101: 
		{
			struct sigevent reg_nfy_sigevent2, ret_nfy_sigevent2;		/* Hold the register sigevent
															 * when calling posix_log_notify_add
															 * and the return sigevent from 
															 * posix_log_notify_get
															 */
			static HANDLE myhandle;
			int flags=0;
			char error_str[BUFSIZE];
			char *query_string2 ="data contains \"signal_handler1\"";
			posix_log_query_t query2;
			/* do something */
			printf("Perform action for sub signal code 101 here ...\n"); 

			if (myhandle > 0) {
				size_t reqlen;
				int  ret_flags;
				char queryStrBuf[BUFSIZE];
				int queryStrLen = BUFSIZE;
				printf("Request was registered, retrieve the request status with posix_notify_get.\n");
				printf("-----------------------------------------------------------\n");
				/*
				 * Retrieve request status
				 * 
				 */
				if(posix_log_notify_get(myhandle, &ret_nfy_sigevent2, &ret_flags,
			        					queryStrBuf, queryStrLen, &reqlen) != 0) {
			        fprintf(stderr, "ERROR: posix_log_notify_get() failed. handle=%d\n", 
			        			reqHandle);
							
				} else {
					printf("handle(%d):signo=%d:sigval_int=%d:ret_flags=%d:\nquery=%s\nreqlen=%d\n\n",
						 		reqHandle, ret_nfy_sigevent2.sigev_signo, ret_nfy_sigevent2.sigev_value.sival_int,
						 		ret_flags, queryStrBuf, reqlen);
				}	
			} else {
				/* Check the query string for its basic syntax req. */
				if(posix_log_query_create(query_string2, POSIX_LOG_PRPS_NOTIFY,
		             &query2, error_str, BUFSIZE) != 0) {
					fprintf(stderr, "ERROR: could not create query string! Error message: \n   %s.\n", error_str);
					exit(1);
				}

				/* 
				 * Initialize the sigevent object 
				 * sigev_notify sshould be SIGEV_SIGNAL
				 * sigev_signo should be a value between SIGRTMIN to SIGRTMAX
				 * (a range of 32 values). When a match occures this app will receive
				 * a signal with signo = this sigev_signo.
				 */
				reg_nfy_sigevent2.sigev_notify=SIGEV_SIGNAL;
				reg_nfy_sigevent2.sigev_signo=SIGRTMIN+2;
				reg_nfy_sigevent2.sigev_value.sival_int=101;		/* sub signal code - optional */
																
				printf("# Register the notification request with posix_notify_add #\n");
				printf("-----------------------------------------------------------\n");
				

				if(posix_log_notify_add(&query2, &reg_nfy_sigevent2, flags, &myhandle)!=0) {
					fprintf(stderr, "ERROR: posix_log_notify_add() failed.\n");
					exit(1);
				} else {
					printf("Registered with POSIX Event logger (handle=%d)\n\n",
						myhandle);
				}
			}
			break;
		}
		case 102:
			/* do something */
			break;
		case 103:
			/* do something */
			break;
		default:
			/* do something */
			break;
	}
	return;
}


/*
 * This signal handler will handle that the notification that this app 
 * register to the service.
 *
 */
static void signal_handler2(int signo, siginfo_t *info, void *context) 
{
	posix_log_recid_t recid;

	size_t reqlen;
	int ret_flags;
	char queryStrBuf[BUFSIZE];
	int queryStrLen = BUFSIZE;
	sigevent_t ret_nfy_sigevent;
	/* 
	 * Since we registered this notification request with the 
	 * sigevent.sigev_value.sival_int with some value, we get it
	 * back here.
	 */
	int subSignalCode = info->_sifields._rt.si_sigval.sival_int;

	posix_log_siginfo_getrecid(info, context, &recid);
	printf("\n\n****************** signal_handler2 activated  ****************\n");
	printf("sig_no=%d, recid=%d, subcode=%d\n", signo, recid, 
				subSignalCode );

	if(posix_log_notify_get(reqHandle, &ret_nfy_sigevent, &ret_flags,
        					queryStrBuf, queryStrLen, &reqlen) != 0) {
        fprintf(stderr, "ERROR: posix_log_notify_get() failed. handle=%d\n", 
        			reqHandle);
				
	} else {
		printf("handle(%d):signo=%d:sigval_int=%d:ret_flags=%d: disabled=%d\nquery=%s\nreqlen=%d\n\n",
			 		reqHandle, ret_nfy_sigevent.sigev_signo, ret_nfy_sigevent.sigev_value.sival_int,
			 		ret_flags, (ret_flags & POSIX_LOG_NFY_DISABLED), queryStrBuf, reqlen);
	}
				
	/* An example of sub signal code could be used for */
	switch(subSignalCode) {
		case 101: 
		{
			/* do something */
			printf("Perform action for sub signal code 101 here ...\n"); 
			break;
		}
		case 102:
			/* do something */
			break;
		case 103:
			/* do something */
			break;
		default:
			/* do something */
			break;
	}
	return;
}

