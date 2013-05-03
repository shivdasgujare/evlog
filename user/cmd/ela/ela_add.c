/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2002
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
 *  Please send e-mail to nguyhien@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */
#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE	1024 

static const char *progName;
static char *ela_bin = "\'/usr/share/evlog/ela %recid%\'";


/*
 * Execute a program and capture the output (either stdout, stderr)
 */
int run_cmd(const char *prog, const char *params, char *sout, size_t obufsz)
{
	pid_t child_pid;
	int pfd[2];
	int i, status, n;
	char command[512];

	sprintf(command, "%s %s", prog, params);
	/* Create a pipe */
	if (pipe(pfd) < 0) {
		return -1;
	}

	/* Create a child process */
	if ((child_pid = fork()) < 0) {
		perror("fork");
		return -1;
	}

	/* The child process execute the cmd */
	if (child_pid == 0) {
		/* Attach standard output to the pipe */
		dup2(pfd[1], 1);
		close(pfd[0]);

		execlp("/bin/sh", "sh", "-c", command, (char *) 0);
		/* We should never get here if execlp goes well */
		perror("execlp");
		_exit(127);
	}
	/* We don't write to the pipe */
	close(pfd[1]);

	/* Read the output off the pipe */
	if ((n=read(pfd[0], sout, obufsz-1)) < 0) {
		perror("read");
		return -1;
	}
		
	sout[n] = '\0';
	/* everything went fine */
	close(pfd[0]);
	waitpid(child_pid, &status, 0);
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) != 0) {
			return -1;
		}
		return 0;
	}
	return -1;
}

void
parse_rule(const char * infile)
{
	char line[BUF_SIZE], buf[BUF_SIZE];
	FILE *f;
	char *p, *attname;
	char rule_name[256];
	char rule_filter[256];
	char any_str[256];
	int  threshold, interval;
	char *tmp;
	
	if ((f = fopen(infile, "r")) == NULL) {
		perror("fopen");
		exit(1);
	}
	rule_filter[0]='\0';
	any_str[0]='\0';
	threshold = 1;
	interval = -1;
	while (fgets(line, BUF_SIZE, f) != NULL) {
		/* skip comment and blank */	
		for (p=line; isspace(*p); ++p);
		if (*p == '\0' || *p == '#' || *p == '\n') {
			continue;
		}
		/* replace the newline char with null*/
		if (p[strlen(p)-1]=='\n') { 
			p[strlen(p)-1] = '\0';
		}

		strcpy(buf, p);
		attname = strtok(buf, "=");
		if (strchr(attname, '{')) {
			/* Rule name  */
			for (tmp = rule_name; *p && !strchr("\t ", *p); ) {
				*tmp++ = *p++;
			}
			*tmp='\0';
			continue;
		}		
		
		if (strstr(attname, "filter")) {
			/* Filter */
			p = strchr(p, '=');
			p++;
			/* get rid of leading space */
			for (; isspace(*p); ++p);
 			if (*p == '\n' || *p == '\0') {
				fprintf(stderr, "Error - filter string\n");
				exit(1);
			}
			
			
			/* Look for the open quote */
			if (*p != '\'') {
				fprintf(stderr, "Error - Missing open quote.\n");
				exit(1);
			}
			
			tmp = rule_filter;
			*tmp++ = *p++; 	/* Copy the open quote */
			for ( ; *p && !strchr("'", *p); ) {
		       		*tmp++ = *p++;
			}
			if (*p == '\n' || *p == '\0') {
				fprintf(stderr, "Error - filter string\n");
				exit(1);
			}
			*tmp++ = *p++;	/* Copy the closing quote */
			*tmp='\0';
			continue;
		}
		if (strstr(attname, "forany")) {
			/* Filter */
			p = strchr(p, '=');
			p++;
			/* get rid of leading space */
			for (; isspace(*p); ++p);
 			if (*p == '\n' || *p == '\0') {
				fprintf(stderr, "Error - filter string\n");
				exit(1);
			}
			
			/* Look for the open quote */
			if (*p != '\"' && *p != '\'') {
				fprintf(stderr, "Error - Missing open quote.\n");
				exit(1);
			}
			
			tmp = any_str;
			*tmp++ = *p++; 	/* copy the open quote */
			for ( ; *p && !strchr("\"", *p) && !strchr("'", *p) ; ) {
		       		*tmp++ = *p++;
			}
			if (*p == '\n' || *p == '\0') {
				fprintf(stderr, "Error - any_str string\n");
				exit(1);
			}
			*tmp++ = *p++;	/* Copy the closing quote */
			*tmp='\0';
			continue;
		}
		if (strstr(attname, "threshold")) {
			/* Threshold */
			char *endchar = 0;
			p = strchr(p, '=');
			p++;
			threshold = strtol(p, &endchar, 0);
			if (p == (char *)&endchar ) {
				fprintf(stderr, "Error - threshold\n");
				exit(1);
			}
			continue;
		}
		if (strstr(attname, "interval")) {
			/* Interval */
			char *endchar = 0;
			int i;
			p = strchr(p, '=');
			p++;
			/* get rid of leading space */
			for (; isspace(*p); ++p);
 			if (*p == '\n' || *p == '\0') {
				fprintf(stderr, "Error - filter string\n");
				exit(1);
			}
			/* Look for the open quote */
			if (*p == '\"' || *p == '\'') {
				p++;
			}
			
			i = strtol(p, &endchar, 0);
			if (p == (char *) &endchar) {
				fprintf(stderr, "Error - interval\n");
				exit(1);
			}
			
			if (*endchar == '\0' || *endchar == '\n' || 
			    *endchar == 's' || *endchar == 'S'  || 
		            *endchar == ' ' || *endchar == '\"' ||
			    *endchar == '\'') {
				interval = i;
			} else if (*endchar == 'm' || *endchar == 'M') {
				interval = i * 60;
			} else if (*endchar == 'h' || *endchar == 'H') {
				interval = i * 60 * 60;
			} else if (*endchar == 'd' || *endchar == 'D') {
				interval = i * 60 * 60 * 24;
			} else {
				fprintf(stderr, "Error - interval %c-- \n", *endchar);
				exit(1);
			}
			continue;
		}
		/* We should have all params now */		
		if (strchr(attname, '}')) {
			char argsbuf[512];
			char sout[512];
			if (strlen(any_str) > 0) {
				sprintf(argsbuf, "-a %s -n %s -f %s -y %s -t %d -i %d",  
						ela_bin, 
						rule_name, 
						rule_filter,
						any_str, 
						threshold, 
						interval);
			} else {	
				sprintf(argsbuf, "-a %s -n %s -f %s -t %d -i %d",  
						ela_bin, 
						rule_name, 
						rule_filter, 
						threshold, 
						interval);
			}
			/* Print rule */
			//fprintf(stdout, "-a %s --name %s --filter %s -y %s -t %d -i %d\n", 
			//	ela_bin, rule_name, rule_filter, any_str, threshold, interval);
			
			if (run_cmd("/sbin/evlnotify", argsbuf, sout, sizeof(sout)) != 0) {
				fprintf(stderr, "Error - register event name=%s\n", rule_name);
				exit(1);
			}
			rule_filter[0]='\0';
			any_str[0]='\0';
			threshold = 1;
			interval = -1;
			continue;
		}
	}	
}
	

void
usage()
{
	fprintf(stderr, "Usage: %s -f filename\n", progName);
	exit(1);
}

int 
main (int argc, char * argv[])
{
	auto int c;
	char *infile;
	uid_t uid = getuid();

	if (uid != 0) {
		fprintf(stderr, "ERROR: Only root is allowed to use the ela tools.\n");
		exit(1);
	}
													
	progName = argv[0];
	
	if (argc < 2) {
		usage();
	}
	while ((c = getopt(argc, argv, "hf:")) != EOF) {
		switch (c) {
			case 'f':
				infile = optarg;
				break;
			case 'h':
				usage();
				break;
			default:
				usage();
				break;
		}
	}


	parse_rule(infile);
	return 0;
}
		
