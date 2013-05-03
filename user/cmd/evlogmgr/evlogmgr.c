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
#include <fcntl.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <values.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>		/* Defines bcopy() */
#include <sys/stat.h>
#include <time.h>
#include <assert.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_logmgmt.h"

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stdout, fmt, ##args)
#else
#define TRACE(fmt, args...)		/* fprintf(stdout, fmt, ##args) */
#endif


#define BOOLARG 	1
#define STRINGARG 	2
#define INTARG 		3

char *progName = 0;
char *logfile;
/* String args */
char *cfilterStr = NULL;
char *showstat = NULL;
char *logFilePath = NULL;


int privFlag = 0;
int forceFlag = 0;

int repairFlag = 0;
int helpFlag = 0;
int somethingBesidesHelp = 0;

int gzBackUpFlag = 0;

struct argInfo {
	char	*argName;
	char 	*shrtName;
	void	*argVal;
	int	argType;
	int	argSeen;
};

struct argInfo argInfo[] = {
	{ "--private",		"-p", 		&privFlag, 			BOOLARG, 	0},	
	{ "--force",		"-F", 		&forceFlag, 		BOOLARG, 	0},	
	{ "--show-status",	"-s", 		&showstat, 			STRINGARG, 	0},	
	{ "--compact",		"-c",		&cfilterStr,		STRINGARG,	0},
	{ "--fix",			"-f", 		&repairFlag, 		BOOLARG, 	0},	
	{ "--log",			"-l", 		&logFilePath, 		STRINGARG, 	0},	
	{ "--compr-bak",	"-C",		&gzBackUpFlag, 		BOOLARG,	0},
	{ "--help",			"-h",		&helpFlag, 			BOOLARG,	0},
	{ NULL,				NULL,		NULL,			0,			0}
};

void usage()
{
	fprintf(stderr, "Usage:\n"
					"evlogmgr  -c | --compact <filter> [-F | --force] [-C | compr-bak]\n"
					"         [[-p | --private] | [-l | --log <logfilepath>]]\n"
					"evlogmgr  -f | --fix\n" 
					"         [[-p | --private] | [-l | --log <logfilepath>]]\n"
					"evlogmgr  -s | --show-status <filter>\n"
					"         [[-p | --private] | [-l | --log <logfilepath>]]\n");
	exit(1);
}

struct argInfo *
findArg(const char *argName)
{
	struct argInfo *ai;
	for (ai = argInfo; ai->argName; ai++) {
		if (!strcmp(argName, ai->argName) || !strcmp(argName, ai->shrtName)) {
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

		if (strcmp(arg, "--help") && strcmp(arg, "-h")) {
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
    			break;
    		}
    		case STRINGARG:
    		{
    			char *endChar = 0;
    	
    			*((char**)(ai->argVal)) = arg;
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
	else if (cfilterStr) {
		badOptionCombo("--compact", "--show-status");
		badOptionCombo("--compact", "--fix");
	}	
	else if (showstat) {
		badOptionCombo("--show-status", "--fix");
		badOptionCombo("--show-status", "--compr-bak");
	}
	else if (repairFlag) {
		badOptionCombo("--fix", "--force");	
	}
	else {
		usage();
	}			
}


void 
processCommand()
{
	int num_delrec;
	size_t delbytes;
	int flags = 0;
	
	if (logFilePath) {
		logfile = logFilePath;
	}
	else if (privFlag) {
		logfile = LOG_PRIVATE_PATH;
	}
	else {
		logfile = LOG_CURLOG_PATH;
	}
	if (cfilterStr) {
		if (forceFlag) {
			flags |= EVL_FORCE_COMPACT;
		}
		evl_compact_log(logfile, cfilterStr, flags);
	}
	else if (showstat) {
		
		if (evlGetNumDelRec(logfile, showstat, &num_delrec, &delbytes) == 0) {
			float kbytes = delbytes/1024;
			fprintf(stdout, "Number of records matching the filter is %d.\n"
							"Log file size would be reduced by %2.2f kbytes.\n", 
							 num_delrec, kbytes);	
		}	
	}
	else if (repairFlag) {
		evl_repair_log(logfile);
	}
		
}

void SIGTerm_handler()
{
	restoreLogfileFromBackup(logfile);
	exit(1);
}

int
main (int argc, char *argv[])
{
	static struct sigaction SigAllAction; /* Signal handler to terminate gracefully */
	static struct sigaction act;
	void SIGTerm_handler();
	
	uid_t uid = getuid();
	/* 
	 * Only root is allowed to run this tool, we check it here and also 
	 * install the binary with root permission only.
	 */
	if (uid != 0) {
		fprintf(stderr, "Only root is allowed to execute this utility.\n");
		exit(1);
	}
	/* Parse command line option */
	if (argc < 2) {
		usage();
	}
	
	processArgs(argc, argv);
	checkSemantics();

	/*
	 * Create new signal handler for SIGTERM. This will remove dr_socket and
	 * terminate gracefully.
	 */
	(void)memset(&SigAllAction, 0, sizeof(struct sigaction));
	SigAllAction.sa_handler = SIGTerm_handler;
	SigAllAction.sa_flags = 0;

	if (sigaction(SIGTERM, &SigAllAction, NULL) < 0){
		fprintf(stderr,
			"%s: WARNING - sigaction failed for SIGTERM.\n", argv[0]); 
	}
	if (sigaction(SIGINT, &SigAllAction, NULL) < 0){
		fprintf(stderr,
			"%s: WARNING - sigaction failed for new SIGINT.\n", argv[0]); 
	}
	if (sigaction(SIGHUP, &SigAllAction, NULL) < 0){
		fprintf(stderr,
			"%s: WARNING - sigaction failed for new SIGHUP.\n", argv[0]);
	}

	processCommand();
	return 0;
} 


