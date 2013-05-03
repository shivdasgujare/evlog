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
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>       	
#include <fcntl.h>
#include <sys/klog.h>
#include <signal.h>           	
#include <errno.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>


#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_list.h"
#include "evl_common.h"
#include "../evlactiond/evlactiond.h"

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stdout, fmt, ##args)
#endif

#ifndef DEBUG2
#define TRACE(fmt, args...)		/* fprintf(stdout, fmt, ##args) */
#endif

/* Function prototype */
int establishActiondConnection(struct sockaddr *sa, char * sockpathname);
uid_t parseUID(const char *uid_str, int * result);
void processActionCmd(int sd);
int addNfyAction(int sd, char * queryStr, char *actionCmd, char *name,
		char *any_str, int flags);
int removeNfyAction(int sd, int id);
int modifyNfyAction(int sd, int id, char *action_cmd);
/*int listPersistedNotification();*/
int listAllNotification(int sd);
void batchCommand(int sd, const char * cmdFilePath);
size_t splitCmd(char * cmdbuf, size_t n, char **argv);
void resetOpts();


#define MAX_NFYID_ARRAY		256

char *progName = 0;

/* String args */
char *filterStr = NULL;
char *cmdStr = NULL;
char *name = NULL;
char *showName = NULL;
char *cmdFilePath = NULL;
char *cmdStrToChg = NULL;
char *anyStr = NULL;
char *uid_str = NULL;
char *interval = NULL;


/* Boolean args */
int listFlag = 0;
int helpFlag = 0;
int persistFlag = 0;
int onceOnlyFlag = 0;
int removeFlag = 0;
int showAllFlag = 0;
int somethingBesidesHelp = 0;

/* Integer args */
int rm_nfyIDArray[MAX_NFYID_ARRAY];
int change_nfyID = -1;
int thres = 0;

int interval_sec = 0;

#define BOOLARG 	1
#define STRINGARG 	2
#define INTARG 		3
#define	INTARRAYARG	4

struct argInfo {
	char	*argName;
	char 	*shrtName;
	void	*argVal;
	int	argType;
	int	argSeen;
};

struct argInfo argInfo[] = {
	{ "--list",	"-l",		&listFlag,	BOOLARG,	0},
	{ "--filter",	"-f",		&filterStr,	STRINGARG,	0},
	{ "--name",	"-n",		&name,		STRINGARG,	0},
	{ "--any",	"-y",		&anyStr,	STRINGARG,	0},
	{ "--show",	"-s",		&showName,	STRINGARG,	0},
	{ "--showall", "-S",		&showAllFlag, 	BOOLARG, 	0},
	{ "--help",	"-h",		&helpFlag,	BOOLARG,	0},
	{ "--persistent","-p",		&persistFlag,	BOOLARG,	0},
	{ "--add",	"-a",		&cmdStr,	STRINGARG,	0},
	{ "--file",	"-F",		&cmdFilePath,	STRINGARG,	0},
	{ "--once-only","-o",		&onceOnlyFlag,	BOOLARG,	0},
	{ "--delete",	"-d",		&removeFlag,	BOOLARG,	0},
	{ "--change",	"-c",		&cmdStrToChg,	STRINGARG,	0},
	{ "--uid",	"-u",		&uid_str,	STRINGARG,	0},
	{ "--threshold", "-t",		&thres, 	INTARG, 	0},
	{ "--interval", "-i", 		&interval, 	STRINGARG, 	0},
	{ NULL,			NULL,		NULL,			0,			0}
};


void usage()
{
	fprintf(stderr, "Usage:\n"
			"evlnotify -l | --list\n"
			"evlnotify -s | --show <rulename>\n"
			"evlnotify -S | --showall\n"
			"evlnotify -a | --add <notify-action> [-o | --once-only]\n"
			"          [-f | --filter <filter>]\n"
			"          [-p | --persistent] [-u | --uid <userid>]\n"
		        "          [-t | --threshold <cnt> -i | --interval <n[s|m|h|d]>]\n" 
			"          [-n | --name <rulename>]\n"
			"          [-y | --any <attribute name in template>]\n"
			"evlnotify -d | --delete <notify-id ...>\n"
			"evlnotify -c | --change <new-notify-action notify-id>\n"
			"evlnotify -F | --file <cmd-file>\n");
	exit(1);
}

/*
 * Reset all options to initial state
 */
void
resetOpts() {
	int i;
	struct argInfo *ai;
	
	for(ai = argInfo; ai->argName; ai++) {
		ai->argSeen = 0;
	}
	
	filterStr = NULL;
	cmdStr = NULL;
	name = NULL;
	showName = NULL;
	anyStr = NULL;
	cmdFilePath = NULL;
	cmdStrToChg = NULL;
	uid_str = NULL;
	interval = NULL;
	
/* Boolean args */
	listFlag = 0;
	helpFlag = 0;
	persistFlag = 0;
	onceOnlyFlag = 0;
	removeFlag = 0;
	showAllFlag = 0;
	somethingBesidesHelp = 0;

/* Integer args */
	for (i = 0 ; i < MAX_NFYID_ARRAY; i++) {
		rm_nfyIDArray[i] = -1;
	}	
	
	change_nfyID = -1;
	thres = 0;
}

struct argInfo *
findArg(const char *argName)
{
	struct argInfo *ai;
	for (ai = argInfo; ai->argName; ai++) {
		if (!strcmp(argName, ai->argName) || !strcmp(argName, ai->shrtName)) {
			TRACE("argName=%s\n", argName);
			return ai;
		}
	}
	return NULL;
}

/*
 * Processes the arg list and stores the values of the various options in
 * the appropriate variables.  Detects duplicate args, args without values,
 * etc.  No real semantic checks -- e.g., -timeout without -new.
 */
void
processArgs(int argc, char **argv)
{
	struct argInfo *ai;
	int n;

	for (++argv; *argv; ++argv) {
		char *arg = *argv;
		ai = findArg(arg);
		if (!ai) {
			fprintf(stderr, "%s: Unknown option: %s\n", progName, arg);
			usage();
		}

		if (ai->argSeen) {
			fprintf(stderr,"%s: Multiple %s options not supported\n",
				progName, arg);
			exit(1);
		}
		ai->argSeen = 1;

		if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
			somethingBesidesHelp = 1;
		}

		if (ai->argType != BOOLARG) {
			argv++;
			arg = *argv;
			if (!arg) {
				fprintf(stderr,
					"%s: Missing value for %s option\n",
					progName, ai->argName);
				usage();
			}
		}

		switch (ai->argType) {
    		case BOOLARG:
    		{
    			*((int*)(ai->argVal)) = 1;
    			if (!strcmp(ai->argName, "--delete") | !strcmp(ai->shrtName, "-d")) {
    				for (n = 0; n < MAX_NFYID_ARRAY ;n++) {
        		    	char *endChar = 0;
        		    	long val;
        		    	argv++;
        				arg = *argv;
        				if (arg == NULL) {
        					return;
        				}
            		    val = strtol(arg, &endChar, 0);
            			if (*endChar != '\0' || val == -1) {
            				fprintf(stderr,"%s: Illegal value for %s option\n",
            					progName, ai->argName);
            				usage();
            			}
            			
            			rm_nfyIDArray[n] = (int)val;	
        			}	
    			}
    			break;
    		}
    		case STRINGARG:
    		{
    			char *endChar = 0;
    	
    			*((char**)(ai->argVal)) = arg;
    			if (!strcmp(ai->argName, "--change") || !strcmp(ai->shrtName, "-c")) {
    				argv++;
    				arg = *argv;
    				change_nfyID = (int) strtol(arg, &endChar, 0);
    		    } 	
    			break;
    		}
    		case INTARG:
    		{
    			char *endChar = 0;
    		    long val = strtol(arg, &endChar, 0);
    			if (*endChar != '\0' || val == -1) {
    				fprintf(stderr,
    					"%s: Illegal value for %s option\n",
    					progName, ai->argName);
    				usage();
    			}
    			*((int*)(ai->argVal)) = (int) val;
    		    break;
    		}
    		default:
    			fprintf(stderr,"%s: Internal error handling %s option\n", progName, arg);
    			exit(2);
		} /*end switch */
	} /* end for */
}

void
semanticError(const char *msg)
{
	fprintf(stderr, "%s: %s\n", progName, msg);
	exit(1);
}

static void
badOptionCombo(const char *opt1, const char *opt2)
{
	struct argInfo *ai1 = findArg(opt1);
	struct argInfo *ai2 = findArg(opt2);
	assert(ai1 != NULL);
	assert(ai2 != NULL);
	if (ai1->argSeen && ai2->argSeen) {
		char msg[200];
		snprintf(msg, sizeof(msg), "Cannot specify %s with %s.", opt1, opt2);
		semanticError(msg);
	}
}

long parseInterval(char * in)
{
	long inter;
	char *endchar = 0;
	long i = strtol(in, &endchar, 0);
	if (i != LONG_MIN && i != LONG_MAX) {
		if (*endchar == 's' || *endchar == 'S'
		    || *endchar == '\0') {
       			inter = i;
		} else if (*endchar == 'm' || *endchar == 'M') {
			inter = i * 60;
		} else if (*endchar == 'h' || *endchar == 'H') {
			inter = i * 60 * 60;
		} else if (*endchar == 'd' || *endchar == 'D') {
			inter = i * 60 * 60 * 24;
		} else {
			return 0;
		}
		
	} else {
	 	return 0;
	}
	return inter;
}	
	
				

/*
 * The command line has been parsed.  Verify that the selected combination
 * of options, and the values of those options, make sense.  We finish most
 * of our initialization work as a by-product.
 */
void
checkSemantics()
{
	int status;

	/*
	 * If -help is specified, print the usage message.  It's an error
	 * if any other options are specified with -help.
	 */
	if (helpFlag) {
		usage();
		exit(somethingBesidesHelp ? 1 : 0);
	}
	else if (listFlag) {
		badOptionCombo("--list", "--add");
		badOptionCombo("--list", "--delete");
		badOptionCombo("--list", "--change");
		badOptionCombo("--list", "--file");
		badOptionCombo("--list", "--help");
		badOptionCombo("--list", "--showall");
		badOptionCombo("--list", "--show");
	}	
	else if (showAllFlag) {
		badOptionCombo("--showall", "--add");
		badOptionCombo("--showall", "--delete");
		badOptionCombo("--showall", "--change");
		badOptionCombo("--showall", "--file");
		badOptionCombo("--showall", "--help");
		badOptionCombo("--showall", "--list");
	}	
	else if (showName) {
		badOptionCombo("--show", "--add");
		badOptionCombo("--show", "--delete");
		badOptionCombo("--show", "--change");
		badOptionCombo("--show", "--file");
		badOptionCombo("--show", "--help");
		badOptionCombo("--show", "--list");
		badOptionCombo("--show", "--showall");
		
		if (strchr(showName, ' ')) {
			fprintf(stderr, "ERROR: name should not contain space\n");
			exit(1);
		}

	}
	else if (cmdStr) {
		uid_t euid;
		gid_t egid;
		struct passwd *pw;
		int n;
		int result = 0;

		badOptionCombo("--add", "--list");
		badOptionCombo("--add", "--delete");
		badOptionCombo("--add", "--change");
		badOptionCombo("--add", "--file");
		badOptionCombo("--add", "--help");
		if ((interval && thres==0) || 
		    (interval == NULL && thres > 0)) {
			fprintf(stderr, "ERROR: Both threshold and interval must be set.\n");
			exit(1);
		}
		if (interval) {
			if ((interval_sec = parseInterval(interval)) == 0) {
				fprintf(stderr, "ERROR: Failed to parse interval value. Please re-enter.\n");
				exit(1);
			}
			if (thres < 0 ) {
				fprintf(stderr, "ERROR: Can't have negative threshold.\n");
				exit(1);
			}
		}

		/* --any option requires --name option */
		if (anyStr && !name) {
			fprintf(stderr, "ERROR: '--any' option requires '--name' option.\n");
			exit(1);
		}
		
			
		if (uid_str) {
			int i, found = 0;
			uid_t uid = getuid();
			if (uid != 0) {
				fprintf(stderr, "ERROR: Only root is allowed to specify the -uid or -gid option.\n");
				exit(1);
			} else {
				euid = parseUID(uid_str, &result);
				if (result == -1) {
					fprintf(stderr, "ERROR: %s is not a valid user", uid_str);
					exit(1);
				}
				pw = getpwuid(euid);
				if (pw == NULL) {
					fprintf(stderr,"ERROR: Invalid user ID %i\n", euid);
					exit(1);
				}
				egid = pw->pw_gid;
				/* set effective gid to user gid */
				if (setegid(egid) == -1) {
					perror("setegid") ;
				}
				
				if (initgroups(pw->pw_name, egid) == -1) {
					perror("initgroups");
					exit(1);
				}
				/* set effective uid */
				if (seteuid(euid) == -1) {
					perror("seteuid");
					exit(1);
				}
				
			}
		}
	}	
	else if (removeFlag) {
		badOptionCombo("--delete", "--show");
		badOptionCombo("--delete", "--showall");
		badOptionCombo("--delete", "--list");
		badOptionCombo("--delete", "--add");
		badOptionCombo("--delete", "--change");
		badOptionCombo("--delete", "--file");
		badOptionCombo("--delete", "--help");
		
	}
	else if (cmdStrToChg) {
		badOptionCombo("--delete", "--show");
		badOptionCombo("--delete", "--showall");
		badOptionCombo("--change", "--add");
		badOptionCombo("--change", "--list");
		badOptionCombo("--change", "--delete");
		badOptionCombo("--change", "--file");
		badOptionCombo("--change", "--help");	
		if (change_nfyID == -1) {
			usage();
		}
	}
	else if (cmdFilePath) {
		badOptionCombo("--delete", "--show");
		badOptionCombo("--delete", "--showall");
		badOptionCombo("--file", "--add");
		badOptionCombo("--file", "--list");
		badOptionCombo("--file", "--delete");
		badOptionCombo("--file", "--help");
		badOptionCombo("--file", "--change");		
	}
	else {
		usage();
	}			
}


int
main (int argc, char *argv[])
{
	int sd = 0;				
	struct sockaddr sock;	
    int flags, n;
	
	/* Parse command line option */
	if (argc < 2) {
		usage();
	}
	resetOpts();
	processArgs(argc, argv);
	checkSemantics();
	
	if((sd = establishActiondConnection(&sock, EVLACTIOND_SERVER_SOCKET)) <= 0) {
		fprintf(stderr, "Failed to comminucate with evlactiond daemon.\n");
		exit(1);
	}

	processActionCmd(sd);
	
	exit(0);
} /* end main */

/*
 * FUNCTION	: processActionCmd
 *
 * PURPOSE	: process each command (add, remove, modify, list)
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
void
processActionCmd(int sd)
{
	int flags;
	int n;
	
	if (cmdStr) {
		char buf[sizeof(nfy_action_hdr_t)];
		flags = 0;
		
		if (onceOnlyFlag) {
			flags |= POSIX_LOG_ONCE_ONLY;
		}
		if (persistFlag) {
			flags |= POSIX_LOG_ACTION_PERSIST;
		}
		if (!filterStr) {
			filterStr = strdup("<null>");  	/* match every thing */
		}
		if (!name) {
			name = strdup("noname");
		}
		if (!anyStr) {
			anyStr = strdup("no_any_str");
		}
		if (name && strcmp(name, "noname") && ruleExists(sd, (nfy_action_hdr_t *) buf, name)) {
			fprintf(stderr, "ERROR: Rule '%s' already exists.\n", name);
			exit(1);
		}
		addNfyAction(sd, filterStr, cmdStr, name, anyStr, flags);
	}
	else if (rm_nfyIDArray[0] != -1) {
    	
    		for(n =0; n < MAX_NFYID_ARRAY; n++) {
    			if (rm_nfyIDArray[n] != -1) {
				TRACE("Removing id=%d\n", rm_nfyIDArray[n]);
    				if (removeNfyAction(sd, rm_nfyIDArray[n]) == -1) {
    					fprintf(stderr, "evlnotify: Failed to remove notify id=%d.\n", rm_nfyIDArray[n]);
    					fprintf(stderr, "The notification may be registered under different user.\n");
    				}	
    			}
		}
    	}
	else if ( change_nfyID != -1) {
    		modifyNfyAction(sd, change_nfyID, cmdStrToChg);	
	}
	else if (cmdFilePath) {
		batchCommand(sd, cmdFilePath);
	}
	else if (listFlag) {
		if (listAllNotification(sd) == -1) {
			fprintf(stderr, "ERROR: The list command did not completely list all registered events\n");
		}
	}
	else if (showName) {
		if (!strcmp(showName, "noname")) 
			return;
		listRuleByName(sd, showName);
	}
	else if (showAllFlag) {
		listAllNotification(sd);
	}
}

/*
 * FUNCTION	: parseUID
 *
 * PURPOSE	: convert login to uid if needed
 *
 * ARGS		:	
 *
 * RETURN	: return uid, caller also should check the result
 *			  if uid is valid result = 0, else result = -1
 *
 */
uid_t
parseUID(const char *uid_str, int *result)
{
	char *endChar = 0;
	long val;
	
	struct passwd *pw;
	
	val = strtol(uid_str, &endChar, 0);
	if (*endChar != '\0' || val == -1) {
		pw = getpwnam(uid_str);
		if (pw == NULL) {
			*result = -1;
			return getuid();
		}
		val = pw->pw_uid; 		
	}
	
	pw = getpwuid(val);
	if (pw == NULL) {
	 	*result = -1;
	 	return getuid();
	}
	*result =0;
	return val;	
}

/* 
 * FUNCTION	: addNfyAction
 * 
 * PURPOSE	: asks the action daemon to register an action
 * 
 * ARGS		:	
 * 
 * RETURN	: returns 0 if succeeded, otherwise -1
 * 
 */
int 
addNfyAction(int sd, char * queryStr, char *actionCmd, char *name, 
		char *any_str, int flags)
{
	int n, status = 0;
	char buf_hdr[sizeof(nfy_action_hdr_t)];
	nfy_action_hdr_t * act_hdr;
	char error_str[200];		
	int query_flags = 0;
	
	posix_log_query_t query;
	/* it is ok for user passed in a null query  */
	if(strcmp(queryStr, "<null>")) {
		if (posix_log_query_create(queryStr, POSIX_LOG_PRPS_NOTIFY,
             	&query, error_str, 200) == 0) {
			query_flags = POSIX_LOG_PRPS_NOTIFY;
		} else if (posix_log_query_create(queryStr, 
			POSIX_LOG_PRPS_GENERAL, &query, error_str, 200) == 0) {
			query_flags = POSIX_LOG_PRPS_GENERAL;
		} else if (posix_log_query_create(queryStr,
			EVL_PRPS_TEMPLATE, &query, error_str, 200) == 0) {
			query_flags = EVL_PRPS_TEMPLATE;
		} else {	
			fprintf(stderr, "evlnotify: could not create query string! Error message: \n   %s.\n", error_str);
			return -1;
		}
	}

	act_hdr = (nfy_action_hdr_t *) buf_hdr;

	act_hdr->cmd_type = nfyCmdAdd;
	act_hdr->uid = geteuid();
	act_hdr->gid = getegid();
	act_hdr->flags = flags;			/* once only flag */
	act_hdr->qu_strLen = strlen(queryStr);
	act_hdr->action_cmdLen = strlen(actionCmd);
	act_hdr->name_len = strlen(name);
	act_hdr->any_str_len = strlen(any_str);
	act_hdr->thres = thres;
	act_hdr->interval = interval_sec;
	act_hdr->query_flags = query_flags;
	

	if((n = write(sd, act_hdr, sizeof(nfy_action_hdr_t))) != sizeof(nfy_action_hdr_t)) {
		fprintf(stderr, "evlnotify: Failed to add notification.\n");
		return -1;
	}
	if((n = write(sd, queryStr, act_hdr->qu_strLen)) != act_hdr->qu_strLen) {
		fprintf(stderr, "evlnotify: Failed to send notification's query string to daemon.\n");
		return -1;
	}
	if((n = write(sd, actionCmd, act_hdr->action_cmdLen)) != act_hdr->action_cmdLen) {
		fprintf(stderr, "evlnotify: Failed to send notification's action cmd string to daemon.\n");
		return -1;
	}	

	if((n = write(sd, name, act_hdr->name_len)) != act_hdr->name_len) {
		fprintf(stderr, "evlnotify: Failed to send notification's action cmd string to daemon.\n");
		return -1;
	}	

	if((n = write(sd, any_str, act_hdr->any_str_len)) != act_hdr->any_str_len) {
		fprintf(stderr, "evlnotify: Failed to send notification's action cmd string to daemon.\n");
		return -1;
	}
    	TRACE("Done sending query, cmdline strings\n");

	read(sd, &status, sizeof(status));

	TRACE("addNfyAction return\n");
	return status;
}

/*
 * FUNCTION	: removeNfyAction
 *
 * PURPOSE	: asks the action daemon to remove an action
 *
 * ARGS		:	
 *
 * RETURN	: returns 0 if succeeded, otherwise -1
 *
 */
int
removeNfyAction(int sd, int id)
{
	int status, n;
	char buf[sizeof(nfy_action_hdr_t)];
	nfy_action_hdr_t * action_hdr;
	
	
	action_hdr = (nfy_action_hdr_t *) buf;
	
	action_hdr->cmd_type = nfyCmdRemove;
	action_hdr->nfy_id = id;
	
	if((n = write(sd, action_hdr, sizeof(nfy_action_hdr_t))) != sizeof(nfy_action_hdr_t)) {
		fprintf(stderr, "evlnotify: Failed to remove notification.\n");
		return -1;
	}
	TRACE("remove command sent - id=%d\n", id);
	read(sd, &status, sizeof(status));
	
	return (status < 0)? -1: 0;
}

/*
 * FUNCTION	: modifyNfyAction
 *
 * PURPOSE	: asks the action daemon to modify a action cmd for an action
 *
 * ARGS		:	
 *
 * RETURN	: returns 0 if succeeded, otherwise -1
 *
 */
int
modifyNfyAction(int sd, int id, char *action_cmd)
{
	int status, n;
	char buf[sizeof(nfy_action_hdr_t)];
	nfy_action_hdr_t * action_hdr;
	
	action_hdr = (nfy_action_hdr_t *) buf;
	action_hdr->cmd_type = nfyCmdChange;	
	action_hdr->nfy_id = id;
	action_hdr->action_cmdLen = strlen(action_cmd);
	
	if((n = write(sd, action_hdr, sizeof(nfy_action_hdr_t))) != sizeof(nfy_action_hdr_t)) {
		fprintf(stderr, "evlnotify: Failed to modify notification.\n");
		return -1;
	}
	if((n = write(sd, action_cmd, action_hdr->action_cmdLen)) != action_hdr->action_cmdLen) {
		fprintf(stderr, "evlnotify: Failed to send notification's action cmd string to daemon.\n");
		return -1;
	}	
	read(sd, &status, sizeof(status));
	
	return (status < 0)? -1: 0;
}
/*
 * ruleExist - check to see if rule (rule name) already exists on the system
 * returns 1 = exist, 0 = does not exist or socket failure.
 */
int 
ruleExists(int sd, nfy_action_hdr_t * action_hdr, const char * name)
{
	int n;
	
	action_hdr->cmd_type = nfyListByName;
	action_hdr->name_len = strlen(name);

	if((n = write(sd, action_hdr, sizeof(nfy_action_hdr_t))) != sizeof(nfy_action_hdr_t)) {
		fprintf(stderr, "ERROR: ruleExist() failed to write header notification.\n");
		return 0;
	}
	
	if((n = write(sd, name, action_hdr->name_len)) != action_hdr->name_len) {
		fprintf(stderr, "ERROR: ruleExist() failed to write name_len.\n");
		return 0;
	}
	if (read(sd, (char *)action_hdr, sizeof(nfy_action_hdr_t)) != sizeof(nfy_action_hdr_t)) {
		fprintf(stderr, "ERROR: ruleExist() failed to read action header notification.\n");
		return 0;
	}
	
	if(action_hdr->flags == -1) {
		return 0;
	}
	return 1;
}

char *
getIntervalStr(int interval)
{
	static char inter_str[10];
	int iu;

	if ((iu = interval / (60 * 60 * 24)) > 0) {
		sprintf(inter_str, "%dD", iu);
	} else if ((iu = interval / (60 * 60)) > 0) {
		sprintf(inter_str, "%dH", iu);
	} else if ((iu = interval / 60) > 0) {
		sprintf(inter_str, "%dM", iu);
	} else {
		sprintf(inter_str, "%dS", interval);
	}
	return inter_str;
}
	

int 
listRuleByName(int sd, const char *name)
{
	int status, n;
	char act_buf[512];
	char filter_buf[512];
	char name_buf[80];
	char any_str_buf[256];
	
	char buf[sizeof(nfy_action_hdr_t)];
	nfy_action_hdr_t * action_hdr;
	struct group *grp;
	struct passwd *pw;
	
	action_hdr = (nfy_action_hdr_t *) buf;
	if (!ruleExists(sd, action_hdr, name)) {
		return -1;
	}

	if (read(sd, filter_buf, action_hdr->qu_strLen) != action_hdr->qu_strLen) {
		fprintf(stderr, "evlnotify: listActionByName() failed to read action qu_str.\n");
		return -1;
	}
	filter_buf[action_hdr->qu_strLen] = '\0';
	
	if (read(sd, act_buf, action_hdr->action_cmdLen) != action_hdr->action_cmdLen) {
		fprintf(stderr, "evlnotify: listActionByName() failed to read action cmd.\n");
		return -1;
	}
	act_buf[action_hdr->action_cmdLen] = '\0';
	if (read(sd, name_buf, action_hdr->name_len) != action_hdr->name_len) {
		fprintf(stderr, "evlnotify: listActionByName() failed to read action name.\n");
		return -1;
	}
	name_buf[action_hdr->name_len] = '\0';
	if (read(sd, any_str_buf, action_hdr->any_str_len) != action_hdr->any_str_len) {
		fprintf(stderr, "evlnotify: listActionByName() failed to read action any_str.\n");
		return -1;
	}
    	any_str_buf[action_hdr->any_str_len] = '\0';
	if ((pw = getpwuid(action_hdr->uid)) == NULL) {
    		fprintf(stderr, "Failed to look up the user name.\n");
    	} 
	else {
		if (!strcpy(any_str_buf, "no_any_str")) {
			fprintf(stdout, "id=%u\n%s {\n\tfilter = '%s'\n\tthreshold = %d\n\tinterval = %s\n}\n",
					action_hdr->nfy_id, name_buf,
					filter_buf,
					action_hdr->thres,
					getIntervalStr(action_hdr->interval));
		} else {				
	
			fprintf(stdout, "id=%u\n%s {\n\tfilter = '%s'\n\tthreshold = %d\n\tinterval = %s\n\tforany = \"%s\"\n}\n",
					action_hdr->nfy_id, name_buf,
					filter_buf,
					action_hdr->thres,
					getIntervalStr(action_hdr->interval),
					any_str_buf);
		}		
	}
	return 0;

}
		
		
/*
 * FUNCTION	: listAllNotification
 *
 * PURPOSE	: asks the action daemon for all the registered actions
 *
 * ARGS		:	
 *
 * RETURN	: returns 0 if succeeded, otherwise -1
 *
 */	
int
listAllNotification(int sd)
{
	int status, n;
	char act_buf[512];
	char filter_buf[512];
	char name_buf[80];
	char any_str_buf[256];
	
	char buf[sizeof(nfy_action_hdr_t)];
	nfy_action_hdr_t * action_hdr;
	struct group *grp;
	struct passwd *pw;
	
	action_hdr = (nfy_action_hdr_t *) buf;
	action_hdr->cmd_type = nfyCmdListAll;	
	
	if((n = write(sd, action_hdr, sizeof(nfy_action_hdr_t))) != sizeof(nfy_action_hdr_t)) {
		fprintf(stderr, "evlnotify: listAllNotification() failed to write header notification.\n");
		return -1;
	}
	
	for(;;) {
		if (read(sd, buf, sizeof(nfy_action_hdr_t)) != sizeof(nfy_action_hdr_t)) {
			fprintf(stderr, "evlnotify: listAllNotification() failed to read action header notification.\n");
			return -1;
		}
		action_hdr = (nfy_action_hdr_t *) buf;
		
		if(action_hdr->nfy_id == 0) {
			break;
		}
			
		if (read(sd, filter_buf, action_hdr->qu_strLen) != action_hdr->qu_strLen) {
			fprintf(stderr, "evlnotify: listAllNotification() failed to read action qu_str.\n");
			return -1;
		}
		filter_buf[action_hdr->qu_strLen] = '\0';
		
		if (read(sd, act_buf, action_hdr->action_cmdLen) != action_hdr->action_cmdLen) {
			fprintf(stderr, "evlnotify: listAllNotification() failed to read action cmd.\n");
			return -1;
		}
		act_buf[action_hdr->action_cmdLen] = '\0';
		if (read(sd, name_buf, action_hdr->name_len) != action_hdr->name_len) {
			fprintf(stderr, "evlnotify: listAllNotification() failed to read action name.\n");
			return -1;
		}
		name_buf[action_hdr->name_len] = '\0';
		if (read(sd, any_str_buf, action_hdr->any_str_len) != action_hdr->any_str_len) {
			fprintf(stderr, "evlnotify: listAllNotification() failed to read action any_str.\n");
			return -1;
		}
		any_str_buf[action_hdr->any_str_len] = '\0';
		if ((pw = getpwuid(action_hdr->uid)) == NULL) {
			fprintf(stderr, "Failed to look up the user name.\n");
		} else {
			if (listFlag) {
				fprintf(stdout,"%u:%s:%s:%s:%u:%d:%d:%u\n", action_hdr->nfy_id,
						filter_buf,
 						act_buf,
    						pw->pw_name,
    						action_hdr->flags & POSIX_LOG_ONCE_ONLY,
						action_hdr->thres,
						action_hdr->interval,
    						(action_hdr->flags & POSIX_LOG_ACTION_PERSIST) >> 4
						);
			} 
			else if ( showAllFlag) { 
				if (!strcmp(name_buf, "noname")) {
					continue;
				}
				if (!strcmp(any_str_buf, "no_any_str")) {
					fprintf(stdout, "id=%u\n%s {\n\tfilter = '%s'\n\tthreshold = %d\n\tinterval = %s\n}\n",
						action_hdr->nfy_id, name_buf,
						filter_buf,
						action_hdr->thres,
						getIntervalStr(action_hdr->interval));
				} else {
					fprintf(stdout, "id=%u\n%s {\n\tfilter = '%s'\n\tthreshold = %d\n\tinterval = %s\n\tforany = \"%s\"\n}\n",
						action_hdr->nfy_id, name_buf,
						filter_buf,
						action_hdr->thres,
						getIntervalStr(action_hdr->interval),
						any_str_buf);
				}
			}	
		}
	}
	if (action_hdr->flags == 0)
		return 0;
	else
		return -1;
}

/*
 * FUNCTION	: batchCommand
 *
 * PURPOSE	: batch processing evnotify command.
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
void
batchCommand(int sd, const char *infile)
{
	char cmd[SMALL_BUF_SIZE];
	char line[SMALL_BUF_SIZE];
	FILE *f;
	char *args[64];
	int nargs;
	
	if((f = fopen(infile, "r")) == NULL) {
		perror("fdopen");
		fclose(f);
		exit(1);
	}
	
	while (fgets(line, SMALL_BUF_SIZE, f) != NULL) {
		if(line[0] == '#' || line[0] == '\n' || strcspn(line, " ") == 0) {
			continue;
		}
		/* replace line newline char with null */
		if (line[strlen(line) -1] == '\n') {
			line[strlen(line) -1] = '\0';
		}
		snprintf(cmd, sizeof(cmd), "/sbin/evlnotify %s", line);
		nargs = _evlSplitCmd(cmd, 64, args);
		args[nargs] = NULL;
		
		if (nargs < 2) {
			fprintf(stderr, "Missing command line options.\n");
			continue;
		}
		if (!strcmp(args[1], "--file") || !strcmp(args[1], "-F")) {
			fprintf(stderr, "--file option is not allowed.\n");
			continue;
		}
		resetOpts();
		processArgs(nargs, args);
		checkSemantics() ;
		processActionCmd(sd);
		
	}
	fclose(f);	
} 	

/* 
 * FUNCTION	: establishActiondConnection
 * 
 * PURPOSE	:
 * 
 * ARGS		:	
 * 
 * RETURN	:
 * 
 */
int 
establishActiondConnection(struct sockaddr *sa, char * sockpathname)
{
	int sd;
	size_t sock_len;				/* Size of generic UD address structure */
	int retry;
	int flags;
	char abyte;
#if 0	
	/*
	 * Open the notify daemon
	 */
	if ((sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0){
		(void)fprintf(stderr, "Cannot create a connection to the action daemon.\n");
		return -ESOCKTNOSUPPORT;
	}
	/*
	 * Request connect to the specified socket address.  In this case,
	 * UNIX Domain format, The socket address is really just a file.
	 */

	sa->sa_family = PF_UNIX;
	(void) strcpy(sa->sa_data, sockpathname);
	sock_len = sizeof(sa->sa_family) + strlen(sa->sa_data);

	/*
	 * connect will be retried 5 times to avoid a timing issue.
	 */
	retry = 0;
	while (connect(sd, sa, sock_len) < 0) {
		retry++;
		if (retry==4) {
			(void)fprintf(stderr, "Cannot connect to the action daemon.\n");
			return -ECONNREFUSED;
		}
		(void) sleep(1);
	}
#endif
	if ((sd = _establishNonBlkConnection(sockpathname, (struct sockaddr_un *)sa, 1)) < 0) {
		(void)fprintf(stderr, "Cannot connect to the action daemon.\n");
		exit(1);
	}
	read(sd, &abyte, sizeof(char));
	if (abyte == (char) NFY_ACCESS_DENIED) {
		fprintf(stderr, "Access denied.\n");
		exit(1);
	} else if (abyte == (char) NFY_MAX_CLIENTS) {
		fprintf(stderr, "Max number of clients reached. Try again later!\n");
		exit(1);
	}
	return sd;
}



	

