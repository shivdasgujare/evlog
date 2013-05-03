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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "config.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"

#if !defined(__GLIBC__)
# define __NR_ksyslog __NR_syslog
_syscall3(int,ksyslog,int, type, char *, buf, int, len);
#else
#include <sys/klog.h>
#define ksyslog klogctl
#endif

const char *progname;

typedef struct options {
        int  conflist;
        char *scr_filter;
        int  dcount;			/* 0 = infinite count */
        int  dinterval;			/* 0 = infinite interval */
		int	 dnumdups;			/* 0 = default value */
        char *dis_on;
        int sev_level;			/* -1 = disable console message */
} options_t;

static options_t cmd_opt = {0, NULL, -1, -1, -1, NULL, 0xbad};

#define ACCESS_OK(pid) (getuid() == pid)

#define MAX_EVL_LINE 		1024
#define DIS_COUNT 		0
#define DIS_INTERVAL 		1
#define SEV_LEVEL		2
#define LOOKBACK_SIZE		3
#define DIS_DUPS		4
#define EVENT_SCREEN		5

#define MAX_DUP_INTERVAL        3600
#define MAX_DUP_COUNT           10000
#define MAX_DUP_ARRAY_SIZE	1000
#define CONSOLE_OUTPUT_DISABLE	111

char *confPath = LOG_EVLOG_CONF_DIR "/evlog.conf";

static void
usage()
{
	fprintf(stderr,
		"Usage: \tevlconfig -l | --list\n"
		"\tevlconfig -d | --discarddups on | off\n"
		"\tevlconfig -i | --interval seconds\n"
		"\tevlconfig -c | --count events\n"
		"\tevlconfig -L | --lookbacks size\n"
		"\tevlconfig -s | --screen filter | nofilter\n"
		"\tevlconfig -o | --output severity-level | off\n"
		"\tevlconfig --help\n");
	exit(1);
}

static long
valid_integer(const char *s)
{
	char *end = '\0';
	long value = strtol(s, &end, 0);

	if (end == s) {
		fprintf(stderr, "evlconfig: Invalid value\n");
		return -1;
	}
	return value;
}

/*
 * Parse the command line options.
 */
static void
get_cmd_opts(int argc, char **argv, options_t *optp)
{
	int i = 1;
	int error = 0;

	if (argc == 1) usage();

	while (i < argc){
		if (!strcmp(argv[i], "--help")) {
			usage();
		} else if (!strcmp(argv[i], "--list") || 
			!strcmp(argv[i], "-l")) {
			optp->conflist = 1;
		} else if (!strcmp(argv[i], "--lookbacks") ||
			!strcmp(argv[i], "-L")) {
			if ((argc - i) == 1) {
				usage();
			}
			optp->dnumdups = valid_integer(argv[++i]);
			if (optp->dnumdups == -1) {
				usage();
			}
			if (optp->dnumdups < 0 ||
				optp->dnumdups > MAX_DUP_ARRAY_SIZE) {
				fprintf(stderr,
						"%s: Invalid duplicate lookback size. Allowable range is from 1 to %d.\n",
						progname, MAX_DUP_ARRAY_SIZE);
				exit(1);
			}
		} else if (!strcmp(argv[i], "--interval") || 
			!strcmp(argv[i], "-i")) {
			if ((argc - i) == 1) {
				usage();
			}
			optp->dinterval = valid_integer(argv[++i]);
			if (optp->dinterval == -1) {
				usage();
			}
			if (optp->dinterval < 0 || 
				optp->dinterval > MAX_DUP_INTERVAL) {
				fprintf(stderr, 
					"%s: Invalid Discard Interval. Allowable range is from 1 to 3600 (in seconds).\n",
					progname);
				exit(1);
			}
		} else if (!strcmp(argv[i], "--count") || 
			!strcmp(argv[i], "-c")) {
			if ((argc - i) == 1) {
				usage();
			}
			optp->dcount = valid_integer(argv[++i]);
			if (optp->dcount == -1) {
				usage();
			}
			if (optp->dcount < 0 || optp->dcount > MAX_DUP_COUNT) {
				fprintf(stderr,"%s: Invalid Discard Count. Allowable range is from 1 to 10000.\n",
					progname);
				exit(1);
			}
		} else if (!strcmp(argv[i], "--screen") || 
			!strcmp(argv[i], "-s")) {
			if ((argc - i) == 1) {
				usage();
			}
			optp->scr_filter = argv[++i];
		} else if (!strcmp(argv[i], "--discarddups") || 
				!strcmp(argv[i], "-d")) {
			if ((argc - i) == 1) {
				usage();
			}
			if (strcmp(argv[++i], "on") && strcmp(argv[i], "off")) {
				fprintf(stderr, "%s: Invalid value with '--discarddups' option\n", 
					progname);
				usage();
			}
			optp->dis_on = argv[i];
		} else if (!strcmp(argv[i], "--output") ||
				!strcmp(argv[i], "-o")) {
			char *sev_level_str;
			if ((argc - i) == 1) {
				usage();
			}
			sev_level_str = argv[++i];
			 
			if (!strncasecmp(sev_level_str, "off", 3)) {
				optp->sev_level = CONSOLE_OUTPUT_DISABLE;
			} else if (!strncasecmp(sev_level_str, "EMERG", 5)) {
				optp->sev_level = LOG_EMERG;
			} else if (!strncasecmp(sev_level_str, "ALERT", 5)) {
				optp->sev_level = LOG_ALERT;
			} else if (!strncasecmp(sev_level_str, "CRIT", 4)) {
				optp->sev_level = LOG_CRIT;
			} else if (!strncasecmp(sev_level_str, "ERR", 3)) {
				optp->sev_level = LOG_ERR;
			} else if (!strncasecmp(sev_level_str, "WARNING", 7)) {
				optp->sev_level = LOG_WARNING;
			} else if (!strncasecmp(sev_level_str, "NOTICE", 6)) {
				optp->sev_level = LOG_NOTICE;
			} else if (!strncasecmp(sev_level_str, "INFO", 4)) {
				optp->sev_level = LOG_INFO;
			} else if (!strncasecmp(sev_level_str, "DEBUG", 5)) {
				optp->sev_level = LOG_DEBUG;
			} else {
				usage();
			} 
			
		}			  
		else {
			usage();
		}
		i++;
	}

	if ((optp->dis_on != NULL) && !strcmp(optp->dis_on,"off") &&
			((optp->dcount > 0) || (optp->dinterval > 0))) {
		fprintf(stderr,
			"%s: Cannot specify '-discard_dups' as 'off' with options '--count' or '--interval'\n", progname);
		usage();
	}
}

/*
 * Update the file with the requested value. Find the pointer where need to be
 * updated, copy the rest of the file to the buffer and copy back after the
 * requested value is updated. Also, truncate the file with the new file size.
 *
 * FUNCTION     : updateFile
 *
 * PURPOSE      : Update the evlog.conf file with the requested config value.
 *
 * RETURN       : returns 0 for success, otherwise -1
 *
 */
static int
updateFile(int fd, FILE *fp, char *buf, int buflen, char *symbol)
{
	char line[MAX_EVL_LINE];
	char *mov_buf, *s, *c, *p;
	size_t act_pos, cur_off, move_size;
	struct stat st;

	while (fgets(line, MAX_EVL_LINE, fp)) {
		if (line[0] == '#') {
			continue;
		}
		p = line + strspn(line, " \t");
		if (p[0] == '\n') {
			/* Empty line */
			continue;
		}
		if ((s = strchr(p, ':')) == NULL) {
			fprintf(stderr, 
				"%s: '%s' file got corrupted\n", progname, confPath);
			return -1;
		}
		*s++ = '\0';
		if (!strncmp(p, symbol, strlen(symbol))) {
			c = s + strspn(s, " \t");
			act_pos = strlen(c);
			cur_off = ftell(fp);
			(void)fstat(fd, &st);
			move_size = st.st_size - cur_off;

			if ((mov_buf = (char *)malloc(move_size + 1)) == NULL) {
				fprintf(stderr,
					"%s: Out of memory to resize evlog.conf file\n",
					progname);
				return -1;
			}
			(void)fread(mov_buf + 1, move_size, 1, fp);
			(void)fseek(fp, -(move_size + act_pos), SEEK_CUR);
			(void)fwrite(buf, buflen, 1, fp);
			mov_buf[0] = '\n';
			(void)fwrite(mov_buf, move_size + 1, 1, fp);
			(void)ftruncate(fd, ftell(fp));
			fseek(fp, 0, SEEK_SET);
			free(mov_buf);
			return 0;
		}
	}

	fprintf(stderr, "%s: '%s' file got corrupted\n", progname, confPath);
	return -1;
}

/*
 * FUNCTION     : updateConfValues
 *
 * PURPOSE      : Update evlog.conf file with the requested value and 
 *		  also send to evlogd daemon.
 *
 * RETURN       : returns 0 for success. If write to socket fails, returns 1. 
 *		  otherwise -1 for other failures
 */
static int
updateConfValues(int fd, FILE *fp, int sd, int type, int value, 
                 char *wbuf, int len, char *symbol)
{
	/*
	 * If user changes the console display level we could do it right here
	 * we don't need to connect to the daemon to do it
	 */
	if (type == SEV_LEVEL) {
		if (value == CONSOLE_OUTPUT_DISABLE ) {
			if (ksyslog(6, NULL, 0) != 0) {
				fprintf(stderr, 
					"evlogd: Fail to disable console display\n");
				return 1;
			}	
		} else {
			if (ksyslog(8, NULL, value) != 0) {
				fprintf(stderr, 
					"evlogd: Fail to set console display level\n");
				return 1;
			}	
		}
	} else {
		/*
		 * Send data to evlogd daemon.
		 */
		if (write(sd, &type, sizeof(int)) <= 0) {
			fprintf(stderr, "%s: Failed to write on socket\n", progname);
			return 1;
		}
		if (type >= DIS_DUPS) {
			/*
			 * Send string values
			 */
			if (write(sd, &len, sizeof(int)) <= 0) {
				fprintf(stderr, "%s: Failed to write on socket\n",
						progname);
				return 1;
			}
			if (write(sd, wbuf, len) <= 0) {
				fprintf(stderr, "%s: Failed to write on socket\n",
						progname);
				perror("write");
				return 1;
			}
		} else {
			if (write(sd, &value, sizeof(int)) <= 0) {
				fprintf(stderr, "%s: Failed to write on socket\n",
						progname);
				return 1;
			}
		}
	} 
		
	/* Updated evlog.conf file */
	if (updateFile(fd, fp, wbuf, len, symbol) < 0) {
		return -1;
	}

	return 0;
}

/*
 * FUNCTION     : evlconf_ops
 *
 * PURPOSE      : Connect to the evlogd daemon. Also Update evlog.conf file 
 *		  with the requested value and send these values to the daemon.
 *
 * RETURN       : returns 0 for success. otherwise returns -1
 */
static int
evlconf_ops(int fd, int sd)
{
	FILE *fp;
	char wrbuf[255];
	int retry = 0, sock_len;
	int error = 0;
	struct sockaddr_un sa;
	char *evlconf_str = "evlogd is reading new configure parameter";
	int end_tran = EVENT_SCREEN + 1;
	struct posix_log_entry entry;
	int gotTimeStamp = 0;
	if ((sd = _establishNonBlkConnection(EVLOG_CONF_SOCKET, &sa, 1 /*timeout*/)) < 0) {
			/* Can't connect to log daemon - exit */
			exit(1);
	}
	if ((fp = fdopen(fd, "r+")) == NULL) {
		perror("fdopen");
		error = -1;
		goto out;
	}

	if (cmd_opt.dcount >= 0) {
		snprintf(wrbuf, sizeof(wrbuf), "%d", cmd_opt.dcount);
		if ((error = updateConfValues(fd, fp, sd, DIS_COUNT, 
			cmd_opt.dcount, wrbuf, strlen(wrbuf), 
			"Discard Count")) != 0) {
			goto out;
		}
	}
	if (cmd_opt.dinterval >= 0) {
		snprintf(wrbuf, sizeof(wrbuf), "%d", cmd_opt.dinterval);
		if ((error = updateConfValues(fd, fp, sd, DIS_INTERVAL, 
			cmd_opt.dinterval, wrbuf, strlen(wrbuf),
			"Discard Interval")) != 0) {
			goto out;
		}
	}
	if (cmd_opt.dnumdups >= 0) {
		snprintf(wrbuf, sizeof(wrbuf), "%d", cmd_opt.dnumdups);
		if ((error = updateConfValues(fd, fp, sd, LOOKBACK_SIZE,
			cmd_opt.dnumdups, wrbuf, strlen(wrbuf),
			"Lookback Size")) != 0) {
			goto out;
		}
	}
	if (cmd_opt.dis_on != (char *)NULL) {
		if ((error = updateConfValues(fd, fp, sd, DIS_DUPS, 0, 
			cmd_opt.dis_on, strlen(cmd_opt.dis_on),
			"Discard Dups")) != 0) {
			goto out;
		}
	}
	if (cmd_opt.scr_filter != (char *)NULL) {
		posix_log_query_t query;
		char errbuf[80];
		/*
		 * Test whether the user requested filter is valid.
		 */
		if(strcmp(cmd_opt.scr_filter, "nofilter")) {
			if (posix_log_query_create(cmd_opt.scr_filter,POSIX_LOG_PRPS_NOTIFY, 
					&query, errbuf, 80) != 0) {
				fprintf(stderr, "%s: Invalid event screen: %s\n", 
						progname, errbuf);
				error = -1;
				goto out;
			}
		}
		if ((error = updateConfValues(fd, fp, sd, EVENT_SCREEN, 0,
				cmd_opt.scr_filter, strlen(cmd_opt.scr_filter),
				"Event Screen")) != 0) {
				goto out;
		}
			
	}
	if (cmd_opt.sev_level != 0xbad) {
		snprintf(wrbuf, sizeof(wrbuf), "%d", cmd_opt.sev_level);
		if ((error = updateConfValues(fd, fp, sd, SEV_LEVEL, 
			cmd_opt.sev_level, wrbuf, strlen(wrbuf),
			"Console display level")) != 0) {
			goto out;
		}
	}
	
out:
	/*
	 * Send the end of transmission value to evlogd. This value allows to
	 * send mutiple options in one socket connection. The error value will 
	 * be 1 only if the previous write call failed.
	 */
	if ((error != 1) && write(sd, &end_tran, sizeof(int)) < 0) {
		error = -1;
	}
	fclose(fp);	/* closes fd */
	return error;
}


main(int argc, char **argv)
{
	int i, status, fd, sd, estatus = 0;
	progname = argv[0];

	get_cmd_opts(argc, argv, &cmd_opt);

	if ((cmd_opt.dcount >= 0) || (cmd_opt.dinterval >= 0) ||
		(cmd_opt.dnumdups >= 0) ||
		(cmd_opt.dis_on != (char *)NULL) ||
		(cmd_opt.scr_filter != (char *)NULL) ||
		(cmd_opt.sev_level != 0xbad)) {
		/*
		 * Only root has permission to modify evlog.conf file.
		 */
		if (!ACCESS_OK(0)) {
			fprintf(stderr,
				"%s: Only root has permission to update %s\n",
				progname, confPath);
			exit(1);
		}
		if ((fd = open(confPath, O_RDWR, 0666)) < 0) {
			fprintf(stderr,
				"%s: Cannot open '%s'\n", progname, confPath);
			exit(1);
		}
		if ((sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
			fprintf(stderr, 
				"evlconfig: cannot create socket for communication with 'evlogd' daemon.\n");
			perror("socket");
			return -1;
		}
		if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
			perror("fcntl");
			close(sd);
			return -1;
		}
		(void) fcntl(fd, F_WRLCK);
		estatus = evlconf_ops(fd, sd);
		(void) fcntl(fd, F_UNLCK);
		close(fd);
		close(sd);
		if (estatus != 0) {
			exit(1);
		}
	}
	if (cmd_opt.conflist) {
		char line[MAX_EVL_LINE];
		FILE *conf_f;
		int value, start_qexp = 0;

		if ((fd = open(confPath, O_RDONLY)) < 0) {
			fprintf(stderr,
				"%s: Cannot open evlog.conf file for reading\n",
				progname);
			exit(1);
		}
		if ((conf_f = fdopen(fd, "r")) == NULL) {
			fprintf(stderr, "%s: fdopen failed: %s\n",
				progname, strerror(errno));
			close(fd);
			exit(1);
		}
		(void) fcntl(fd, F_RDLCK);
		while (fgets(line, MAX_EVL_LINE, conf_f)) {
			char *s, *p, *c;
			p = line + strspn(line, " \t");
			if ((p[0] == '\n') || (p[0] == '#')) {
				/* Empty line */
				continue;
			}
			if ((s = strchr(p, ':')) == NULL) {
				fprintf(stderr,
					"%s: '%s' file got corrupted\n", 
					progname, confPath);
				exit(1);
			}
			*s++ = '\0';
			if (!strncmp(p, "Discard Dups", 12)) {
				fprintf(stdout, "Discard Dups = ");
				c = s + strspn(s, " \t");
				if (!strncmp(c, "on", 2)) {
					fprintf(stdout, "on\n");
				} else if (!strncmp(c, "off", 3)) {
					fprintf(stdout, "off\n");
				} else {
					fprintf(stderr,
					  "Invalid string for 'Discard Dups\n");
					exit(1);
				}
				continue;
			} else if (!strncmp(p, "Discard Interval", 16)) {
				if ((value = valid_integer(s)) < 0) {
					exit(1);
				}
				fprintf(stdout,
					"Discard Interval = %d seconds\n", 
					value);
				continue;
			} else if (!strncmp(p, "Discard Count", 13)) {
				if ((value = valid_integer(s)) < 0) {
					exit(1);
				}
				fprintf(stdout,
					"Discard Count = %d identical events\n",
					value);
				continue;
			} else if (!strncmp(p, "Lookback Size", 13)) {
				if ((value = valid_integer(s)) < 0) {
					exit(1);
				}
				fprintf(stdout,
					"Lookback Size = %d lookback events\n",
					value);
				continue;
			} else if (!strncmp(p, "Event Screen", 12)) {
				fprintf(stdout, "Event Screen: \n");
				start_qexp = 1;
				p = s + strspn(s, " \t");
				if (*p == '\n') {
					fprintf(stderr,
						"%s: '%s' file got corrupted\n",
						progname, confPath);
					exit(1);
				}
				fprintf(stdout, "\t%s", p);
				continue;
			} if (!strncmp(p, "Console display level", 21)) {
				fprintf(stdout, "Console display level = ");

				if ((value = valid_integer(s)) < 0) {
					exit(1);
				}
				switch(value) {
					case CONSOLE_OUTPUT_DISABLE:
						fprintf(stdout, "off\n");
						break;
					case LOG_EMERG:
						fprintf(stdout, "EMERG\n");
						break;
					case LOG_ALERT:
						fprintf(stdout, "ALERT\n");
						break;
					case LOG_CRIT:
						fprintf(stdout, "CRIT\n");
						break;
					case LOG_ERR:
						fprintf(stdout, "ERR\n");
						break;
					case LOG_WARNING:
						fprintf(stdout, "WARNING\n");
						break;
					case LOG_NOTICE:
						fprintf(stdout, "NOTICE\n");
						break;
					case LOG_INFO:
						fprintf(stdout, "INFO\n");
						break;
					case LOG_DEBUG:
						fprintf(stdout, "DEBUG\n");
						break;
					default:
						break;
				}
			 
			} else {
				fprintf(stderr, "%s: '%s' file got corrupted\n",
					progname, confPath);
				exit(1);
			} 
		}
		fclose(conf_f);	/* closes fd; frees locks */
	}

	exit(0);
}

