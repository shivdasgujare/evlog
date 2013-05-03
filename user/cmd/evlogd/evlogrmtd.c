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
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "posix_evlog.h"
#include "evlog.h"

#include "evl_list.h"
#include "evl_util.h"
#include "evl_template.h"
#include "evl_common.h"
#include "shared/rmt_common.h"

#define EVLOG_TCP_PORT			12000
#define EVLOG_UDP_PORT          34000
#define MAX_RMT_CONNECTIONS		64
#define MAX_RMT_AUTHDATA		128
//#define DEBUG3

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stdout, fmt, ##args)
#endif

#ifndef DEBUG2
#define TRACE(fmt, args...)		/* fprintf(stderr, fmt, ##args) */
#endif

#define max(a,b) (((a)>(b))?(a):(b))
#define CLIENT_SLOT_AVAILABLE	-100

#define 	EVLHOSTS        "/etc/evlog.d/evlhosts"
#define		EVLOGRMTD_CONF	"/etc/evlog.d/evlogrmtd.conf"
#define MAX_BUFREAD_LEN REC_HDR_SIZE + POSIX_LOG_ENTRY_MAXLEN

struct host_entry {
	struct in_addr in;
	int id;
};

static struct host_entry *host_array = (struct host_entry *) 0;
static int num_hosts = 0;
	
typedef struct rmt_clientinfo {
	int sd;
	struct in_addr in;
	int node_id;
	int authenticated;

	int authdata_len;
	void *authdata;
} rmt_clientinfo_t;

rmt_clientinfo_t clients[MAX_RMT_CONNECTIONS];
static int maxcl;			/* max num of clients */
static int maxci;
static int maxsd;
fd_set all_sds;
static int evlog_sd = -1;				/* This socket is for writing events to evlogd */

static char *PidFile = "/var/run/evlogrmtd.pid";

/* Function prototypes */
int authenticate(rmt_clientinfo_t *);
void initClients();
int getMaxClientIndex();
int fwdClientEvent(rmt_clientinfo_t *ci);
static int writeToEvlogd(struct posix_log_entry *rhdr, const char *varbuf);
void closeClientSocket(rmt_clientinfo_t *ci);


/* DEBUG helper functions */
static void printRecord(const struct posix_log_entry *entry, const char *buf);


/*
 * Establish a connect to evlogd
 */
static int connectToEvlogd()
{
	int fd; 
	struct sockaddr_un evlsock;
	size_t sock_len;
	int retry = 0;

	if ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0){
		(void)fprintf(stderr, "Cannot create socket.\n");
		exit(1);
	}
	memset(&evlsock, 0, sizeof(struct sockaddr_un));
	evlsock.sun_family = PF_UNIX;
	(void)strcpy(evlsock.sun_path, EVLOGD_EVTSERVER_SOCKET);
	sock_len = sizeof(evlsock.sun_family) + strlen(evlsock.sun_path);

	if (connect(fd, (struct sockaddr *)&evlsock, sock_len) < 0) {
	//	(void)fprintf(stderr, "Cannot connect to evlogd daemon.\n");
		close(fd);
		return -1;
	}
			
	TRACE("Connected to evlogd, fd=%d.\n", fd); 
	return fd;
}

/*
 * listen to TCP clients
 */
static int listenToStreamClients()
{
	int fd, i=1, port;
	struct sockaddr_in address;
	size_t addrLen = sizeof(struct sockaddr_in);

	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Failed to create server socket.\n");
		exit(1);
	}
	
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	
	address.sin_family = AF_INET;
	if (getConfigIntParam(EVLOGRMTD_CONF, "TCPPort", &port) != 0) {
		port = EVLOG_TCP_PORT;
	}
	TRACE("TCP Port=%d\n", port);
	address.sin_port = htons(port);

	memset(&address.sin_addr, 0, sizeof(address.sin_addr));
	
	if (bind(fd, (struct sockaddr *) &address, sizeof(address))) {
		fprintf(stderr, "Failed to bind server address\n");
		close(fd);
		exit(1);
	}
	
	if (listen(fd, MAX_RMT_CONNECTIONS)) {
		fprintf(stderr, "Failed to create listening socket.\n");
		close(fd);
		exit(1);
	}
	return fd;
}
 
static int createUDPSocket()
{
	int fd, i=1, port;
	struct sockaddr_in sin;
	
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		fprintf(stderr, "Failed to create server socket.\n");
		exit(1);
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	if (getConfigIntParam(EVLOGRMTD_CONF, "UDPPort", &port) != 0) {
		port = EVLOG_UDP_PORT;
	}
	TRACE("UDPPort=%d\n", port);
	sin.sin_port = htons(port);
	
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	if (bind(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Failed to bind server address\n");
		close(fd);
		exit(1);
	}
	return fd;
}

/* 
 * evlogrmtd maintains a list of hosts that are allowed to write events
 * to evlog in memory (caching). 
 * 
 * These are supported routines for loading/unloading hostarray
 */
static int addHost(struct in_addr *in, int id)
{
	/* Allocate host table entry */
	host_array = (struct host_entry *) realloc(host_array,
											   (num_hosts+1) * sizeof(struct host_entry));
	if (host_array == NULL) {
		return 0;
	}

	memcpy(&host_array[num_hosts].in, in, sizeof(struct in_addr));
	host_array[num_hosts].id = id;
	++num_hosts;
	TRACE("Add host with ip=%s into host array.\n", inet_ntoa(*in));
	return 1;
	 	
}

int initHostArray()
{
	char *p, *name, *id;
	char line[256];
	FILE * f;
	struct hostent *hp;
	int nodeID;
	struct in_addr inet_addr;
	int ret = 0;
	
	num_hosts = 0;
	if((f = fopen(EVLHOSTS, "r")) == NULL) {
		fprintf(stderr, "can't open evlhosts file.\n");
		return -1;
	}	

	while (fgets(line, 256, f) != NULL) {
		int len;
		if (line[0] == '#' || line[0] == '\n' 
			|| strcspn(line, "") == 0) {
			continue;
		}
		len = strlen(line);
		/* replace newline with null char */
		if (line[len -1] == '\n') {
			line[len -1] = '\0';
		}

		/* host id */		
		id = (char *) strtok(line, " \t");
		if (!id) {
			/* probably a blank line with a tab or space */
			continue;
		}
		trim(id);
		/*  host name */
		name = (char *) strtok(NULL, " \t");
		if (!name) {
			ret = -1;
			goto error_exit;
		}
		/* Remove leading space and trailing space */
		trim(name);
		if ((hp = (struct hostent *)gethostbyname(name)) == NULL) {
			fprintf(stderr, "unknown host: %s.\n", name);
			continue;
		}

		if ((p=strchr(id, '.')) != NULL) {
			/* id in 255.255 format */
			char *endp = 0;
			int lowerbyte, upperbyte;
			upperbyte = (int) strtoul(id, &endp, 10);
			TRACE("ubyte=%d\n", upperbyte);
			if (*endp != '.') {
				fprintf(stderr, "%s is an invalid node id.\n", id);
				continue;
			}
			lowerbyte = (int) strtoul(p + 1, &endp, 10);
			TRACE("lbyte=%d\n", lowerbyte);
			if (*endp != '\0') {
				fprintf(stderr, "%s is an invalid node id.\n", id);
				continue;
			}
			if (upperbyte > 255 || lowerbyte > 255) {
				fprintf(stderr, "%s is an invalid node id.\n", id);
				continue;
			}
			nodeID = (upperbyte << 8) + lowerbyte;
			TRACE("NodeID= 0x%x", nodeID);
		} else {
			nodeID = (int) strtoul(id, (char **) NULL, 0);
		}
		
		memcpy(&inet_addr, hp->h_addr_list[0], hp->h_length);
		addHost(&inet_addr, nodeID);

	}
		  
 error_exit:	
	fclose(f);
	return ret;
}

int hostLookUp(struct in_addr *in, int * nodeID)
{
	int i;

	for (i=0; i < num_hosts; ++i) {
		if (host_array[i].in.s_addr == in->s_addr) {
			TRACE("hostLoopUp: Found host in cache\n");
			*nodeID = host_array[i].id;
			return 0;
		}
	}
	return -1; 
}

void destroyHostArray() 
{
	int i;

	if (host_array == NULL) {
		return;
	}
	free(host_array);
	host_array = (struct host_entry *) 0;
}
		
int reinitHostArray()
{
	destroyHostArray();
	return initHostArray();
}

/* 
 * Forward this event to evlog. 
 * Check to see if the sending host is allow to send event. Only forward event to 
 * evlog if it is allowed.
 */ 
void fwdUDPEvent(char * buff, int recvlen, struct sockaddr_in *sock)
{
	int nodeID;
	char hostname[80];
	int ret;
	struct posix_log_entry hdr;
	struct net_rechdr * net_hdr;
	char *vardata;

	/* Lookup host */
	if (hostLookUp((struct in_addr *) &sock->sin_addr, &nodeID) != 0) {
		TRACE("fwdUDPEvent: hostLookUp failed.\n");
		return;
	}
	
	net_hdr = (struct net_rechdr *) buff;
	ntoh_rechdr(net_hdr, &hdr);
	if ((hdr.log_size + NET_REC_HDR_SIZE) != recvlen) {
		/* we did not receive the whole record */
		return;
	}
	/* Put  nodeID into the upper word of log_processor field */
	hdr.log_processor |= (nodeID << 16);
	vardata = buff + NET_REC_HDR_SIZE;
	writeToEvlogd(&hdr, vardata);
#ifdef DEBUG3
	printRecord(&hdr, vardata);
#endif	

}	

void _daemonize()
{
 	pid_t pid;
 	int num_fds, i;
	
	if (_evlValidate_pid(PidFile)) {
		fprintf(stderr, "evlogrmtd: Already running.\n");
		exit(1);
	}
	/*
	 * Fork a child and let the parent exit. This guarentees that
	 * the first child is not a process group leader.
	 */
	if ((pid = fork()) < 0) {
		fprintf(stderr, 
				"evlogrmtd: Cannot fork child process. Check system process usage.\n"); 
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
				"evlogrmtd: Cannot fork child process. Check system process usage.\n"); 
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
		LOGERROR(EVLOG_WRITE_PID, "evlogrmtd: Cannot write 'evlogrmtd' PID to '%s' file\n",
				 PidFile);
#endif
		exit(1);
	}
	/*
	 * Redirect fd 0 1 2 to /dev/null .  
	 */
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
 * Handling terminate signal, save request list to file
 * and clean up before exit.
 */
static void
SIGTERM_handler()
{
	/* Clean up here */
	exit(0);
}

static void
validateTCPClients()
{
	int i, nodeid;

	for (i = 0; i < MAX_RMT_CONNECTIONS; i++) {
		if (clients[i].sd == CLIENT_SLOT_AVAILABLE) continue;
		if (hostLookUp(&clients[i].in, &nodeid) == -1) {
			/* Not in the host list anymore - remove him*/
			TRACE("Not in the evlhosts anymore!\n");
			closeClientSocket(&clients[i]);
		}
	}
}
/*
 * reinitialize the host array
 */
static int validateHostArray = 0;
static void
SIGHUP_handler(int signum)
{
	TRACE("validateHostArray flag set.\n");
	validateHostArray = 1;
}


/*
 * MAIN()
 */
int main (int argc, char **argv)
{
	struct sockaddr_in address;
	int tcp_sd, udp_sd, udp_fd, newsd;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	char buff[MAX_BUFREAD_LEN];
	struct sockaddr_in frominet;
	char *from;
	fd_set read_sds; 
	int retry = 0;
	int daemonize = 1;						/* run this app as a daemon */
	auto int c;								/* Command line option */
	struct sigaction act;
	static struct sigaction SigTERMAction;  /* Signal handler to terminate gracefully */
	void SIGTERM_handler();
	static struct sigaction SigHUPAction;  /* Signal handler to validate hostlist */
	//void SIGHUP_handler();
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
	TRACE("evlrmtd starting ...\n");
	/* Daemonize */
	if (daemonize) {
		_daemonize();
	}
	initClients();
	/* Caching the host list */
	if (initHostArray() != 0 || num_hosts == 0) {
		//fprintf(stderr, "Failed to initialize the host array.\n");
		exit(1);
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
	
	if (sigaction(SIGINT, &SigTERMAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGINT.\n", argv[0]);
		perror("sigaction");
	}
	/*
	 * SIGHUP handler - validate hostlist
	 */
	(void) memset(&SigHUPAction, 0, sizeof(SigHUPAction));
	SigHUPAction.sa_handler = SIGHUP_handler;
	SigHUPAction.sa_flags = 0;
	if (sigaction(SIGHUP, &SigHUPAction, NULL) < 0){
		(void)fprintf(stderr, "%s: sigaction failed for new SIGHUP.\n", argv[0]);
		perror("sigaction");
	}
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

	evlog_sd = connectToEvlogd();
	/*
	 * Setup listening socket for accepting connection from TCP clients.
	 */
	tcp_sd = listenToStreamClients();
	udp_fd = createUDPSocket();
	udp_sd = udp_fd;
	TRACE("udp_sd = %d \n", udp_fd);
	FD_ZERO(&all_sds);
	FD_SET(tcp_sd, &all_sds);
	FD_SET(udp_sd, &all_sds);
	maxsd = max(tcp_sd, udp_sd);

	if (evlog_sd != -1) { 
		FD_SET(evlog_sd, &all_sds);
		maxsd = max(maxsd, evlog_sd);
	}

	for (;;) {
		int nsel, i, idx;
		sigset_t oldset, newset;
		int status;
		int sigIsBlocked;
		struct timeval tv;
		
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		if (validateHostArray) {
			reinitHostArray();
			validateTCPClients();
			validateHostArray = 0;
		}
		if (evlog_sd == -1) {
			TRACE( "Try to re-connect\n");
			///connection to evlogd is broken - re-establish 
			if((evlog_sd = connectToEvlogd()) > 0) {
				FD_SET(evlog_sd, &all_sds);
				maxsd = max(evlog_sd, maxsd);
			}
		}
		bcopy((char *)&all_sds, (char *)&read_sds, sizeof(all_sds));
		TRACE("select ............tv_sev=%d\n", tv.tv_sec);
		if ((nsel = select(maxsd + 1, &read_sds, 0, 0,
						   (evlog_sd == -1)? &tv : 0)) >= 0) {
			if (errno == EINTR) {
				TRACE("got EINTR\n");
				continue;
			} 
			if (nsel == 0) {
				continue;
			}
			/*
			 * Temporary block SIGHUP signal
			 * during the time I have socket activity.
			 */
			status = sigaddset(&newset, SIGHUP);
			status = sigprocmask(SIG_BLOCK, &newset, &oldset);
			if (status != 0) {
				perror("sigprocmask");
				/*
				 * Fail to block, but probably the best course is 
				 * just to continue 
				 */
				sigIsBlocked = 0;
			} else {
				sigIsBlocked = 1;
			}

			if (FD_ISSET(tcp_sd, &read_sds)) {
				/* Client is trying to connect. */
				int newsd;
				int nodeID;
				struct sockaddr_in sockin;
				socklen_t socklen = 16;
		
				TRACE("Client's trying to connect...\n");
				if ((newsd = accept(tcp_sd, 
						(struct sockaddr *) &address,
						&addrLen)) < 0) {
					perror("accept");
					goto exit_new_sd;
				}

				if (maxcl == MAX_RMT_CONNECTIONS -1 ) {
					TRACE("Max number of clients reached.\n");
					close(newsd);
					goto exit_new_sd;	
				}
		
				TRACE("Server accepted the connection...\n");
				if (getpeername(newsd, (struct sockaddr *)  &sockin, 
								&socklen) != 0) {
					//fprintf(stderr, "Failed to get peer name.\n");
					close(newsd);
					goto exit_new_sd;
				}

				/* Reject connections from hosts not
				 * listed in our config file */
				if (hostLookUp(&sockin.sin_addr, &nodeID) != 0) {
					TRACE("Rejecting unknown client %s\n",
						inet_ntoa(sockin.sin_addr));
					close(newsd);
					goto exit_new_sd;	
				}

				maxcl++;
				FD_SET(newsd, &all_sds);
				if (maxsd < newsd) {
					maxsd = newsd;
				}
				for (idx = 0 ; idx < MAX_RMT_CONNECTIONS; idx++) {
					if( clients[idx].sd == CLIENT_SLOT_AVAILABLE) {
						TRACE("sd %d in slot %d\n", newsd, idx);
						memset(&clients[idx], 0, sizeof(clients[idx]));
						clients[idx].sd = newsd;
						memcpy(&clients[idx].in, &sockin.sin_addr, 
							   sizeof(struct in_addr));
						clients[idx].node_id = nodeID;
						TRACE("accept: nodeID=0x%x", nodeID);
						break;
					}
				}
				maxci = getMaxClientIndex();
			exit_new_sd: /* make gcc happy */ ;
			}
			/* process UDP clients */ 
			if (FD_ISSET(udp_sd, &read_sds)){
				int len = sizeof(frominet);
				TRACE("udp packet arrived..\n");
				memset(buff, 0, MAX_BUFREAD_LEN);
				i = recvfrom(udp_fd, buff, MAX_BUFREAD_LEN, 0,
							 (struct sockaddr *) &frominet, &len);
				if (i > 0) {
					/* 
					 * forward udp event. Check if message came from
					 * an expected host, and it is a complete message
					 */
					fwdUDPEvent(buff, i, &frominet);
				} else if ( i < 0 && errno != EINTR) {
					TRACE("UDP socket error: %d\n", errno);
				}
							
			}
			/* process TCP clients */
			for (i= 0; i <= maxci; i++) {
				if ( clients[i].sd == CLIENT_SLOT_AVAILABLE
				 || !FD_ISSET(clients[i].sd, &read_sds))
					continue;
				if (!clients[i].authenticated) {
					authenticate(&clients[i]);
					continue;
				}
				fwdClientEvent(&clients[i]);
			}
			if (evlog_sd > 0 && FD_ISSET(evlog_sd, &read_sds)) {
				TRACE("evlogd goes away!\n");
				close(evlog_sd);
				if (evlog_sd == maxsd) {
					maxsd--;
				}
				FD_CLR(evlog_sd, &all_sds);
				evlog_sd = -1;
			}
			/* unblock signal */
			if (sigIsBlocked == 1) {
				status = sigprocmask(SIG_SETMASK, &oldset, NULL);
				if (status != 0) {
					perror("sigprocmask (unblock)");
				}
			}
		} 
#ifdef DEBUG2
		else {
			/* select return -1 */
			perror("select");
		} 
#endif
		
	} /* end for */	
}


/* Authenticate TCP connection */	
int authenticate(rmt_clientinfo_t *ci)
{
	int n, total, pwlen, nodeID;
	char hostname[80];
	char *clnt_password, passwd[80];

	/* When we come here, allocate a reasonably sized buffer */
	if (ci->authdata_len == 0) {
		ci->authdata = malloc(MAX_RMT_AUTHDATA);
		if (ci->authdata == NULL)
			goto failed;
	}

	/* Read more data from the socket - either
	 * the string length, or the string itself
	 */
	if (ci->authdata_len < sizeof(int)) {
		n = sizeof(int) - ci->authdata_len;
		total = -1;
	} else {
		memcpy(&pwlen, ci->authdata, sizeof(int));
		pwlen = ntohl(pwlen);
		total = sizeof(int) + pwlen;
		if (total >= MAX_RMT_AUTHDATA - 1 || total < ci->authdata_len)
			goto failed;
		n = total - ci->authdata_len;
	}

	n = read(ci->sd, ci->authdata + ci->authdata_len, n);
	if (n <= 0)
		goto failed;

	ci->authdata_len += n;
	if (total < 0 || ci->authdata_len < total)
		return 0;
	
	/* We have the password string */
	clnt_password = (char *) (ci->authdata + sizeof(int));
	clnt_password[pwlen] = '\0';
	TRACE("password= %s\n", clnt_password);

	if (getConfigStringParam(EVLOGRMTD_CONF, "Password", passwd, sizeof(passwd))) {
		// Failed to obtain password from password file
		goto failed;
	}
	
	if (strcmp(clnt_password, passwd)) {
		// Failed password check
		goto failed;
	}
	posix_log_printf(LOG_LOGMGMT, 100,
					 LOG_NOTICE, 0, "evlogrmtd: This %s host is successfully authenticated.", 
					 inet_ntoa(ci->in));

	/* Free authentication data, we don't need it anymore */
	free(ci->authdata);
	ci->authdata_len = 0;
	ci->authdata = NULL;

	ci->authenticated = 1;
	return 0;

failed:	TRACE("Authentication for %s failed\n", inet_ntoa(ci->in));
	closeClientSocket(ci);
	return -1;
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
 	maxcl = maxci = 0;
 	for (i=0; i < MAX_RMT_CONNECTIONS; i++) {
 		clients[i].sd = CLIENT_SLOT_AVAILABLE;
 	}
}

/* Compute max client index */
int 
getMaxClientIndex()
{
	int i;

	for (i= MAX_RMT_CONNECTIONS - 1; i >= 0; i--) {
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
 * ARGS		: client info structure, remove request flag	
 *
 * RETURN	:
 *
 */
void 
closeClientSocket(rmt_clientinfo_t *ci)
{
	int i;
	int new_maxsd = 0;
	int tbclosedsd = ci->sd;
	
	FD_CLR(ci->sd, &all_sds);
	(void) close(ci->sd);
	ci->sd = CLIENT_SLOT_AVAILABLE;
	maxcl--;
	
	TRACE("closeClientSocket:sd = %d maxsd = %d\n", tbclosedsd, maxsd);

	if (ci->authdata) {
		free(ci->authdata);
		ci->authdata_len = 0;
		ci->authdata = NULL;
	}

	/* 
	 * If maxsd = evlogd_sd, we don't need to compute the new 
	 * maxsd
	 */ 
	if (maxsd == evlog_sd) {
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
	 * connects to evlogd, and the to be closed sd = maxsd then
	 * maxsd shoulde be maxsd-1
	 */
	if (new_maxsd == 0 && tbclosedsd == maxsd) {
		maxsd--;
	} else if (new_maxsd != 0) {
		if (new_maxsd < evlog_sd)
			maxsd = evlog_sd;
		else
			maxsd = new_maxsd;
	}
	
	TRACE("Done closeClientSocket: maxsd = %d\n", maxsd);
	return;
}


int 
fwdClientEvent(rmt_clientinfo_t *ci)
{
	char buf[MAX_BUFREAD_LEN];
	char *varbuf;
	struct posix_log_entry *rhdr;
	struct net_rechdr net_rhdr;
	int n, ret = 0;

	TRACE("SIZEOF(long long)=%d\n", sizeof(long long));
	TRACE("SIZEOF(net_rechdr)=%d\n", NET_REC_HDR_SIZE);
	rhdr = (struct posix_log_entry*) buf;
	varbuf = (char *)(buf + REC_HDR_SIZE);
	TRACE("fwdClientEvent invoke!\n");
	/* Read the header */
	if (_evlReadEx(ci->sd, &net_rhdr, NET_REC_HDR_SIZE) != NET_REC_HDR_SIZE) {
		TRACE("Failed to read rechdr.\n");
		ret = -1;
		closeClientSocket(ci);
		goto err_exit;
	}

	ntoh_rechdr(&net_rhdr, rhdr);
	
	if (rhdr->log_size > 0) {
		TRACE("fwdClientEvent: reading rec body..\n");
		/* Then read the body */
		if (_evlReadEx(ci->sd, varbuf, rhdr->log_size) != rhdr->log_size) {
			TRACE("Failed to read rec body.\n");
			ret -1;
			closeClientSocket(ci);
			goto err_exit;
		}	
	}
	/* Squeeze the node id in log_processor field */
	ci->node_id &= 0x0000FFFF;
	rhdr->log_processor |= (ci->node_id << 16);
	TRACE("fwdClientEvent herei, nodeID=0x%x, log_processor=0x%x.\n", 
		  ci->node_id,rhdr->log_processor );
	
	/* Write the rec to the evlogd */
	if((ret= writeToEvlogd(rhdr, varbuf)) == -1) {
		TRACE("evlogd probably died\n");
	}
#ifdef DEBUG3
	printRecord(rhdr, varbuf);
#endif
 err_exit:

	return ret;
}
/*
 * write event to log daemon
 */
static int writeToEvlogd(struct posix_log_entry *rhdr, const char *varbuf)
{
	int ret=0;
	unsigned char c;

	if (evlog_sd == -1) 
		return -1;
	/* First write the header */
	if (write(evlog_sd, rhdr, REC_HDR_SIZE) != REC_HDR_SIZE) {
		/* socket is broken */
		fprintf(stderr, "Failed to write the msg header to evlog daemon.\n");
		goto err_exit;
	}
	/* then write the variable message body */
#ifdef POSIX_LOG_TRUNCATE
	if (rhdr->log_format == POSIX_LOG_STRING
	    && (rhdr->log_flags & POSIX_LOG_TRUNCATE) != 0) {
	    char writebuf[POSIX_LOG_ENTRY_MAXLEN];
		/*
		 * buf contains a string that was truncated to
		 * POSIX_LOG_ENTRY_MAXLEN bytes.  Stick a null character at the
		 * end of our copy of the buffer to make a null-terminated
		 * string.
		 */
		bcopy((void *)varbuf, (void *)writebuf, rhdr->log_size);
		writebuf[POSIX_LOG_ENTRY_MAXLEN - 1] = '\0';
		if (write(evlog_sd, writebuf, rhdr->log_size) != rhdr->log_size) {
			/* socket is broken */
			fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
			goto err_exit;
		}
	}
#endif
	if (rhdr->log_size > 0) {
		if (write(evlog_sd, varbuf, rhdr->log_size) != rhdr->log_size) {
			/* socket is broken */
			fprintf(stderr, "Failed to write the msg body to evlog daemon.\n");
			goto err_exit;
		}
	}
	/* The daemon should tell the client that he finishes reading */ 
	read(evlog_sd, &c, sizeof(char));
	if(c != 0xac) {
		TRACE("Write problem, c=%u\n", c);
	} else {
		return 0;
	}
 err_exit:
	close(evlog_sd);
	evlog_sd = -1;
	return -1;
}

/***************************/
/*   DEBUG HELPER          */
/***************************/
/* 
 * This code were yanked from evlview (with a little changes), I use this code
 * for testing this plug-in. Event records will be printed to stdout.
 * Note: The recid is not aligned with recid in the log (or maybe there is 
 * no log at all), it will be incremented by 1 starting from 0.
 */
#define FMT_BIN 0x1
#define FMT_TEXT 0x2
#define FMT_DEFAULT FMT_TEXT
#define FMT_SPEC (FMT_TEXT|0x4)


int format = FMT_TEXT;
int outFd = 1;		/* Write to here */
//FILE *outFile = NULL;
char *defaultSeparator = ", ";

int newlines = -1;
char *fdbuf;		/* Formatted data goes here. */
size_t fdbufLen = 0;

static void
printStringWithNewlines(char *s, int nPrecedingNewlines)
{
	if (newlines == -1) {
		fprintf(stdout, "%s", s);
		if (!_evlEndsWith(s, "\n")) {
			fprintf(stdout, "\n");
		}
	} else {
		int i;
		int nTrailingNewlines = 0;
		int totalNewlines;
		int slen = strlen(s);

		assert(nPrecedingNewlines == 0 || nPrecedingNewlines == 1);

		for (i = slen-1; i >= 0 && s[i] == '\n'; i--) {
			nTrailingNewlines++;
		}
		if (slen > nTrailingNewlines) {
			totalNewlines = nTrailingNewlines;
		} else {
			/* s is nothing but newlines. */
			totalNewlines = nPrecedingNewlines + nTrailingNewlines;
		}
		if (totalNewlines < newlines) {
			fprintf(stdout, "%s", s);
			while (totalNewlines < newlines) {
				fprintf(stdout, "\n");
				totalNewlines++;
			}
		} else if (totalNewlines > newlines) {
			int newlinesToShave = totalNewlines - newlines;
			char *p = s + (slen - newlinesToShave);
			char tmp = *p;
			*p = '\0';
			fprintf(stdout, "%s", s);
			*p = tmp;
		} else {
			fprintf(stdout, "%s", s);
		}
	}
}

static void
printRecord(const struct posix_log_entry *entry, const char *buf)
{
	int status;
	size_t linelen = 80;
	int flags = 0x0;
	
	fdbufLen = _evlGetMaxDumpLen();
	//	assert(fdbufLen > 1000);
	fdbuf = (char*) malloc(fdbufLen);
	//	assert(fdbuf != NULL);
		
	status = evl_format_evrec_fixed(entry, fdbuf, fdbufLen,
									NULL, defaultSeparator, linelen, flags);
	if (status != 0) {
		errno = status;
		perror("evl_format_evrec_fixed");
		exit(2);
	}
	(void) fprintf(stdout, "%s\n", fdbuf);

	/* Now the variable part... */
	
	switch (entry->log_format) {
	case POSIX_LOG_NODATA:
	    {
			static char nada[1] = "";
			printStringWithNewlines(nada, 1);
			break;
	    }
	case POSIX_LOG_STRING:
		printStringWithNewlines((char*)buf, 1);
		break;
	case POSIX_LOG_BINARY:
		status = _evlDumpBytes(buf, entry->log_size,
							   fdbuf, fdbufLen, NULL);
		assert(status == 0);
		printStringWithNewlines(fdbuf, 1);
		break;
	}
		
	free(fdbuf);
}
