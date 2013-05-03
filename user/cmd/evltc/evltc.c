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
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <locale.h>
#include <assert.h>
#include <sys/wait.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "evl_list.h"
#include "evl_template.h"

extern void printFuncCall(evltemplate_t *t);
static char *cppSrcToTmpFile(const char *fileName, evl_list_t *cppOpts);

static char *progName;
static char *tmpFileName = NULL;
#define MAXTEMPLATES 1000

static void
usage()
{
	fprintf(stderr,
"Usage: %s [-f | --func] [-n | --noto] [-c | --cpp] [cpp opts] srcfile\n"
"  or   %s binfile.to\n",
		progName, progName);
	exit(1);
}

/* Called at exit. */
static void
rmTmpFile()
{
	if (tmpFileName) {
		(void) remove(tmpFileName);
	}
}

main(int argc, char **argv)
{
	char *fileName;
	int status = 0;
	int writeTmpls = 1;
	int printFuncs = 0;
	int runCpp = 0;
	template_t *templates[MAXTEMPLATES];
	size_t nTemplates;
	evl_list_t *cppOpts = NULL;

	static struct option longOptions[] = {
		{"cpp", 0, 0, 'c'},
		{"func", 0, 0, 'f'},
		{"noto", 0, 0, 'n'},
		{0, 0, 0, 0}
	};
	int c;

	progName = argv[0];

	/*
	 * Enable translation of multibyte characters in the "native"
	 * environment.
	 */
	(void) setlocale(LC_CTYPE, "");

	for (;;) {
		c = getopt_long(argc, argv, "cfnD:I:U:", longOptions, NULL);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'c':
			runCpp = 1;
			break;
		case 'f':
			printFuncs = 1;
			break;
		case 'n':
			writeTmpls = 0;
			break;
		case 'D':
		case 'I':
		case 'U':
		    {
			char *cppOpt;
			runCpp = 1;
			cppOpt = (char*) malloc(strlen(optarg) + 3);
			assert(cppOpt != NULL);
			snprintf(cppOpt, strlen(optarg) + 3, "-%c%s", c, optarg);
			cppOpts = _evlAppendToList(cppOpts, cppOpt);
			break;
		    }
		default:
			usage();
		}
	}

	if (optind != argc-1) {
		usage();
	}
	fileName = argv[optind];
	if (_evlEndsWith(fileName, ".to")) {
		/*
		 * fileName should be a binary template file.
		 * Ignore -f and -n.  Read in the template and print the
		 * evl_log_write() call that could create that record.
		 */
		evltemplate_t *t;

		if (runCpp) {
			fprintf(stderr,
"%s: Cannot specify cpp options with binary template file %s\n", progName, fileName);
			exit(1);
		}
		t = _evlReadTemplate(fileName);
		if (t == NULL) {
			fprintf(stderr,
"%s: Cannot read template from %s\n", progName, fileName);
			exit(2);
		}
		if (t->tm_header.th_type != TMPL_TH_EVLOG) {
			fprintf(stderr,
"%s: Nothing to do with struct template in %s\n", progName, fileName);
			exit(2);
		}
		templates[0] = t;
		nTemplates = 1;
		printFuncs = 1;
	} else {
		/* fileName should be a template source file. */
		char *srcFileName = fileName;
		if (runCpp) {
			tmpFileName = cppSrcToTmpFile(fileName, cppOpts);
			if (!tmpFileName) {
				exit(1);
			}
			srcFileName = tmpFileName;
			(void) atexit(rmTmpFile);
		}
		status = evl_parsetemplates(srcFileName, templates,
			MAXTEMPLATES, &nTemplates, stderr, progName);
		if (status == EBADMSG) {
			/* One or more errors have been reported to stderr. */
			exit(1);
		} else if (status == EINVAL) {
			/*
			 * Probably couldn't open the source file.  errno
			 * should still contain the reason.
			 */
			perror(srcFileName);
			exit(1);
		} else if (status != 0) {
			errno = status;
			perror("evl_parsetemplates");
			exit(2);
		}

		if (writeTmpls) {
			char *dir = _evlGetParentDir(fileName);
			status = evl_writetemplates(dir,
				(const template_t**) templates, nTemplates);
			if (status != 0) {
				errno = status;
				perror("evl_writetemplates");
				exit(1);
			}
		}
	}
	
	if (printFuncs) {
		int i;
		for (i = 0; i < nTemplates; i++) {
			printFuncCall(templates[i]);
		}
	}

	/* Kludge to test evl_freetemplate() */
	if (getenv("FREETMPLS") != NULL) {
		/* Free in reverse order, in case template n uses n-1. */
		int i;
		for (i = nTemplates-1; i >= 0; i--) {
			status = evl_freetemplate(templates[i]);
			if (status != 0) {
				fprintf(stderr,
"evl_freetemplate() of %s failed.\n", templates[i]->tm_name);
			}
		}
	}
	exit (0);
}

/*
 * Run the indicated file through cpp, creating a temp file.  If all goes
 * well, return the name of the temp file; else return NULL.
 */
static char *
cppSrcToTmpFile(const char *fileName, evl_list_t *cppOpts)
{
#ifndef CPPPATH
#define CPPPATH "/lib/cpp"
#endif
	char *tempFile, *tempDir;
	char *result;
	int argc, argsz;
	char **argv;
	pid_t pid;
	int kidStatus;
	evl_listnode_t *head, *end, *p;

	/*
	 * Make the temp file in the same directory as the real source file,
	 * since that's where we should look first for imports.
	 */
	tempDir = _evlGetParentDir(fileName);
	tempFile = (char*) malloc(strlen(tempDir) + 30);
	snprintf(tempFile, strlen(tempDir) + 30, "%s/tmplSrc%d", tempDir, getpid());
	free(tempDir);

	/* /usr/bin/cpp [cpp options] fileName tempFile NULL */
	argsz = 1 + _evlGetListSize(cppOpts) + 1 + 1 + 1;
	argv = (char**) malloc(argsz * sizeof(char*));
	assert(argv != NULL);

	argc = 0;
	argv[argc++] = CPPPATH;
	for (head=cppOpts, end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		argv[argc++] = (char*) p->li_data;
	}
	argv[argc++] = (char*) fileName;
	argv[argc++] = tempFile;
	argv[argc] = NULL;

	pid = fork();
	if (pid == 0) {
		/* I'm the child. */
		(void) execv(CPPPATH, argv);
		perror("Cannot exec " CPPPATH);
		exit(3);
	}

	/* I'm the parent.  Wait for the child to finish. */
	if (pid == -1) {
		perror("fork for cpp");
		exit(3);
	}
	(void) waitpid(pid, &kidStatus, 0);
	if (WIFEXITED(kidStatus) && WEXITSTATUS(kidStatus) == 0) {
		result = tempFile;
	} else {
		result = NULL;
		(void) remove(tempFile);
	}
	free(argv);
	return result;
}
