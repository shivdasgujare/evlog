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

/*
 * Note: If TEST_EVREC_SPRINTF is defined, we handle the -S/-F options
 * using evl_format_evrec_sprintf.  This is a less efficient approach, but
 * it's a convenient way to test that function.
 */
/* #define TEST_EVREC_SPRINTF 1 */

/* #define TEST_LOGMAINT 1 */
#ifdef TEST_LOGMAINT
#define LMDEBUG(fmt, args...)	fprintf(stderr, fmt, ##args)
#else
#define LMDEBUG(fmt, args...)	do {} while(0)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <locale.h>
#include <time.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_list.h"
#include "evl_util.h"
#include "evl_template.h"

extern const char *evl_time_format;
extern const char *_evlGetHostNameEx();
extern char *getFormatFromString(char *s);
extern char *getFormatFromFile(const char *path);
extern char *parseSeparator(char *sep);

static void seekToEnd();
static void awaitEndOfLogMaint();
static void handleReadOrSeekFailure(int status, const char *operation,
	int resyncAction);

char *progName = 0;

/* String args */
char *filterStr = NULL;
char *logPath = NULL;
char *outPath = NULL;
char *separator = NULL;
char *specFmtPath = NULL;
char *specFmtStr = NULL;
char *dateFmtStr = NULL;

/* Boolean args */
int compact = 0;
int helpFlag = 0;
int newFlag = 0;
int private = 0;
int reverse = 0;
int somethingBesidesHelp = 0;
int acceptNonStdAtts = 0;
int formatWithoutTemplates = 0;
int syslogFlag = 0;
int nmeqvalFlag = 0;

/* Integer args */
int nTailRecs = -1;
int timeout = -1;
int newlines = -1;
int initialRecid = -1;

/* Globals derived from args */

#define FMT_BIN 0x1
#define FMT_TEXT 0x2
#define FMT_DEFAULT FMT_TEXT
#define FMT_SPEC (FMT_TEXT|0x4)
#define FMT_SYSLOG (FMT_TEXT|0x8)
#define FMT_NEQV (FMT_TEXT|0x10)
int format = FMT_DEFAULT;

posix_log_query_t query;
posix_log_query_t nfyQuery;
posix_log_query_t *filter = NULL;
posix_log_query_t *nfyFilter = NULL;
posix_logd_t logdesc;	/* Read from here */
int outFd = 1;		/* Write to here */
FILE *outFile = NULL;
char *defaultSeparator = ", ";

/* for -S and -F */
char *specialFormat = NULL;
#ifdef TEST_EVREC_SPRINTF
char *specialFormat2 = NULL;
#endif
evl_list_t *parsedSpecialFmt;
int needTemplateForSpecialFormat = 0;

char *fdbuf;	/* Formatted data goes here. */
size_t fdbufLen = 0;

/*
 * These counters are used with --new.  If nfyCount > writeCount, it means
 * some events of interest have arrived and we haven't written them out yet.
 */
long nfyCount = 0;
long writeCount = 0;

/*
 * These guys are used to help in recovery from interruption by a
 * log-maintenance operation.  LMRESYNC_* specify where to put the read
 * pointer after recovering: start of log (BOF), end of log, or just
 * before or after the last record successfully copied.
 *
 */
posix_log_recid_t lastRecordCopied = -1;
int logGeneration = 0;
int viewingActiveLog = 1;
int registeredForEndMaintEvents = 0;
#define LMRESYNC_START	1
#define LMRESYNC_END	2
#define LMRESYNC_BEFORELAST	3
#define LMRESYNC_AFTERLAST	4

/* Notification signal handler for --new */
static void
nfyHandler(int signo, siginfo_t *info, void *context)
{
	nfyCount++;
}

/* Notification signal handler for POSIX_LOG_MGMT_ENDMAINT events */
static void
endMaintHandler(int signo, siginfo_t *info, void *context)
{
	logGeneration++;
}

static void
reportReadOrSeekFailure(int status, const char *operation)
{
	errno = status;
	perror(operation);
	exit(2);
}

/* Fuss for parsing command line */
#define BOOLARG 1
#define STRINGARG 2
#define INTARG 3

struct argInfo {
	char	*argName;
	char 	*shrtName;
	void	*argVal;
	int	argType;
	int	argSeen;
};

struct argInfo argInfo[] = {
	{ "--templates","-b",	&acceptNonStdAtts,	BOOLARG,	0},
	{ "--notemplates","-B",	&formatWithoutTemplates,	BOOLARG,	0},
	{ "--compact",	"-c",	&compact,	BOOLARG,	0},
	{ "--datefmt",	"-d",	&dateFmtStr,	STRINGARG,	0},
	{ "--filter",	"-f",	&filterStr,	STRINGARG,	0},
	{ "--formatfile","-F",	&specFmtPath,	STRINGARG,      0},
	{ "--help",	"-h",	&helpFlag,	BOOLARG,	0},
	{ "--log",	"-l",	&logPath,	STRINGARG,      0},
	{ "--syslog",	"-m",	&syslogFlag,	BOOLARG,	0},
	{ "--new",	"-n",	&newFlag,	BOOLARG,	0},
	{ "--newlines",	"-N",	&newlines,	INTARG,		0},
	{ "--out",	"-o",	&outPath,	STRINGARG,	0},
	{ "--private",	"-p",	&private,	BOOLARG,	0},
	{ "--nmeqval",	"-q",	&nmeqvalFlag,	BOOLARG,	0},
	{ "--reverse",	"-r",	&reverse,	BOOLARG,        0},
	{ "--recid",	"-R",	&initialRecid,	INTARG,		0},
	{ "--separator","-s",	&separator,	STRINGARG,      0},
	{ "--formatstr","-S",	&specFmtStr,	STRINGARG,      0},
	{ "--tail",	"-t",	&nTailRecs,	INTARG,		0},
	{ "--timeout",	"-T",	&timeout,	INTARG,		0},
	{ NULL,		NULL,	NULL,		0,		0}
};

static void
printUsage()
{
	fprintf(stderr,
"Usage:\n"
"%s --help\n"
" OR\n"
"%s [input] [output] [-f | --filter filter]\n"
"	[-b | --templates] [-B | --notemplates]\n"
"input (defaults to %s,\n"
"or to %s with -p | --private):\n"
"	[-n | --new] [-T | --timeout nsec] [-R | --recid n]\n"
"	OR\n"
"	[-l | --log srclogfile] [-t | --tail nrec] [-r | --reverse]\n"
"output (defaults to stdout):\n"
"	[-o | --out destlogfile]\n"
"	OR\n"
"	[-S | --formatstr format-string] [format_opts]\n"
"	OR\n"
"	[-F | --formatfile format-file] [format_opts]\n"
"	OR\n"
"	[-c | --compact] [-s | --separator sep] [-q | --nmeqval] [format_opts]\n"
"	OR\n"
"	[-m | --syslog]\n"
"format_opts:\n"
"	[-N | --newlines n] [-d | --datefmt date-format]\n",
		progName, progName, LOG_CURLOG_PATH, LOG_PRIVATE_PATH);
}

static void
usage()
{
	printUsage();
	exit(1);
}

static struct argInfo *
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
static void
processArgs(int argc, char **argv)
{
	struct argInfo *ai;

	for (++argv; *argv; ++argv) {
		char *arg = *argv;
		ai = findArg(arg);
		if (!ai) {
			fprintf(stderr, "%s: Unknown option: %s\n", progName,
				arg);
			usage();
		}

		if (ai->argSeen) {
			fprintf(stderr,
				"%s: Multiple %s options not supported\n",
				progName, arg);
			exit(1);
		}
		ai->argSeen = 1;

		if (strcmp(arg, "--help") != 0 && strcmp(arg, "-h") != 0) {
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
			*((int*)(ai->argVal)) = 1;
			break;
		case STRINGARG:
			*((char**)(ai->argVal)) = arg;
			break;
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
			fprintf(stderr,
				"%s: Internal error handling %s option\n",
				progName, arg);
			exit(2);
		}
	}
}

static void
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
 * Set the needTemplateForSpecialFormat flag.  This is needed for invocations
 * such as
 *	evlview -b -f filterString -S specialFormat
 * If specialFormat refs any non-standard attributes, or the data
 * pseudo-attribute, then we need to try to find a template for each record
 * we print; otherwise we don't bother.
 */
static void
doWeNeedTemplateForSpecialFormat()
{
	evl_listnode_t *head, *end, *p;

	if (formatWithoutTemplates) {
		return;
	}
	head = parsedSpecialFmt;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		evl_fmt_segment_t *fs = (evl_fmt_segment_t*) p->li_data;
		if (fs->fs_type == EVL_FS_ATTNAME
		    || (fs->fs_type == EVL_FS_MEMBER
		    && fs->u2.fs_member == POSIX_LOG_ENTRY_DATA)) {
			needTemplateForSpecialFormat = 1;
			break;
		}
	}
}

/*
 * Register to be notified about end-maintenance events.  Such an event
 * signals that the log is once again ready to be opened and read.
 *
 * If we succeed, set registeredForEndMaintEvents=1 and return 0.
 * If the notification daemon is unavailable, return -1.
 * Else print an error message and exit.
 */
static int
registerForEndMaintEvents()
{
	static struct sigevent endMaint;
	static struct sigaction endMaintAction;
	posix_log_query_t endMaintFilter;
	char endMaintFilterExpr[100];
	int status;
	int ret;

	snprintf(endMaintFilterExpr, sizeof(endMaintFilterExpr),
		"facility=logmgmt && event_type=%d",
		POSIX_LOG_MGMT_ENDMAINT);
	status = posix_log_query_create(endMaintFilterExpr,
		POSIX_LOG_PRPS_NOTIFY, &endMaintFilter, NULL, 0);
	if (status != 0) {
		fprintf(stderr, "%s: internal error: ", progName);
		errno = status;
		perror("posix_log_query_create for log-maintenance events");
		exit(2);
	}

	(void) memset(&endMaintAction, 0, sizeof(endMaintAction));
	endMaintAction.sa_sigaction = endMaintHandler;
	endMaintAction.sa_flags = SA_SIGINFO;
	if (sigaction(SIGRTMIN+2, &endMaintAction, NULL) < 0) {
		fprintf(stderr, "%s: internal error: ", progName);
		perror("sigaction for notification re: log-maintenance events");
		exit(2);
	}

	endMaint.sigev_notify = SIGEV_SIGNAL;
	endMaint.sigev_signo = SIGRTMIN+2;
	endMaint.sigev_value.sival_int = 0;	/* not used */
	status = posix_log_notify_add(&endMaintFilter, &endMaint,
		POSIX_LOG_SEND_SIGVAL, NULL);
	if (status == 0) {
		registeredForEndMaintEvents = 1;
		ret = 0;
	} else if (status == EAGAIN) {
		ret = -1;
	} else {
		fprintf(stderr, "%s: internal error: ", progName);
		errno = status;
		perror("posix_log_notify_add for log-maintenance events");
		exit(2);
	}

	(void) posix_log_query_destroy(&endMaintFilter);
	return ret;
}

/*
 * The command line has been parsed.  Verify that the selected combination
 * of options, and the values of those options, make sense.  We finish most
 * of our initialization work as a by-product.
 */
static void
checkSemantics()
{
	int status;

	/*
	 * If -help is specified, print the usage message.  It's an error
	 * if any other options are specified with -help.
	 */
	if (helpFlag) {
		printUsage();
		exit(somethingBesidesHelp ? 1 : 0);
	}

	if (private) {
		badOptionCombo("--log", "--private");
		logPath = LOG_PRIVATE_PATH;
	}

	/* We know of only two logs that could be active... */
	viewingActiveLog = 1;
	if (logPath
	    && strcmp(logPath, LOG_PRIVATE_PATH) != 0
	    && strcmp(logPath, LOG_CURLOG_PATH) != 0) {
		viewingActiveLog = 0;
	}

	if (newFlag) {
		badOptionCombo("--new", "--reverse");
		badOptionCombo("--new", "--tail");
		if (timeout < 1 && timeout != -1) {
			semanticError("--timeout value must be positive");
		}
		if (initialRecid < 0 && initialRecid != -1) {
			semanticError("--recid value cannot be negative");
		}
		if (!viewingActiveLog) {
			semanticError("Cannot view inactive log with --new");
		}
	} else {
		if (timeout != -1) {
			semanticError("--timeout option requires --new");
		}
		if (initialRecid != -1) {
			semanticError("--recid option requires --new");
		}
	}

	if (outPath) {
		format = FMT_BIN;
		badOptionCombo("--out", "--compact");
		badOptionCombo("--out", "--separator");
		badOptionCombo("--out", "--formatstr");
		badOptionCombo("--out", "--formatfile");
		badOptionCombo("--out", "--nmeqval");
		badOptionCombo("--out", "--newlines");
		badOptionCombo("--out", "--datefmt");
		badOptionCombo("--out", "--syslog");
	} else if (syslogFlag) {
		format = FMT_SYSLOG;
		badOptionCombo("--syslog", "--compact");
		badOptionCombo("--syslog", "--separator");
		badOptionCombo("--syslog", "--formatstr");
		badOptionCombo("--syslog", "--formatfile");
		badOptionCombo("--syslog", "--nmeqval");
		badOptionCombo("--syslog", "--newlines");
		badOptionCombo("--syslog", "--datefmt");
	} else {
		if (nmeqvalFlag) {
			format = FMT_NEQV;
			badOptionCombo("--nmeqval", "--formatstr");
			badOptionCombo("--nmeqval", "--formatfile");
			badOptionCombo("--nmeqval", "--notemplates");
		} else if (specFmtStr) {
			badOptionCombo("--formatstr", "--compact");
			badOptionCombo("--formatstr", "--separator");
			badOptionCombo("--formatstr", "--formatfile");

			format = FMT_SPEC;
			specialFormat = getFormatFromString(specFmtStr);
			/* Parse string into segments later. */
		} else if (specFmtPath) {
			badOptionCombo("--formatfile", "--compact");
			badOptionCombo("--formatfile", "--separator");

			format = FMT_SPEC;
			specialFormat = getFormatFromFile(specFmtPath);
			/* Parse string into segments later. */
		}
		if (newlines == 0 || newlines < -1) {
			semanticError("--newlines must specify a positive number of newlines");
		}
		if (dateFmtStr) {
			evl_time_format = dateFmtStr;
		}
	}

	badOptionCombo("--templates", "--notemplates");

	if (nTailRecs == 0 || nTailRecs < -1) {
		semanticError("--tail must specify a positive number of records");
	}

	if (formatWithoutTemplates) {
		evltemplate_initmgr(TMPL_IGNOREALL);
	} else {
		evltemplate_initmgr(TMPL_LAZYDEPOP|TMPL_REUSE1CLONE);
	}

	if (filterStr) {
		int status;
		int purpose = POSIX_LOG_PRPS_SEEK;
		char errbuf[200];

		if (acceptNonStdAtts) {
			purpose |= EVL_PRPS_TEMPLATE;
		}
		filter = &query;
		status = posix_log_query_create(filterStr, purpose, filter,
			errbuf, 200);
		if (status == EINVAL) {
			fprintf(stderr, "%s: --filter string: %s\n", progName,
				errbuf);
			exit(1);
		} else if (status != 0) {
			fprintf(stderr, "%s: internal error: ", progName);
			errno = status;
			perror("posix_log_query_create");
			exit(2);
		}
	}

	if (newFlag) {
		static struct sigevent nfy;
		static struct sigaction nfyAction;
		int status;

		if (filterStr) {
			int purpose = POSIX_LOG_PRPS_NOTIFY;
			nfyFilter = &nfyQuery;
			status = posix_log_query_create(filterStr, purpose,
				nfyFilter, NULL, 0);
			if (status != 0) {
				/*
				 * Query expression was valid for seeking,
				 * but not notification.  Just ask to be
				 * notified about every event, and let the
				 * seek filter sort it out.
				 */
				nfyFilter = NULL;
			}
		} else {
			nfyFilter = NULL;
		}

		/* Set up signal handler. */
		(void) memset(&nfyAction, 0, sizeof(nfyAction));
		nfyAction.sa_sigaction = nfyHandler;
		nfyAction.sa_flags = SA_SIGINFO;
		if (sigaction(SIGRTMIN+1, &nfyAction, NULL) < 0) {
			fprintf(stderr, "%s: internal error: ", progName);
			perror("sigaction for notification");
			exit(2);
		}

		/*
		 * Register to be notified about new events.  Note that it's OK
		 * if we start getting notifications right away.
		 */
		nfy.sigev_notify = SIGEV_SIGNAL;
		nfy.sigev_signo = SIGRTMIN+1;
		nfy.sigev_value.sival_int = 0;	/* not used */
		status = posix_log_notify_add(nfyFilter, &nfy,
			POSIX_LOG_SEND_SIGVAL, NULL);
		if (status != 0) {
			fprintf(stderr, "%s: internal error: ", progName);
			errno = status;
			perror("posix_log_notify_add");
			if (status == EAGAIN) {
				fprintf(stderr,
					"evlnotifyd may not be running.\n");
			}
			exit(2);
		}
	}

	if (specialFormat) {
		char *errMsg;
#ifdef TEST_EVREC_SPRINTF
		specialFormat2 = strdup(specialFormat);
#endif
		parsedSpecialFmt = _evlParseFormat(specialFormat,
			acceptNonStdAtts, &errMsg);
		if (!parsedSpecialFmt) {
			fprintf(stderr, "%s: %s\n", progName, errMsg);
			exit(1);
		}
		fdbuf = _evlAllocateFmtBuf(parsedSpecialFmt, &fdbufLen);
		doWeNeedTemplateForSpecialFormat();
	} else if (format != FMT_BIN) {
		fdbufLen = _evlGetMaxDumpLen();
		assert(fdbufLen > 1000);
		fdbuf = (char*) malloc(fdbufLen);
		assert(fdbuf != NULL);
	}

	status = posix_log_open(&logdesc, logPath);
	if (status == EBUSY) {
		if (viewingActiveLog) {
			awaitEndOfLogMaint();
		} else {
			fprintf(stderr,
"%s: Cannot open log file %s because it is inactive but marked busy\n",
				progName, logPath);
			/*
			 * TODO: Add a root-only option to patch the
			 * generation number in an inactive log so we can
			 * read it even under these circumstances.
			 */
			exit(1);
		}
	} else if (status != 0) {
		fprintf(stderr, "%s: Cannot open log file\n", progName);
		errno = status;
		perror(logPath ? logPath : LOG_CURLOG_PATH);
		exit(1);
	}

	if (outPath) {
		struct stat st;
		outFile = NULL;
		outFd = open(outPath, O_WRONLY|O_APPEND|O_CREAT, 0666);
		if (outFd < 0) {
			fprintf(stderr, "%s: Cannot append to file\n", progName);
			perror(outPath);
			exit(1);
		}

		/* If this is a new or empty log file, write the log header. */
		if (fstat(outFd, &st) < 0) {
			perror("fstat");
			exit(2);
		}
		if (st.st_size == 0) {
			if (_evlWriteLogHeader(outFd) < 0) {
				fprintf(stderr,
					"%s: Cannot write log header to file\n",
					progName);
				perror(outPath);
				exit(2);
			}
		}
	} else {
		outFd = 1;
		outFile = stdout;
	}

	if (separator) {
		/* Convert \n, \t, etc. */
		separator = parseSeparator(separator);
	} else {
		separator = defaultSeparator;
	}
}

/*
 * If there is a template for event record (entry, buf), get it and make
 * sure it is populated with that record.  Returns a pointer to the cloned,
 * populated template on success, or NULL on failure.
 */
static evltemplate_t *
getPopulatedTemplate(const struct posix_log_entry *entry, const char *buf)
{
	int status;
	evltemplate_t *tmpl;

	status = evl_gettemplate(entry, buf, &tmpl);
	if (status == ENOENT) {
		/* No such template. */
		return NULL;
	} else if (status != 0) {
		errno = status;
		perror("evl_gettemplate");
		exit(1);
	}

	/* tmpl is a clone of the template for this facility and event type.  */
	if (tmpl->tm_recid != entry->log_recid
	    || tmpl->tm_entry != entry
	    || tmpl->tm_data != buf) {
		/*
		 * It's not yet populated with the values from this
		 * record.
		 */
		status = evl_populatetemplate(tmpl, entry, buf);
		if (status != 0) {
			errno = status;
			perror("evl_populatetemplate");
			exit(1);
		}
	}
	return tmpl;
}

/*
 * Honor the --newlines option.  s is a string containing the last
 * (or only) part of the record to be printed.  Print it, ending with
 * the appropriate number of newlines.
 *
 * If <newlines> == -1 (--newlines not specified), just make sure there's
 * at least one newline at the end.
 *
 * Otherwise, make sure there are exactly <newlines> newlines, either by
 * printing extras or by temporarily truncating s.
 *
 * nPrecedingNewlines is the number of newlines terminating the previously
 * printed part of this record (i.e., 0 or 1).
 */
static void
printStringWithNewlines(char *s, int nPrecedingNewlines)
{
	if (newlines == -1) {
		if (s[0] == '\0') {
			if (nPrecedingNewlines == 0) {
				fprintf(outFile, "\n");
			}
		} else {
			fprintf(outFile, "%s", s);
			if (!_evlEndsWith(s, "\n")) {
				fprintf(outFile, "\n");
			}
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
			fprintf(outFile, "%s", s);
			while (totalNewlines < newlines) {
				fprintf(outFile, "\n");
				totalNewlines++;
			}
		} else if (totalNewlines > newlines) {
			int newlinesToShave = totalNewlines - newlines;
			char *p = s + (slen - newlinesToShave);
			char tmp = *p;
			*p = '\0';
			fprintf(outFile, "%s", s);
			*p = tmp;
		} else {
			fprintf(outFile, "%s", s);
		}
	}
}

static void
formatLikeSyslog(const struct posix_log_entry *entry, const char *buf)
{
	int status;
	size_t len;
	time_t timestamp = (time_t) entry->log_time.tv_sec;
	struct tm tm;

	len = strftime(fdbuf, fdbufLen, "%b %e %H:%M:%S ",
			localtime_r(&timestamp, &tm));
	assert(len != 0);
	(void) strcat(fdbuf, _evlGetHostNameEx( entry->log_processor >> 16));
	(void) strcat(fdbuf, " ");
	if ((entry->log_flags & EVL_KERNEL_EVENT) != 0) {
		(void) strcat(fdbuf, "kernel: ");
	}
	(void) fprintf(outFile, "%s", fdbuf);

	if ((entry->log_flags & EVL_PRINTK) != 0) {
		switch (entry->log_format) {
		case POSIX_LOG_NODATA:
			/* Shouldn't happen. */
			fprintf(outFile, "\n");
			break;
		case POSIX_LOG_BINARY:
			/*
			 * For a binary record, we assume that it starts
			 * with the null-terminated printk message.
			 */
			/*FALLTHROUGH*/
		case POSIX_LOG_STRING:
			printStringWithNewlines((char*)buf, 0);
			break;
		case POSIX_LOG_PRINTF:
			status = _evlFormatPrintfRec(buf, entry->log_size,
				fdbuf, fdbufLen, NULL, 1);
			assert(status == 0);
			printStringWithNewlines(fdbuf, 0);
			break;
		}
	} else {
		status = evl_format_evrec_variable(entry, buf,
			fdbuf, fdbufLen, NULL);
		assert(status == 0 || status == EMSGSIZE);
		printStringWithNewlines(fdbuf, 0);
	}
}

static void
copyRecord(const struct posix_log_entry *entry, const char *buf)
{
	int status;
	evltemplate_t *tmpl;

	switch (format) {
	case FMT_BIN:
		status = _evlFdWrite(outFd, entry, buf);
		if (status != 0) {
			perror(outPath);
			exit(2);
		}
		break;
	case FMT_TEXT:
	case FMT_NEQV:
	    {
		size_t linelen = 80;
		int flags = 0x0;
		if (compact) {
			linelen = 0;	/* All one line */
			flags = EVL_COMPACT;
		}
		status = evl_format_evrec_fixed(entry, fdbuf, fdbufLen,
			NULL, separator, linelen, flags);
		if (status != 0) {
			errno = status;
			perror("evl_format_evrec_fixed");
			exit(2);
		}
		(void) fprintf(outFile, "%s\n", fdbuf);

		/* Now the variable part... */
		if (format == FMT_NEQV
		    && (tmpl = getPopulatedTemplate(entry, buf)) != NULL) {
			status = evltemplate_neqvdump(tmpl, fdbuf, fdbufLen);
		} else {
			status = evl_format_evrec_variable(entry, buf, fdbuf,
				fdbufLen, NULL);
		}
		assert(status == 0 || status == EMSGSIZE);
		printStringWithNewlines(fdbuf, 1);
		break;
	    }
	case FMT_SPEC:
#ifdef TEST_EVREC_SPRINTF
	    {
		size_t reqlen;
		status = evl_format_evrec_sprintf(entry, buf, specialFormat2,
			fdbuf, fdbufLen, &reqlen);
	    	if (status != 0) {
			errno = status;
			perror("evl_format_evrec_sprintf");
			fprintf(stderr, "reqlen = %u\n", reqlen);
			exit(2);
		}
	    }
#else
		tmpl = NULL;
		if (needTemplateForSpecialFormat) {
			tmpl = getPopulatedTemplate(entry, buf);
		}
		_evlSpecialFormatEvrec(entry, buf, tmpl, parsedSpecialFmt,
			fdbuf, fdbufLen, NULL);
	    	if (tmpl) {
			evl_releasetemplate(tmpl);
		}
#endif
		printStringWithNewlines(fdbuf, 0);
		break;
	case FMT_SYSLOG:
		formatLikeSyslog(entry, buf);
		break;
	default:
		assert(0);
	}

	/*
	 * Given 'evlview -n > file', ensure that the records show up in
	 * file right away.
	 */
	if (newFlag && format != FMT_BIN) {
		(void) fflush(stdout);
	}

	lastRecordCopied = entry->log_recid;
}

/*
 * logdesc's read pointer points at the next record to read (or EOF).
 * Read the record and copy it to the output stream in the format previously
 * specified by the user.
 * If we hit end of file, return -1.
 * If the read fails because of a log-management event, we return -2.  In this
 *	case, we return with the read pointer set as indicated by resyncAction.
 * If the read fails for any other reason, print an error message and exit.
 * Return 0 on success.
 */
static int
readAndCopyRecord(int resyncAction)
{
	struct posix_log_entry entry;
	char buf[POSIX_LOG_ENTRY_MAXLEN];
	int status;
	
	status = posix_log_read(logdesc, &entry, buf, POSIX_LOG_ENTRY_MAXLEN);
	if (status != 0) {
		if (status == EAGAIN) {
			/* end of file */
			return -1;
		}	
		handleReadOrSeekFailure(status, "read", resyncAction);
		return -2;
	}
	copyRecord(&entry, buf);
	return 0;
}

/*
 * Seek to the end of the log; then read backward through the log, copying
 * all records that match the filter.  If nTailRecs > 0, copy at most that
 * many records.
 */
static void
copyBackward()
{
	int i, status;

startOver:
	seekToEnd();

	for (i = 1; nTailRecs == -1 || i <= nTailRecs; i++) {
		status = posix_log_seek(logdesc, filter, POSIX_LOG_SEEK_BACKWARD);
		if (status == ENOENT) {
			/* No matching record found. */
			return;
		} else if (status != 0) {
			handleReadOrSeekFailure(status, "seek backward",
				LMRESYNC_BEFORELAST);
			if (lastRecordCopied == -1) {
				goto startOver;
			}
			/* Redo this iteration. */
			i--;
			continue;
		}

		status = readAndCopyRecord(LMRESYNC_BEFORELAST);
		if (status == -1) {
			/* EOF */
			return;
		} else if (status == -2) {
			/* Interrupted by log maintenance */
			if (lastRecordCopied == -1) {
				goto startOver;
			}
			i--;
			continue;
		}

		status = posix_log_seek(logdesc, NULL, POSIX_LOG_SEEK_BACKWARD);
		if (status != 0) {
			handleReadOrSeekFailure(status,
				"seek backward past record just read",
				LMRESYNC_BEFORELAST);
			/*
			 * readAndCopyRecord() has already succeeded,
			 * so no need to redo this iteration.
			 */
		}
	}
}

/*
 * From the current position in the log, read through the log, copying
 * all records that match the filter.  If nTailRecs > 0, copy at most that
 * many records.
 *
 * Return -2 if we get interrupted by a log-maintenance operation and can't
 * get resync-ed.  (This happens only if we get interrupted before the process
 * copies its first record.)  Otherwise return 0.
 */
static int
copyForward()
{
	int i, status;
	for (i = 1; nTailRecs == -1 || i <= nTailRecs; i++) {
		if (filter) {
			status = posix_log_seek(logdesc, filter, POSIX_LOG_SEEK_FORWARD);
			if (status == ENOENT) {
				/* No matching record found. */
				return 0;
			} else if (status != 0) {
				handleReadOrSeekFailure(status, "seek forward",
					LMRESYNC_AFTERLAST);
				if (lastRecordCopied == -1) {
					return -2;
				}
				/* Redo this iteration. */
				i--;
				continue;
			}
		}

		status = readAndCopyRecord(LMRESYNC_AFTERLAST);
		if (status == -1) {
			/* EOF */
			return 0;
		} else if (status == -2) {
			if (lastRecordCopied == -1) {
				return -2;
			}
			i--;
			continue;
		}
	}
	return 0;
}

/*
 * We want to read the last nTailRecs records in the log that match the
 * filter -- but NOT in reverse order.  So seek to the end, then seek
 * backward past nTailRecs matching records.  Then copyForward() will
 * copy them.
 */
static void
prepareToTailForward()
{
	int i, status;

	assert(nTailRecs > 0);
	seekToEnd();

startOver:
	for (i = 1; i <= nTailRecs; i++) {
		status = posix_log_seek(logdesc, filter, POSIX_LOG_SEEK_BACKWARD);
		if (status == ENOENT) {
			/* No matching record found. */
			return;
		} else if (status != 0) {
			handleReadOrSeekFailure(status, "seek backward",
				LMRESYNC_END);
			goto startOver;
		}
	}
}

/*
 * User has specified --new --recid n.  Seek to the lowest-numbered record
 * whose ID is >= n, and copy all matching records from there to EOF.
 */
static void
copyOldTail()
{
	posix_log_query_t query;
	char qstring[40];
	int status;
	struct posix_log_entry entry;

	/*
	 * Assume that record n is close to the end.  For efficiency's sake,
	 * we seek to the last record whose ID is < n, then skip forward
	 * over that.
	 */
	snprintf(qstring, sizeof(qstring), "recid < %u", initialRecid);
	status = posix_log_query_create(qstring, POSIX_LOG_PRPS_SEEK,
		&query, NULL, 0);
	assert(status == 0);

startOver:
	status = posix_log_seek(logdesc, &query, POSIX_LOG_SEEK_LAST);
	if (status == 0) {
		/* Skip over record n-1 by reading it. */
		status = posix_log_read(logdesc, &entry, NULL, 0);
		if (status != 0) {
			handleReadOrSeekFailure(status, "skip to first record",
				LMRESYNC_START);
			goto startOver;
		}
	} else if (status != ENOENT) {
		handleReadOrSeekFailure(status, "seek to first record",
			LMRESYNC_START);
		goto startOver;
	}
	/*
	 * status == ENOENT means there's no record newer than n.
	 * If so, we want to start at BOF, and we're there already.
	 */

	if (copyForward() == -2) {
		goto startOver;
	}
	(void) posix_log_query_destroy(&query);
}

/*
 * This function implements the --new option.  Run copyForward() whenever
 * our notification handler indicates that a new record of interest has
 * been logged.
 */
static void
copyNewRecords()
{
	/*
	 * We sleep at most 5 seconds at a time, so we can detect within
	 * that time an event that arrived just before we went to sleep.
	 */
	int maxSleep = 5;
	int status;

startOver:
	if (initialRecid != -1) {
		/*
		 * Note that if we come through here a second time, it
		 * means that there were no records of interest between
		 * initialRecid and the original EOF.  However, there
		 * may be some between the old EOF and the new one, and
		 * this way we're sure to catch them.
		 */
		copyOldTail();
	} else {
		seekToEnd();
	}

	if (timeout != -1) {
		int slept, totalSlept;
		for (;;) {
			/* Copy new events until they stop arriving. */
			while (writeCount != nfyCount) {
				writeCount = nfyCount;
				if (copyForward() == -2) {
					goto startOver;
				}
			}

			/*
			 * Sleep until a new event arrives or until the
			 * timeout interval passes with no new events.
			 * Note that arrival of the notification signal will
			 * terminate sleep().
			 */
			for (totalSlept = 0;
			    totalSlept < timeout;
			    totalSlept += slept) {
				int sleepThisMuch = timeout - totalSlept;
				if (sleepThisMuch > maxSleep) {
					sleepThisMuch = maxSleep;
				}
				slept = sleepThisMuch - sleep(sleepThisMuch);
				if (writeCount != nfyCount) {
					/* A new event has arrived. */
					break;
				}
			}
			if (writeCount == nfyCount) {
				/* Timed out, no event. */
				exit(0);
			}
			/* A new event has arrived.  Go back to the top. */
		}
	} else {
		/* No timeout.  Read and write records until they kill us. */
		for (;;) {
			while (writeCount != nfyCount) {
				writeCount = nfyCount;
				if (copyForward() == -2) {
					goto startOver;
				}
			}
			while (writeCount == nfyCount) {
				sleep(maxSleep);
			}
		}
	}
}

static void
seekToEnd()
{
	int status = posix_log_seek(logdesc, NULL, POSIX_LOG_SEEK_END);
	if (status != 0) {
		handleReadOrSeekFailure(status, "seek to end", LMRESYNC_END);
	}
}

/*
 * What's a reasonable amount of time to wait for completion of the
 * log-maintenance operation that interrupted us?
 * 1. If a timeout was specified with --timeout, use that.
 * 2. Figure 0.5 seconds per MByte of log file is very generous.
 * 3. No matter what number we get from #2, never give up in less than 10
 * seconds or wait more than 5 minutes.
 */
static int
computeLogMgmtTimeout()
{
	int lmTimeout;
	const char *path = logPath;
	struct stat st;
	double secondsPerMbyte = 0.5;
	int minTimeout = 10, maxTimeout = 5*60;

	if (timeout > 0) {
		return timeout;
	}

	if (!path) {
		path = LOG_CURLOG_PATH;
	}
	if (stat(path, &st) != 0) {
		fprintf(stderr, "%s: stat of log file failed.\n", progName);
		perror(path);
		exit(2);
	}
	lmTimeout = secondsPerMbyte * (st.st_size / (1000*1000));
	if (lmTimeout < minTimeout) {
		lmTimeout = minTimeout;
	} else if (lmTimeout > maxTimeout) {
		lmTimeout = maxTimeout;
	}
	return lmTimeout;
}

/*
 * Access to the log file has failed in a way that indicates that log
 * maintenance is underway.  The log has been closed (or never successfully
 * opened).  We try to reopen it immediately, again upon the arrival of a
 * POSIX_LOG_MGMT_ENDMAINT event, and again upon termination of a timeout.
 * We return on success.  If the timeout expires and our last try fails,
 * we print an error message and exit.
 */
static void
awaitEndOfLogMaint()
{
	int lmTimeout;
	time_t now, deadline;
	int generation;
	int status;

	LMDEBUG("Awaiting end of log maintenance operation...\n");

	lmTimeout = computeLogMgmtTimeout();
	(void) time(&now);
	deadline = now + lmTimeout;

	if (!registeredForEndMaintEvents) {
		if (registerForEndMaintEvents() == -1) {
			/* Notification daemon unavailable. */
			fprintf(stderr,
"%s: Interrupted by evlogmgr, and evlnotifyd not running.  Cannot recover.\n",
				progName);
			exit(2);
		}
	}

	for (;;) {
		generation = logGeneration;
		status = posix_log_open(&logdesc, logPath);
		if (status == 0) {
			LMDEBUG("Successfully reopened log.\n");
			return;
		}
		if (status != EBUSY) {
			fprintf(stderr,
"%s: Log file was busy, but now cannot be opened at all!\n",
				progName);
			exit(2);
		}

		if (now >= deadline) {
			/* Timed out, no end-maintenance event. */
			fprintf(stderr,
"%s: Log file is inaccessible, apparently due to ongoing maintenance.\n"
"Timed out after %d seconds.\n",
				progName, lmTimeout);
			exit(2);
		}

		/*
		 * Sleep until an end-maint event arrives (and increments
		 * log_generation) or until the timeout interval passes.
		 * Note that arrival of any notification signal will
		 * terminate sleep().
		 */
		while (now < deadline && generation == logGeneration) {
			sleep(deadline - now);
			(void) time(&now);
		}

		/*
		 * Even if we've timed out, go back and try one more time
		 * to open the log.
		 */
	}
}

/*
 * This object encodes what kind of seek we have to do after successfully
 * reopening the log after a log-maintenance operation.
 */
struct resyncStrategy {
	posix_log_query_t	rsQuery;	/* seek query */
	int	rsQueryNeeded;	/* Is rsQuery meaningful? */
	int	rsDirection;	/* seek direction */
	int	rsNoentDxn;	/* What to do if 1st seek fails with ENOENT */
	int	rsJustSeek;	/* 1 if all we do is seek once */
};

/*
 * Populate rs with the resync strategy needed to accomplish the indicated
 * resync action.
 */
static void
figureResyncStrategy(int resyncAction, struct resyncStrategy *rs)
{
	const char *op;
	char queryString[100];
	int status;

	assert(resyncAction == LMRESYNC_BEFORELAST
		|| resyncAction == LMRESYNC_AFTERLAST);

	if (newFlag || nTailRecs != -1) {
		/*
		 * It's probably more efficient to seek from EOF.
		 * Seek backward to the last record whose recid is <=
		 * lastRecordCopied.  (After that the caller will have to
		 * do some more poking around to make sure we're in the
		 * right place.)  If that seek finds no record (i.e.,
		 * lastRecordCopied and all earlier records are gone),
		 * just seek to BOF.
		 */
		op = "<=";
		rs->rsDirection = POSIX_LOG_SEEK_LAST;
		rs->rsNoentDxn = POSIX_LOG_SEEK_START;
		rs->rsJustSeek = 0;
	} else {
		/*
		 * Seek forward to the first record whose recid is > (or >=)
		 * lastRecordCopied.  If that seek finds no record (i.e.,
		 * all later records are gone), just seek to EOF.
		 */
		rs->rsDirection = POSIX_LOG_SEEK_FIRST;
		rs->rsNoentDxn = POSIX_LOG_SEEK_END;
		rs->rsJustSeek = 1;
		if (resyncAction == LMRESYNC_BEFORELAST) {
			op = ">=";
		} else {
			op = ">";
		}
	}
	rs->rsQueryNeeded = 1;
	snprintf(queryString, sizeof(queryString), "recid %s %d", op, lastRecordCopied);
	status = posix_log_query_create(queryString, POSIX_LOG_PRPS_SEEK,
		&rs->rsQuery, NULL, 0);
	if (status != 0) {
		fprintf(stderr, "%s: internal error: bad query in figureResyncStrategy()\n",
			progName);
		errno = status;
		perror("posix_log_query_create");
		exit(2);
	}
}

/*
 * A posix_log_read or posix_log_seek operation has failed.  If the log
 * is active, and the failure is EBADF (indicating that a log
 * maintenance operation has started), we close the log, wait until the log
 * maintenance operation is done, open the log, and put the read pointer
 * back where it needs to be (as indicated by resyncAction).
 *
 * Otherwise, we report the error and exit.
 */
static void
handleReadOrSeekFailure(int failure, const char *operation, int resyncAction)
{
	struct resyncStrategy rs;
	int status;
	struct posix_log_entry entry;

	if (!viewingActiveLog || failure != EBADF) {
		reportReadOrSeekFailure(failure, operation);
		exit(2);
	}

	LMDEBUG("Interrupted by log maintenance.  Last record copied = %d\n",
		lastRecordCopied);

	/*
	 * If we're going to have to resync on the last record copied,
	 * set up the necessary query.  Note that this has to work even if
	 * lastRecordCopied is no longer in the log.
	 *
	 * If no record has been copied yet, we'll just leave the read
	 * pointer at BOF.
	 */
	if (lastRecordCopied != -1
	    && (resyncAction == LMRESYNC_BEFORELAST
	    || resyncAction == LMRESYNC_AFTERLAST)) {
		figureResyncStrategy(resyncAction, &rs);
	} else {
		rs.rsQueryNeeded = 0;
		rs.rsJustSeek = 1;
	}

tryAgain:
	(void) posix_log_close(logdesc);
	awaitEndOfLogMaint();

	/*
	 * We have successfully reopened the log.  Now put the read pointer
	 * back where the caller wants it.
	 */
	if (resyncAction == LMRESYNC_START || lastRecordCopied == -1) {
		goto finish;
	}
	
	switch (resyncAction) {
	case LMRESYNC_END:
		status = posix_log_seek(logdesc, NULL, POSIX_LOG_SEEK_END);
		break;
	case LMRESYNC_BEFORELAST:
	case LMRESYNC_AFTERLAST:
		status = posix_log_seek(logdesc, &rs.rsQuery, rs.rsDirection);
		break;
	}
	if (status == EBADF) {
		/* Log maintenance again! */
		goto tryAgain;
	} else if (status == ENOENT) {
		/* No match.  Just go to BOF or EOF, as appropriate. */
		status = posix_log_seek(logdesc, NULL, rs.rsNoentDxn);
		if (status == EBADF) {
			goto tryAgain;
		} else if (status != 0) {
			reportReadOrSeekFailure(status, "resync #2 after log maintenance");
		}
		goto finish;
	} else if (status != 0) {
		reportReadOrSeekFailure(status, "resync after log maintenance");
	}

	/* We successfully completed the seek prescribed in our strategy. */
	if (rs.rsJustSeek) {
		goto finish;
	}

	/* We must have sought backward from EOF.  Still not done. */
	status = posix_log_read(logdesc, &entry, NULL, 0);
	if (status == EBADF) {
		goto tryAgain;
	} else if (status != 0) {
		reportReadOrSeekFailure(status, "resync #3 after log maintenance");
	}
	if (rs.rsDirection == POSIX_LOG_SEEK_LAST
	    && resyncAction == LMRESYNC_AFTERLAST) {
		/*
		 * We sought backward to the last record <= lastRecordCopied.
		 * Then we advanced the read pointer past that record.
		 * No matter what that record is, we're done.
		 */
		goto finish;
	}
	assert(rs.rsDirection == POSIX_LOG_SEEK_LAST
	    && resyncAction == LMRESYNC_BEFORELAST);
	if (entry.log_recid != lastRecordCopied) {
		/*
		 * lastRecordCopied has been deleted.  The read pointer is
		 * now where lastRecordCopied used to be.
		 */
		goto finish;
	}

	/*
	 * As it turns out, we sought backward to the beginning of
	 * lastRecordCopied, then read past it.  Back up one.
	 */
	status = posix_log_seek(logdesc, NULL, POSIX_LOG_SEEK_BACKWARD);
	if (status == EBADF) {
		goto tryAgain;
	} else if (status != 0) {
		reportReadOrSeekFailure(status, "resync #4 after log maintenance");
	}

finish:
	LMDEBUG("Recovered from log-maintenance interruption.\n");
	if (rs.rsQueryNeeded) {
		(void) posix_log_query_destroy(&rs.rsQuery);
	}
}

main(int argc, char **argv)
{
	int status;

	progName = argv[0];
	(void) setlocale(LC_ALL, "");
	processArgs(argc, argv);
	checkSemantics();

	if (newFlag) {
		copyNewRecords();
	} else if (reverse) {
		copyBackward();
	} else if (nTailRecs > 0) {
		do {
			prepareToTailForward();
			status = copyForward();
		} while (status == -2);
	} else {
		do {
			status = copyForward();
		} while (status == -2);
	}
	exit(0);
}
