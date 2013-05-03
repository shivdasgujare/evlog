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
#include <netdb.h>

#include "config.h"
#include "posix_evlog.h"
#include "evlog.h"
#include "callback.h"
#include "../shared/rmt_common.h"

/* 
 * This is the source for an evlogd plugin (DLL), that forwards events via
 * UDP datagrams 
 */

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stderr, fmt, ##args)
#else
#define TRACE(fmt, args...)		/* fprintf(stderr, fmt, ##args) */
#endif


#define TRUE	1
#define FALSE   0

#define EVLOG_UDP_PORT				34000

static int udp_rmtlog_be_sd = -1;
static struct sockaddr_in address;
evl_callback_t evl_cb = NULL;
static char udp_rmtlog_be_conf_path[] = EVLOG_PLUGIN_CONF_DIR "/udp_rmtlog_be.conf";
static int udp_rmtlog_be_Failed = FALSE;

/* This is the backend function that the backend manager will invoke */
static int udp_rmtlog_be_init(evl_callback_t cb_func);
static int udp_rmtlog_be_func(const char *data1, const char *data2, 
						evl_callback_t cb_func);

/* The Plugin Descriptor must have this name */
be_desc_t _evlPluginDesc = {
	udp_rmtlog_be_init,
	udp_rmtlog_be_func
};

int initSockAddr()
{ 
	char rmt_hostname[80], password[80];
	int pass_len, evl_port;
	FILE *f;
	
	struct in_addr inaddr;
	struct hostent *rmthost;

	TRACE("Loading plug-in.\n");
	
	if (getConfigStringParam(udp_rmtlog_be_conf_path, "Remote Host", rmt_hostname, sizeof(rmt_hostname)) == -1) {
		return -1;
	}
	
	if (getConfigIntParam(udp_rmtlog_be_conf_path, "Port", &evl_port) == -1) {
		evl_port = EVLOG_UDP_PORT;
	}
	/* get host name/ip address */
	if (inet_aton(rmt_hostname, &inaddr)) {
		rmthost = (struct hostent *) gethostbyaddr((char *) &inaddr, sizeof(inaddr), AF_INET);
	}
	else {
		rmthost = (struct hostent *) gethostbyname(rmt_hostname);
	}

	if (!rmthost) {
		if (evl_cb) {
			(* evl_cb) (CB_MK_EVL_REC, "udp_rmtlog_be: Failed to resolve remote host.", NULL);
		}
		fprintf(stderr, "Can't find remote host.\n");
		return -1;
	}	

	if ((udp_rmtlog_be_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		if (evl_cb) {
			(* evl_cb) (CB_MK_EVL_REC, "udp_rmtlog_be: Failed to create datagram socket.", NULL);
		}
		fprintf(stderr, "Failed to create datagram socket.\n");
		return -1;
	}		

	address.sin_family = AF_INET;
	address.sin_port = htons(evl_port);
	 
	memcpy(&address.sin_addr, rmthost->h_addr_list[0], rmthost->h_length);

	return 0;	
}

/*
 * Plugin initialization
 */
void _init()
{

}

void _fini()
{
	if (udp_rmtlog_be_sd != -1) {
		close(udp_rmtlog_be_sd);
		udp_rmtlog_be_sd = -1;
	}
	
	(* evl_cb) (CB_MK_EVL_REC, "udp_rmtlog_be: plugin unloaded.", NULL);
	TRACE("udp_rmtlog_be plugin unloaded.\n");
}

/*************************
 * The backend functions *
 *************************/
/*
 * Initialize the plugin -
 */
static int
udp_rmtlog_be_init(evl_callback_t cb_func)  
{
	char disable[4]= "yes";

	/* first, store the callback function */
	evl_cb = cb_func;
	
	/* Check to see if this plugin is disable */
	getConfigStringParam(udp_rmtlog_be_conf_path, "Disable", 
			  disable, sizeof(disable));
	if(!strcasecmp(disable, "yes")) {
		(* evl_cb) (CB_MK_EVL_REC, "udp_rmtlog_be: plugin is disabled.", NULL);
		return -1;  /* return -1 will unload plugin immediately */
	}
	if (initSockAddr() == -1) {
		udp_rmtlog_be_Failed = TRUE;
		return 0;
	}
	(* evl_cb) (CB_MK_EVL_REC, "udp_rmtlog_be: plugin loaded.", NULL);
	TRACE("udp_rmtlog_be plugin loaded.\n");
	return 0;
}

static int 
udp_rmtlog_be_func(const char *data1, const char *data2, evl_callback_t cb_func)
{
	char buf[NET_REC_HDR_SIZE + POSIX_LOG_ENTRY_MAXLEN];
	struct posix_log_entry *rhdr;
	struct net_rechdr *netrechdr;

	if (udp_rmtlog_be_Failed == TRUE) {
		/* 
		 * Fail to initialize the socket, due to network services
		 * is not available during start up.
		 * Try again. 
		 */ 
		if(initSockAddr() == -1) {
		       return 0;
		}
		else {
			udp_rmtlog_be_Failed = FALSE;
		}
	}
	TRACE("Send data to remote host now...\n");
	rhdr = (struct posix_log_entry *) data1;
	netrechdr = (struct net_rechdr *) buf;
	hton_rechdr(rhdr, netrechdr);
	if (rhdr->log_size > 0) {
		memcpy(buf + NET_REC_HDR_SIZE, data1 + REC_HDR_SIZE, 
				rhdr->log_size);
	}
	sendto(udp_rmtlog_be_sd, buf, rhdr->log_size + NET_REC_HDR_SIZE, 0, 
			(struct sockaddr *) &address, 
			sizeof(struct sockaddr_in));

	return 0;	
}

