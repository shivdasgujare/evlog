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
#include <sys/types.h>

#include <signal.h>           	
#include <errno.h>
#include <assert.h>
#include <grp.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/syscall.h>

static const char *progName;

void
usage()
{
	fprintf(stderr, "Usage: %s -p <pid> -r <recid> -f <io | platform>\n", progName);
	exit(1);
}
int 
main(int argc, char *argv[])
{
	auto int c;
	int recid = -1;
	pid_t pid = -1;
	int sub_id = 234;	/* just an extra field, that we can use */
	struct siginfo si;
	union sigval val;
	int ret;
	char *flag;

	progName = argv[0];
	if (argc < 2) {
		usage();
	}

	while ((c = getopt(argc, argv, "hp:r:f:")) != EOF) {
		switch (c) {
			case 'p':
				pid = atoi(optarg);
				break;
			case 'r':
				recid = atoi(optarg);
				break;	
			case 'f':
				flag = optarg;
				break;
			case 'h':
				usage();
				break;
		}
	}
	
	if (strcmp(flag, "io") && strcmp(flag, "platform")) {
		usage();
	}
	if (pid == -1 || recid == -1) {
		usage();
	}
	/* printf("pid=%d, recid=%d\n", pid, recid); */
	
	si.si_code = SI_QUEUE;
	if (!strcmp(flag, "io")) {
		si.si_signo = SIGRTMIN+15;
	} 
	else {
		si.si_signo = SIGRTMIN+14;
	}
	si.si_errno = recid;	

	/* 
	 * Notice the sub id code, in the sig_recv
	 * We can use this to pass some extra stuff
	 * if we want to
	 */
	memcpy(&si._sifields._rt.si_sigval, &sub_id, sizeof(sigval_t));

	if ((ret =syscall(SYS_rt_sigqueueinfo, pid, si.si_signo, &si)) != 0) {
		if (ret == -ESRCH) {
			fprintf(stderr, "Error: No such process\n");
			exit(1);
		}
		fprintf(stderr, "SYS_rt_sigqueueinfo return %d\n", ret);
		exit(1);
	}
			
	return 0;
}

	
