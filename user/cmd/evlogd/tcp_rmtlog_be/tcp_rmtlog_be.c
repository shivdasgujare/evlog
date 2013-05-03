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
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "config.h"
#include "posix_evlog.h"
#include "evlog.h"
#include "callback.h"
#include "../shared/rmt_common.h"

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stderr, fmt, ##args)
#else
#define TRACE(fmt, args...)		/* fprintf(stderr, fmt, ##args) */
#endif

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define TRUE	1
#define FALSE   0

#define EVLOG_PORT				12000
#define TCP_BUFFER_SIZE         128*1024
#define TCP_RMTLOG_BE_SOCKET	"/var/run/evlog_tcp_socket"

#define MAX_BUFREAD_LEN REC_HDR_SIZE + POSIX_LOG_ENTRY_MAXLEN

pid_t tcp_rmtlog_be_child = -1;

typedef struct evlbucket {
	char *buf;
	char *watermark;
	int dropped_evt;
} evlbucket_t;
static evlbucket_t tcp_rmtlog_buffer = {NULL, NULL, 0};
static size_t tcp_rmtlog_bufsize;
static fd_set tcp_rmtlog_be_all_sds;
static int tcp_rmtlog_be_maxsd = -1;
static int rmt_sd = -1;
static int tcp_rmtlog_be_Failed = FALSE;
static int tcp_rmtlog_be_socket_des = 0;
static struct sockaddr	tcp_sa;
static char tcp_rmtlog_be_conf_path[] = EVLOG_PLUGIN_CONF_DIR "/tcp_rmtlog_be.conf";
evl_callback_t evl_cb = NULL;

/* This is the backend function that the backend manager will invoke */
static int tcp_rmtlog_be_init(evl_callback_t cb_func);
static int tcp_rmtlog_be_func(const char *data1, const char *data2, 
         	              evl_callback_t cb_func);

static void  tcp_rmtlog_be_childthread();
int processMsg(int fd);
static void printStringWithNewlines(char *s, int nPrecedingNewlines);
static void printRecord(const struct posix_log_entry *entry, const char *buf);

/* The Plugin Descriptor must have this name */
be_desc_t _evlPluginDesc = {
  tcp_rmtlog_be_init,
  tcp_rmtlog_be_func
};


int connectEvlRmt()
{ 
	static char rmt_hostname[80], password[80];
	int pass_len, net_pass_len, evl_port;
	FILE *f;
	struct sockaddr_in address;
	struct in_addr inaddr;
	struct hostent *rmthost;
	int fd;
	
	if (getConfigStringParam(tcp_rmtlog_be_conf_path, "Remote Host", 
							 rmt_hostname, sizeof(rmt_hostname)) == -1) {
		return -1;
	}
	
	if (getConfigStringParam(tcp_rmtlog_be_conf_path, "Password", 
							 password, sizeof(password)) == -1) {
		return -1;
	}

	if (getConfigIntParam(tcp_rmtlog_be_conf_path, "Port", 
						  &evl_port) == -1) {
		evl_port = EVLOG_PORT;
	}
	if (getConfigIntParam(tcp_rmtlog_be_conf_path, "BufferLenInKbytes", 
						  &tcp_rmtlog_bufsize) == -1) {
		tcp_rmtlog_bufsize = TCP_BUFFER_SIZE;
		TRACE("Set buffer size to %d .\n", TCP_BUFFER_SIZE);
	} else {
		tcp_rmtlog_bufsize = tcp_rmtlog_bufsize * 1024;
	}

	/* get host name/ip address */
	if (inet_aton(rmt_hostname, &inaddr)) {
		rmthost = (struct hostent *) gethostbyaddr((char *) &inaddr, sizeof(inaddr), AF_INET);
	}
	else {
		rmthost = (struct hostent *) gethostbyname(rmt_hostname);
	}

	if (!rmthost) {
		fprintf(stderr, "Can't find remote host.\n");
		return -1;
	}	

	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Failed to create a client socket.\n");
		return -1;
	}


	address.sin_family = AF_INET;
	address.sin_port = htons(evl_port);
	 
	memcpy(&address.sin_addr, rmthost->h_addr_list[0], rmthost->h_length);

	if(connect(fd, (struct sockaddr *) &address, sizeof(address)) != 0) {
		TRACE("Failed to connect to remote server.\n");
		close(fd);
		return -1;
	}
	TRACE("fd = %d\n", fd);

	pass_len = strlen(password);
	net_pass_len = htonl(pass_len);
	if (write(fd, &net_pass_len, sizeof(int)) != sizeof(int)) {
		TRACE("Failed to exhange password len.\n");
		close(fd);
		return -1;
	}	
	if (write(fd, password, pass_len) != pass_len) {
		TRACE("Failed to exhange password.\n");
		close(fd);
		return -1;
	}	
	return fd;	
}

int _writeToRmtHost(struct posix_log_entry *rhdr, const char *varbuf) 
{
	struct net_rechdr net_hdr;
	
	TRACE("SIZEOF(pid_t)=%d\n", sizeof(pid_t));
	TRACE("SIZEOF(unsigend int)=%d\n", sizeof(unsigned int));
	TRACE("SIZEOF(long long)=%d\n", sizeof(long long));
	TRACE("SIZEOF(long long)=%d\n", sizeof(long long));
	TRACE("SIZEOF(unsigned long long)=%d\n", sizeof(unsigned long long));
	TRACE("SIZEOF(net_rechdr)=%d\n", NET_REC_HDR_SIZE);
	hton_rechdr(rhdr, &net_hdr);
	
	if (write(rmt_sd, &net_hdr,  NET_REC_HDR_SIZE) != NET_REC_HDR_SIZE) {
		TRACE("Failed to write to socket\n");
		if (tcp_rmtlog_be_maxsd == rmt_sd)
			tcp_rmtlog_be_maxsd--;
		close(rmt_sd);
		FD_CLR(rmt_sd, &tcp_rmtlog_be_all_sds);			
		rmt_sd = -1;
		return -1;
	}

	if (rhdr->log_size > 0) {
		TRACE("Write body log_size=%d ....", rhdr->log_size);
		if (write(rmt_sd, varbuf , rhdr->log_size ) != 
			rhdr->log_size) {
			TRACE("Failed to write to socket\n");
			if (tcp_rmtlog_be_maxsd == rmt_sd)
				tcp_rmtlog_be_maxsd--;
			close(rmt_sd);
			FD_CLR(rmt_sd, &tcp_rmtlog_be_all_sds);			
			rmt_sd = -1;
			return -1;
		}
		TRACE("Done write body.\n");
	}
	return 0;
}

/* Read from the socket and wrtie to the inet socket to a remote host */
int writeToRmtHost(int fd)
{
	char buf[MAX_BUFREAD_LEN];
	struct posix_log_entry *rhdr;
	int bytes_read;
	size_t reclen;

	if ((bytes_read = read(fd, buf, REC_HDR_SIZE)) != REC_HDR_SIZE ) {
		return -1;
	}
	TRACE("Read %d bytes from socket(rechrd)\n", bytes_read);

	rhdr = (struct posix_log_entry *) buf;
	if((bytes_read = read(fd, buf+REC_HDR_SIZE, rhdr->log_size)) != rhdr->log_size) {
		return -1;
	}
	TRACE("Read %d bytes from socket(vardata)\n", bytes_read);
	if (rmt_sd == -1) {
		TRACE("tcp_rmtlog_be: connection dropped ");
		if (tcp_rmtlog_buffer.buf == NULL) {
			if ((tcp_rmtlog_buffer.buf = malloc(tcp_rmtlog_bufsize)) == NULL){
				perror("malloc");
				return -1;
			}
			tcp_rmtlog_buffer.watermark = tcp_rmtlog_buffer.buf;
			tcp_rmtlog_buffer.dropped_evt = 0;
			TRACE("tcp_rmtlog_be: Buffer allocated. Size = %d.\n", 
				  tcp_rmtlog_bufsize);
		}
		reclen = rhdr->log_size + REC_HDR_SIZE;
		if ((tcp_rmtlog_buffer.watermark + reclen) <= 
			(tcp_rmtlog_buffer.buf + tcp_rmtlog_bufsize)) {
			memcpy(tcp_rmtlog_buffer.watermark, buf, reclen);
			tcp_rmtlog_buffer.watermark += reclen;
			TRACE(": Event buffered.\n");
		} else {
			/* Buffer is full drop event */
			tcp_rmtlog_buffer.dropped_evt++;
			TRACE(": Event dropped.\n");
		}
		
		return 0;
	}
		
	return _writeToRmtHost(rhdr, buf + REC_HDR_SIZE);
}

/* 
 * A little handshake from the parent thread to check
 * the remote connection.
 * Write an 'o' back to the parent for connection is OK
 * or a 'b' for connection is broken.
 * 
 * return 0 if connection is OK, otherwise -1
 */
int 
check_rmt_socket(int sd)
{
	char c;
	
	read(sd, &c, sizeof(c));
	if (c != 'c') {
		return -1;
	}
	if (rmt_sd == -1) {
		c = 'b';	/* broken */
	} else {
		c = 'o';	/* OK */
	}
	write(sd, &c, sizeof(c));
	return (c=='o')? 0 : -1; 
}  


/*
 * This process is  selecting between the socket_des and the remote sd.
 * When data is comming to the socket_des it will read from it then send to 
 * the remote host via the rmt sd.
 * When the remote connection goes away it will close the rmt sd, and tries 
 * re-connect periodicly. 
 */ 
static void tcp_rmtlog_be_childthread()
{
	int listen_sd;
	struct sockaddr_un listen_sock;
	socklen_t socklen;
	int socket_des;

	fd_set tcp_rmtlog_be_read_sds;
	int nsel;
	
	
	/* Create listening socket */
	unlink(TCP_RMTLOG_BE_SOCKET);
	if ((listen_sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return;
	}
	memset(&listen_sock, 0, sizeof(struct sockaddr_un));
	listen_sock.sun_family = PF_UNIX;
	strcpy(listen_sock.sun_path, TCP_RMTLOG_BE_SOCKET);
	socklen = sizeof(listen_sock.sun_family)+strlen(listen_sock.sun_path);
	
	if (bind(listen_sd, (struct sockaddr *) &listen_sock, socklen) < 0) {
		perror("bind");
		return;
	}
	chmod(TCP_RMTLOG_BE_SOCKET, 0600);
	if (listen(listen_sd, 0) < 0) {
		perror("listen");
		return;
	}
	
	TRACE("Listening socket created------------------\n");	

	FD_ZERO(&tcp_rmtlog_be_all_sds);
	FD_ZERO(&tcp_rmtlog_be_read_sds);
	FD_SET(listen_sd, &tcp_rmtlog_be_all_sds);
	tcp_rmtlog_be_maxsd = MAX(tcp_rmtlog_be_maxsd, listen_sd);
	TRACE("tcp_maxsd = %d, listen_sd = %d\n", tcp_rmtlog_be_maxsd, listen_sd);
	rmt_sd = connectEvlRmt();
	TRACE("rmt_sd = %d\n", rmt_sd);
	if (rmt_sd != -1) {
		FD_SET(rmt_sd, &tcp_rmtlog_be_all_sds);
		tcp_rmtlog_be_maxsd = MAX(tcp_rmtlog_be_maxsd, rmt_sd);
	}
	for(;;) {
		struct timeval tv;
		int waitfortimeout =0;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
	
		if (rmt_sd == -1 && waitfortimeout==0) {
			TRACE("Re-connect to remote host.\n");
			rmt_sd = connectEvlRmt();
			if (rmt_sd != -1) {
				TRACE("Connected to remote host. \n");
				FD_SET(rmt_sd, &tcp_rmtlog_be_all_sds);
				tcp_rmtlog_be_maxsd = MAX(tcp_rmtlog_be_maxsd, rmt_sd);
				/* 
				 * If buffer is not empty then drain it 
				 * (send events to remote hosts)
				 */
				if (tcp_rmtlog_buffer.buf != NULL &&
					(tcp_rmtlog_buffer.watermark > tcp_rmtlog_buffer.buf)) {
					tcp_rmtlog_drain_events();
				}
			}
		} 
	
		bcopy((char *) &tcp_rmtlog_be_all_sds, 
			  (char *) &tcp_rmtlog_be_read_sds, 
			  sizeof(tcp_rmtlog_be_all_sds));

		TRACE("select............tcp_child\n");
		waitfortimeout=0;
		if ((nsel = select(tcp_rmtlog_be_maxsd + 1,
				   &tcp_rmtlog_be_read_sds,
				   (fd_set *) 0, (fd_set *) 0,
				   (rmt_sd == -1)? &tv : 0)) >= 0) {
			if (errno == EINTR) {
				TRACE("select interrupt\n");
				continue;
			}
			if (nsel == 0) {
				TRACE("SELECT timeout\n");
				continue;
			}
			if (FD_ISSET(listen_sd, &tcp_rmtlog_be_read_sds)) {
				/* Accept the connetion from the parent */
				TRACE("Accepting connection from tcp parent\n");
				if ((socket_des = accept(listen_sd, NULL, NULL)) < 0) {
					perror("accept");
					goto error_accept;
				}
				FD_SET(socket_des, &tcp_rmtlog_be_all_sds);
				if (tcp_rmtlog_be_maxsd < socket_des) {
					tcp_rmtlog_be_maxsd = socket_des;
				}
				TRACE("parent connected\n");
			
			error_accept: /* make gcc happy */;
			}

			if (rmt_sd != -1 &&
				FD_ISSET(rmt_sd, &tcp_rmtlog_be_read_sds)) {
				/* remote host goes away, or got kick out*/
				if (tcp_rmtlog_be_maxsd == rmt_sd) {
					tcp_rmtlog_be_maxsd--;
				}
				close(rmt_sd);
				FD_CLR(rmt_sd, &tcp_rmtlog_be_all_sds);
				rmt_sd = -1;
				waitfortimeout=1;
			}
				
			if (FD_ISSET(socket_des, &tcp_rmtlog_be_read_sds)) {
				/* write to remote host */
				TRACE("Some data coming in the socket\n");
				if (check_rmt_socket(socket_des) == 0) {
					writeToRmtHost(socket_des);
				}
			}
		}
		
#ifdef  DEBUG2
		else {
		/* select return -1 */
			perror("select");
		} 
#endif
	}
}


/*
 * Plugin initialization.
 */
void _init()
{
	
}

void _fini()
{
	char buf[8];
	/* Kill child thread */
	if (tcp_rmtlog_be_child > 0) {
		TRACE("tcp_rmtlog_be plug-in kills its child. \n");
		kill(tcp_rmtlog_be_child, 9);
		tcp_rmtlog_be_child = -1;
	}
	if (tcp_rmtlog_be_socket_des > 0) {
		sprintf(buf,"%d", tcp_rmtlog_be_socket_des);
		(* evl_cb) (CB_PLUGIN_FD_UNSET, buf, NULL);
	}
	(* evl_cb) (CB_MK_EVL_REC, "tcp_rmtlog_be: plugin unloaded.", NULL);
	TRACE("tcp_rmtlog_be plugin unloaded.\n");
}

int 
tcp_rmtlog_drain_events()
{
	char *tmp;
	size_t reclen;
	struct posix_log_entry rechdr;

	tmp = tcp_rmtlog_buffer.buf;
	while (tmp < tcp_rmtlog_buffer.watermark) {
		memcpy(&rechdr, tmp, REC_HDR_SIZE);
		reclen = rechdr.log_size + REC_HDR_SIZE;
		if (_writeToRmtHost(&rechdr, tmp+REC_HDR_SIZE) == -1) {
			TRACE("Failed to write to the remote host\n");
			free(tcp_rmtlog_buffer.buf);
			return -1;
		}
		tmp += reclen;
	}
	TRACE("tcp_rmtlog_be: buffer drained.\n");
	if (tcp_rmtlog_buffer.dropped_evt > 0) {
	/* TODO: Write a message indicates the number of dropped records */
		char evtrec[1024];
		struct posix_log_entry *hdr;
		char *varbuf;
		
		hdr = (struct posix_log_entry *) evtrec;
		varbuf = evtrec + REC_HDR_SIZE;
		sprintf(varbuf, "%d events were dropped during the disconnected period.",
				tcp_rmtlog_buffer.dropped_evt);
	   
		hdr->log_flags = 0;
		hdr->log_size  = strlen(varbuf) +1;
		hdr->log_format= POSIX_LOG_STRING;
		hdr->log_event_type = 1;
		hdr->log_facility = LOG_LOGMGMT;
		hdr->log_severity = LOG_INFO;
		hdr->log_uid = geteuid();
		hdr->log_gid = getegid();
		hdr->log_pid = getpid();
		hdr->log_pgrp = getpgrp();
		hdr->log_processor = _evlGetProcId();
#ifdef _POSIX_THREADS
		hdr->log_thread 	= pthread_self();
#else
		hdr->log_thread 	= 0;
#endif
		reclen = hdr->log_size + REC_HDR_SIZE;
		if (_writeToRmtHost(hdr, varbuf) == -1) {
			TRACE("Failed to write to remote host\n");
			free(tcp_rmtlog_buffer.buf);
			return -1;
		}
	}
	free(tcp_rmtlog_buffer.buf);
	tcp_rmtlog_buffer.buf = tcp_rmtlog_buffer.watermark = NULL;
	tcp_rmtlog_buffer.dropped_evt = 0;
	return 0;
}

int 
rmt_connection_status()
{
	char c='c';
	write(tcp_rmtlog_be_socket_des, &c, sizeof(c));
	read(tcp_rmtlog_be_socket_des, &c, sizeof(c));
	TRACE("c = %c\n", c);
	return (c=='o')? 0: -1;
}	

/*************************
 * The backend functions *
 *************************/
/*
 * Initialize the plugin - 
 */
static int
tcp_rmtlog_be_init(evl_callback_t cb_func)
{
	int retry = 0;
	socklen_t socklen;
	char buf[8];
	char disable[4]="yes";

        /* first, store the callback function */
	evl_cb = cb_func;

	/* Check to see if this plugin is disable */
	getConfigStringParam(tcp_rmtlog_be_conf_path, "Disable", 
					  disable, sizeof(disable));
	if (!strcasecmp(disable, "yes")) {
		(* evl_cb) (CB_MK_EVL_REC, "tcp_rmtlog_be: plugin is disabled.", NULL);
		return -1; /* return - pending unload */ 
	}


	if ((tcp_rmtlog_be_child = fork()) < 0) {
		fprintf(stderr, "fork: can't fork a child!\n");
	}
	if (tcp_rmtlog_be_child == 0) {
		/* Ignore some signals */
		signal(SIGTERM, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGINT, SIG_IGN);

		tcp_rmtlog_be_childthread();
		_exit(0);
	} else {

		/* Sleep a little to make sure that child create the fifo */
		TRACE("Parent sleeps a little here..\n");
		sleep(1);
		memset(&tcp_sa, 0, sizeof(struct sockaddr));
		/* Connect to child thread */
		if ((tcp_rmtlog_be_socket_des = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
			perror("connect");
			return -1;
		}
		tcp_sa.sa_family = PF_UNIX;
		strcpy(tcp_sa.sa_data, TCP_RMTLOG_BE_SOCKET);
		socklen = sizeof(tcp_sa.sa_family)+strlen(tcp_sa.sa_data);
		
		while (connect(tcp_rmtlog_be_socket_des, &tcp_sa, socklen) < 0) {
			if (retry >= 2) {
				fprintf(stderr, "Can't connect to child thread\n");
				close(tcp_rmtlog_be_socket_des);
				return -1;
			}
			retry++;
			sleep(1);
		}
		sprintf(buf, "%d", tcp_rmtlog_be_socket_des);
		(* evl_cb) (CB_PLUGIN_FD_SET, buf, NULL);
	}
	(* evl_cb) (CB_MK_EVL_REC, "tcp_rmtlog_be plugin loaded.", NULL);
	TRACE("tcp_rmtlog_be plugin loaded.\n");
	return 0;
}

static int 
tcp_rmtlog_be_func(const char *data1, const char *data2, evl_callback_t cb_func)
{
	struct posix_log_entry *rhdr;
	size_t reclen;
	char buf[8];
	
	/* If the remote socket is not initialized or broken- 
	 * then just return 
	 */
	if (rmt_connection_status() == -1) {
		TRACE("Socket is not initialized\n");
		return 0;
	}
	/* Get the record header */
	rhdr = (struct posix_log_entry *) data1;
	if (write(tcp_rmtlog_be_socket_des, data1, rhdr->log_size+REC_HDR_SIZE) 
			!= rhdr->log_size + REC_HDR_SIZE) {
		TRACE("Failed to write to the child thread socket\n");
		 sprintf(buf, "%d", tcp_rmtlog_be_socket_des);
                (* evl_cb) (CB_PLUGIN_FD_UNSET, buf, NULL);		
		return -1;
	}
	return 0;	
}

