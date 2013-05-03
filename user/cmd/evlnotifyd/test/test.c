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
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/klog.h>
#include <sys/socket.h>       	/* Defines struct sockaddr */
#include <signal.h>           	/* Defines struct sigaction */

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"


static void signal_handler(int signo, siginfo_t *info, void *context);


#define SMALLBUFSIZE	1024
#define MAXTIMEOUT	3600
#define MAX_REQS	100
char query_string[SMALLBUFSIZE], error_string[SMALLBUFSIZE];
posix_log_query_t *query;
char q[SMALLBUFSIZE], logfile[SMALLBUFSIZE];
int reqIdx, reqIdx2;
HANDLE myhandle[MAX_REQS];

static int sd;
static struct sockaddr sock;            /* Generic UD socket address */

int	oflag=0, tflag=0, timeout=0;
extern char *optarg;
extern int optind, opterr, optopt;

void cleanup() {

    printf("\nDe-register Event logging and exit...\n");
	
	posix_log_query_destroy(query);
	for(reqIdx = 0; reqIdx < MAX_REQS; reqIdx++) {
		printf("Remove the request handle(%d)=%d ", reqIdx, myhandle[reqIdx]);
		if(posix_log_notify_remove(myhandle[reqIdx]) != 0) {
			printf("failed\n");
			fprintf(stderr, 
			"WARNING: posix_log_notify_remove() failed for handle(%d)=%d\n",
				reqIdx, myhandle[reqIdx]);
		} else {
			printf("succeeded\n");
		}
	}	
	printf("\n");
    exit(0);
}

int usage() {
	printf("Usage: test [-o] [-t timeout] [query string]\n");
	printf("  -o   specifies that the query should be registered as \"once only\" so\n");
	printf("       that only a first query match will lead to a callback.\n");
	printf("  -t   Specifies a number of seconds to run the test before exitting.\n");
	printf("       Default action is to run the test forever. Exitting early may be\n");
	printf("       useful with automated test suites and the -n and -x options.\n");
	printf("  query: String for query. Example: (severity > WARNING) & (facility = KERN)\n");
	return(0);
}



int main(int argc, char *argv[]) {

	int m;
	char *mesg;
	int i, count1, count2;
	int ret;
	int once=0;
	struct sigevent mynotification, retnfysigevent;
	static struct sigaction SigRTAction; 
	void signal_handler();

        while((m=getopt(argc, argv, "not:x:")) != EOF)
                switch (m) {
                        case 'o':
                                oflag=1;
                                break;
                        case 't':
                                timeout=atoi(optarg);
				if(timeout<1 || timeout>MAXTIMEOUT) {
					fprintf(stderr, "ERROR: test: Invalid timeout. Must be between 1 and %d.\n", MAXTIMEOUT);
				}
                                tflag=1;
                                break;
		}

	if(argc < 2) {
		usage();
		exit(1);
	}

	once=oflag;
	if(optind < argc) {
		/* Get the query string if one is there */
		mesg=malloc(strlen(argv[optind])+1);
		sprintf(mesg, "%s", argv[optind]);
		for(i=optind+1; i<argc; ++i) {
			realloc(mesg, strlen(mesg)+strlen(argv[i])+1);
			strcat(mesg, " ");
			strcat(mesg, argv[i]);
		}
	} else {
		mesg=NULL;
	}
	if(mesg==NULL) {
		strcpy(query_string, "size >= 0");		/* This will match everything */
	} else {
		strcpy(query_string, mesg);
	}
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGTERM, cleanup);

	(void) memset(&SigRTAction, 0, sizeof(SigRTAction));
	SigRTAction.sa_handler = (void *) signal_handler;
	SigRTAction.sa_flags = SA_SIGINFO;			/* This flag is set so that we 
												 * get the siginfo pass back
												 * to the signal handler
												 */
	if (sigaction(SIGRTMIN+1, &SigRTAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGRTMIN.\n", argv[0]);
		perror("sigaction");
	}

	query = (posix_log_query_t *)&q;
	
	/* Check the query string for its basic syntax req. */
	if(posix_log_query_create(query_string, POSIX_LOG_PRPS_NOTIFY,
             query, error_string, SMALLBUFSIZE) != 0) {
		fprintf(stderr, "ERROR: could not create query string! Error message: \n   %s.\n", error_string);
		exit(1);
	}

	/* Initialize myntification with the local function name which is
	   to be the callback function and set the desired POSIX signal
	   value. If your application only needs one callback registration
	   then use SIGRTMIN+1. For each additional callback function, you
	   should use different signal values between SIGRTMIN to SIGRTMAX
	   (a range of 32 values). */
	mynotification.sigev_notify=SIGEV_SIGNAL;
	mynotification.sigev_signo=SIGRTMIN+1;
	mynotification.sigev_value.sival_int=101;		/* sub signal code - optional */
													
	printf("###########################################################\n");
	printf("# Register the notification request with posix_notify_add #\n");
	printf("###########################################################\n");
	
	for(reqIdx = 0; reqIdx < MAX_REQS; reqIdx++) {
		if(posix_log_notify_add(query, &mynotification, once, 
			&myhandle[reqIdx])!=0) {
			fprintf(stderr, "ERROR: posix_log_notify_add() failed.\n");
			if(errno==ECONNREFUSED)
				fprintf(stderr, "	notifyd may not be running.\n");
			exit(1);
		} else {
			printf("Registered with POSIX Event logger (handle=%d)\n",
				myhandle[reqIdx]);
		}
	}
	printf("###########################################################\n");
	printf("# Retrieve the request status with posix_notify_get       #\n");
	printf("###########################################################\n");
	/*
	 * Retrieve request status
	 * 
	 */
	for(reqIdx = 0; reqIdx < MAX_REQS; reqIdx++) {
		size_t reqlen;
		int once_only;
		char qsbuf[200];
		int qslen = 200;
		
		if(posix_log_notify_get(myhandle[reqIdx],
        		&retnfysigevent, &once_only,
        		qsbuf, qslen, &reqlen) != 0) {
        	fprintf(stderr, "ERROR: posix_log_notify_get() failed. handle(%d)=%d\n", 
        					reqIdx, myhandle[reqIdx]);
				
		} else {
			printf("handle(%d) - signo=%d, sigval_int=%d, onceonly=%d, query=%s, reqlen=%d\n",
			 		myhandle[reqIdx], retnfysigevent.sigev_signo, retnfysigevent.sigev_value.sival_int,
			 		once_only & POSIX_LOG_ONCE_ONLY, qsbuf, reqlen);
		}	
	}
	printf("Waiting for an event..");
	fflush(stdout);
	count1=0;
	count2=0;
	while(1==1) {
		if(count1++>=60) {
			printf(".");
			fflush(stdout);
			count1=0;
		}
		sleep(1);
		if(tflag==1 && count2++>=timeout) {
			printf("\nExiting on timeout=%d\n", timeout);
			cleanup();
		}
	}
}/* end of main */

/*
 * This signal handler will handle that the notification that this app 
 * register to the service.
 *
 */

static void signal_handler(int signo, siginfo_t *info, void *context) 
{
	static int cnt;
	posix_log_recid_t recid;
	/* 
	 * Since we registered this notification request with the 
	 * sigevent.sigev_value.sival_int with some value, we get it
	 * back here.
	 */
	int subSignalCode = info->_sifields._rt.si_sigval.sival_int;

	posix_log_siginfo_getrecid(info, context, &recid);
	printf("\n\n****************** signal_handler activated  ****************\n");
	printf("sig_no=%d, recid=%d, subcode=%d, count=%d\n", signo, recid, 
				subSignalCode, cnt++ + 1 );
				
	/* An example of sub signal code could be used for */
	switch(subSignalCode) {
		case 101: 
			/* do something */
			break;
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


