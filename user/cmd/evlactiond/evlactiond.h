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


#ifndef _EVL_ACTIOND_H_
#define _EVL_ACTIOND_H_

#define EVLACTIOND_REG_FILE		LOG_EVLOG_CONF_DIR "/action_registry"
#define EVLACTIOND_CONF_FILE	LOG_EVLOG_CONF_DIR "/action_profile"
#define ACTION_DOT_ALLOW		LOG_EVLOG_CONF_DIR "/action.allow"
#define ACTION_DOT_DENY			LOG_EVLOG_CONF_DIR "/action.deny"

#define SMALL_BUF_SIZE		1024

typedef enum _nfy_cmd {
	nfyCmdAdd,
	nfyCmdRemove,
	nfyCmdChange,
	nfyCmdListAll,
	nfyListByName,
	nfyRemoveByName
} nfy_cmd_t;

typedef struct _nfy_info {
	posix_log_recid_t recid;
	time_t timestamp;
} nfy_info_t;

typedef struct _any {
	char * ident;
	evl_listnode_t *hist;
} any_t;	
	
typedef struct _nfy_action_hdr {
	int 	cmd_type;
	int 	nfy_id;
	uid_t	uid;
	gid_t	gid;
	int 	flags;				/* once_only, persist */
	int 	qu_strLen;
	int	action_cmdLen;
	int	name_len;
	int 	any_str_len;
	int	thres;
	unsigned int interval;
	int 	query_flags;
} nfy_action_hdr_t;

typedef struct nfy_action {
	HANDLE 	handle;	
	nfy_action_hdr_t hdr;
	char * qu_str;
	char * cmd;
	char * name;
	char * any_str;
	evl_listnode_t *any_list;
	int state;
} nfy_action_t;


#endif /* end _EVL_ACTIOND_H_ */

