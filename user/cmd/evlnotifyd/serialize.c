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


#include <stdio.h>            	/* Defines NULL */
#include <string.h>           	/* Defines strcmp(), strcpy(), strlen() */
#include <fcntl.h>            	/* Defines FILE, fopen() */
#include <unistd.h>	      		/* Defines fork() and vfork() */
#include <sys/socket.h>       	/* Defines struct sockaddr */
#include <errno.h>            	/* Defines errno */
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/types.h>


#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_list.h"
#include "evlnotifyd.h"

evl_listnode_t *pNfyReqList;	/* List of notify request */

#define EVLNOTIFYD_REQ_FILE	EVLOG_STATE_DIR "/notify_request"
#define EVLNOTIFYD_REQ_TMPFILE 	EVLOG_STATE_DIR "/notify_request~"
#define SMALL_BUF_SIZE			1024

#define FILE_OP_FAILED			0
#define FILE_OP_SUCCEEDED		1

/* 
 * FUNCTION	: openReqFile
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int 
openReqFile(int flags)
{
#if 0
	int fd;
	
	fd = open(EVLNOTIFYD_REQ_FILE, flags, S_IRWXU);
	if ( fd < 0) {
		perror(EVLNOTIFYD_REQ_FILE);
		return -1;
	}
	
	return fd;
#endif

	int fd, tmpfd;
	int n, nbytesRead;
	char bytes[SMALL_BUF_SIZE];
	
	if ((fd = open(EVLNOTIFYD_REQ_FILE, flags, S_IRWXU)) < 0) {
		LOGERROR(EVLOG_OPEN_REQF_FAILED, "evlnotifyd: Cannot open nfy request file");
		(void)fprintf(stderr, "file to open file \n");
		return -1;
	}
	/*
	 * Create temporary copy of the file, caller will work on
	 * this copy and rename it to the original when every operation
	 * with the file succeeded by invoking closeReqFile
	 * 
	 */
	if ((tmpfd = open(EVLNOTIFYD_REQ_TMPFILE, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU)) < 0) {
		LOGERROR(EVLOG_OPEN_REQF_FAILED, "evlnotifyd: Cannot create nfy temp request file");
		TRACE("Failed to open temp file\n");
		(void)fprintf(stderr, "file to open file \n");
		close(fd);
		return -1;
	}

	/* Copy the contents of the real file to the temp file */
	while((nbytesRead = read(fd, bytes, SMALL_BUF_SIZE)) > 0) {
	
		if((n = write(tmpfd, bytes, nbytesRead)) != nbytesRead) {
			TRACE("Failed to write to temp file\n");
			close(fd);
			close(tmpfd);
		 	return -1;
		}
	}
	/* done with the original file for now */
	close(fd);
	
	if(nbytesRead < 0) {
		close(tmpfd);
		return -1;
	}
	/* goto begining of the file */
	lseek(tmpfd, 0, SEEK_SET);
	return tmpfd;

}

/* 
 * FUNCTION	: closeReqFile
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
void 
closeReqFile(int fd, int fileop_success)
{
#if 0
	(void) fcntl(fd, F_UNLCK);
	(void) close(fd);
	
#endif
	(void) fcntl(fd, F_UNLCK);
	(void) close(fd);
	if(fileop_success == FILE_OP_SUCCEEDED) {
		unlink(EVLNOTIFYD_REQ_FILE);
		/* rename the temporary file to the original file */
		rename(EVLNOTIFYD_REQ_TMPFILE, EVLNOTIFYD_REQ_FILE);
	}
	return;
}

/* 
 * FUNCTION	: loadReqFromFile
 * 
 * PURPOSE	: Populate the list with notify requests that are loaded from file
 * 
 * ARGS		:	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */
int 
loadReqFromFile()
{
	int fd;
	char errbuf[200];
	int status;
	nfyReq_t *nfyReq = NULL;
	req_rec_hdr_t rec;
	char *cmd_str, *query_str;
	int n;
	char buf[sizeof(req_rec_hdr_t)];

	if ((fd = openReqFile(O_RDWR|O_CREAT)) < 0) {
		return -1;
	}
	(void) fcntl(fd, F_RDLCK);
	while ((n = readNextReqHdrFromFile(fd, &rec)) > 0) {
		if(rec.recMagic != 0xbeefface) {
			closeReqFile(fd, FILE_OP_FAILED);
			return -1;
		}
		if((query_str = (char *) malloc(rec.queryLen + 1)) == NULL ) {
			closeReqFile(fd, FILE_OP_FAILED);
			return -1;
		}
		if ((n = readNextQueryStrFromFile(fd, query_str, rec.queryLen)) == -1 ) {
			closeReqFile(fd, FILE_OP_FAILED);
			return -1;
		}
		query_str[n] = '\0';
		TRACE("query_str=%s\n", query_str);
		if ((cmd_str = (char *) malloc(rec.cmdLineLen + 1)) == NULL) {
			closeReqFile(fd, FILE_OP_FAILED);
			return -1;
		}
		if ((n = readNextCmdStrFromFile(fd, cmd_str, rec.cmdLineLen)) == -1) {
			return -1;
		}
		cmd_str[n] = '\0';

		/*
		 * Validate if process is still around, only instantiate
		 * a request object if the process is still there
		 */
		if(validateProc(rec.nfyhdr.pid, cmd_str) == -1) {
			TRACE("Process %d not found\n", rec.nfyhdr.pid );
		 	free(query_str);
		 	free(cmd_str);
		 	continue;
		}

		if((nfyReq = (nfyReq_t *) malloc(sizeof(nfyReq_t))) == NULL ) {
			perror("malloc:");
		 	exit(1);
		}
		
		if (!strcmp(query_str, "<null>")) {
	    	nfyReq->nfy_query.qu_tree = NULL;
	    	status = 0;
		} else {
			if((status = posix_log_query_create(query_str,
					POSIX_LOG_PRPS_NOTIFY, 
					&nfyReq->nfy_query, errbuf, 200)) == 0){
				/* nop */
			} else if ((status = posix_log_query_create(query_str,
					POSIX_LOG_PRPS_NOTIFY,
					&nfyReq->nfy_query, errbuf, 200)) == 0){
				/* nop */
			} else if ((status = posix_log_query_create(query_str,
					EVL_PRPS_TEMPLATE,
					&nfyReq->nfy_query, errbuf, 200)) == 0){
				/* nop */
			}
		}
		
		if (status == EINVAL) {
			fprintf(stderr, "query error: %s\n", errbuf);
			closeReqFile(fd, FILE_OP_FAILED);
			return -1;
		} else if (status != 0) {
			fprintf(stderr, "Internal error: errno %d.\n", status);
			closeReqFile(fd, FILE_OP_FAILED);
			return -1;
		}
		/* done with query_str, free it */
		if (query_str) {
			free(query_str);
		}
		/* Copy sigevent */
		memcpy(&nfyReq->nfy_sigevent, &rec.sig_evt, sizeof(sigevent_t));
		
		setNotifyHandle(rec.nfyhdr.handle);
		nfyReq->nfyhdr.handle = rec.nfyhdr.handle;
		nfyReq->nfyhdr.pid = rec.nfyhdr.pid;
		nfyReq->procCmd = cmd_str;
		nfyReq->nfyhdr.flags = rec.nfyhdr.flags;
		nfyReq->nfyhdr.sd = -1;						/* socket gone away by now :-) */
		
		
		if((pNfyReqList =_evlAppendToList(pNfyReqList, nfyReq)) == NULL) {
			fprintf(stderr,"Failed append request to list\n");
			closeReqFile(fd, FILE_OP_FAILED);
			return -1;
		}
	}
	if (n < 0) {
		closeReqFile(fd, FILE_OP_FAILED);
		return -1;
	}
	closeReqFile(fd, FILE_OP_SUCCEEDED);	 	
	TRACE("Load succeeded\n");
	return 0;
 }
 
/* 
 * FUNCTION	: writeRequestsToFile
 * 
 * PURPOSE	: write the whole list of notify requests to file
 * 
 * ARGS		:	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */
int 
writeRequestsToFile()
{
	int fd;
	evl_listnode_t *p = pNfyReqList->li_next;		/* since the first node is empty */
													/* we start with the next one 	*/ 
	nfyReq_t *req;

	if((fd = openReqFile(O_RDWR|O_TRUNC)) < 0) {
		fprintf(stderr, "Failed to open notify_requests file\n");
		return -1;
	}
	(void) fcntl(fd, F_WRLCK);
	if (p == pNfyReqList) {
		/* it points to itself it is an empty list */
		closeReqFile(fd, FILE_OP_SUCCEEDED);
		return 0;
	}
	
	do {
		req = (nfyReq_t *) p->li_data;

		if(writeReqToFile(fd, req) == -1) {
			closeReqFile(fd, FILE_OP_FAILED);
			return -1;
		}	
		p = p->li_next;
	} while (p != pNfyReqList);
	closeReqFile(fd, FILE_OP_SUCCEEDED);
	return 0;
}

/* 
 * FUNCTION	: apendReqToFile
 * 
 * PURPOSE	: append a notify request to file
 * 
 * ARGS		:	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */
int 
appendReqToFile(nfyReq_t *req)
{
	int fd;
	
	if((fd = openReqFile(O_RDWR|O_CREAT)) < 0) {
		fprintf(stderr, "Can't open notify_request file\n.");
		return -1;
	} 
	lseek(fd, 0, SEEK_END);
	(void) fcntl(fd, F_WRLCK);
	if(writeReqToFile(fd, req) == -1) {
		closeReqFile(fd, FILE_OP_FAILED);
		return -1;
	}	
	closeReqFile(fd, FILE_OP_SUCCEEDED);
	return 0;
}	

/* 
 * FUNCTION	: writeReqToFile
 * 
 * PURPOSE	: write a notify request to file
 * 
 * ARGS		:	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */
int 
writeReqToFile(int fd, nfyReq_t *req)
{
	static char *nullqu="<null>";
	req_rec_hdr_t rec;

	rec.recMagic = 0xbeefface;
	memcpy(&rec.nfyhdr, &req->nfyhdr, sizeof(nfy_req_hdr_t));
	memcpy(&rec.sig_evt, &req->nfy_sigevent, sizeof(sigevent_t));
	
	if (!req->procCmd) {
		/* This should never happen */	
		return -1;
	}
	rec.cmdLineLen = strlen(req->procCmd);
	if (req->nfy_query.qu_tree != NULL) {
		rec.queryLen = strlen(req->nfy_query.qu_expr);
		TRACE("querystr=%s;cmdline=%s\n",req->nfy_query.qu_expr, req->procCmd);
	} else {
		rec.queryLen = strlen(nullqu);
		TRACE("querystr=%s;cmdline=%s\n",nullqu, req->procCmd);
	}
	
	if(write(fd, &rec, sizeof(req_rec_hdr_t)) != sizeof(req_rec_hdr_t)) {
#if 0
		LOGERROR(EVLOG_WRITE_REQF_FAILED, "evlnotifyd: Cannot write request header");
#endif
		return -1;
	}
	if(req->nfy_query.qu_tree != NULL) {
		rec.queryLen = strlen(req->nfy_query.qu_expr);
		if(write(fd, req->nfy_query.qu_expr, rec.queryLen) != rec.queryLen) {
#if 0
			LOGERROR(EVLOG_WRITE_REQF_FAILED, "evlnotifyd: Cannot write query string");
#endif
			return -1;
		}
	} else {
		
		TRACE("write null query str\n");
		rec.queryLen = strlen(nullqu);
		if(write(fd, nullqu, strlen(nullqu)) != rec.queryLen) {
#if 0
			LOGERROR(EVLOG_WRITE_REQF_FAILED, "evlnotifyd: Cannot write query string");
#endif
			return -1;
		}
		TRACE("done write null query str\n");
	}		
	if(write(fd, req->procCmd, rec.cmdLineLen) != rec.cmdLineLen) {
#if 0
		LOGERROR(EVLOG_WRITE_REQF_FAILED, "evlnotifyd: Cannot write cmdline string");
#endif
		return -1;
	}
	TRACE("Done writing rec...\n");
	return 0;
}

/* 
 * FUNCTION	: readNextReqHdrFromFile
 * 
 * PURPOSE	: read the notify request header from file
 * 
 * ARGS		:	
 * 
 * RETURN	: returns number of bytes read, returns -1 if failed
 * 
 */
int 
readNextReqHdrFromFile(int fd, req_rec_hdr_t *rec)
{
	int n = 0;
	
	n = read(fd, rec, sizeof(req_rec_hdr_t));
	if(n < 0) {
#if 0
		LOGERROR(EVLOG_READ_REQF_FAILED, "evlnotifyd: Cannot read nfy rec header");
#endif
		return -1;
	}
	return n;
}

/* 
 * FUNCTION	: readNextQueryStrFromFile
 * 
 * PURPOSE	: read the query string
 * 
 * ARGS		:	
 * 
 * RETURN	: returns number of bytes read, returns -1 if failed
 * 
 */
int 
readNextQueryStrFromFile(int fd, char *qu_str, int len)
{
	int n;
	
	if((n = read(fd, qu_str, len)) != len) {
#if 0
		LOGERROR(EVLOG_READ_REQF_FAILED, "evlnotifyd: Cannot read query string");
#endif
		return -1;
	}
	return n;
}

/* 
 * FUNCTION	: readNextCmdStrFromFile
 * 
 * PURPOSE	: read the notify's cmdline from file
 * 
 * ARGS		:	
 * 
 * RETURN	: returns number of bytes read, returns -1 if failed
 * 
 */
int 
readNextCmdStrFromFile(int fd, char *cmd_str, int len)
{
	int n;

	if((n = read(fd, cmd_str, len)) != len) {
#if 0
		LOGERROR(EVLOG_READ_REQF_FAILED, "evlnotifyd: Cannot read cmdline string");
#endif
		return -1;
	}
	return n;
}


/* 
 * FUNCTION	: validateProc
 * 
 * PURPOSE	: verify that the process with the pid and cmdline
 *			  still running
 * 
 * ARGS		: pid, cmdline	
 * 
 * RETURN	: returns 0 if such process exist, otherwise -1
 * 
 */
int 
validateProc(pid_t pid, char *cmd_str)
{
	char cmd[MAX_PROGRAM_NAME+1];
	char path[40];
	int fd;
	size_t r;

	snprintf(path, sizeof(path), "/proc/%u/cmdline", pid);
	if((fd = open(path, O_RDONLY)) == -1) {
		return -1;
	}

	if (( r = read(fd, cmd, MAX_PROGRAM_NAME)) == -1) {
		close(fd);
		return -1;
	}
	cmd[r] = '\0';

/*	TRACE("cmd_str=%s : cmdline=%s\n", cmd_str, cmd);	*/
	if((r = strcmp(cmd_str, cmd)) != 0) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}





