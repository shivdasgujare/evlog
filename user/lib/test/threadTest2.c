#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include <getopt.h>

#include "posix_evlog.h"

/*
 * Test of thread safety in posix_log_printf/posix_log_vprintf.
 * Part of investigation of Glenn Highland's bug #704012.
 * 
 * Usage: threadTest -w workers -s seconds [-S]
 * - workers is the integer number of worker threads to run.
 * - seconds is the number of seconds the worker threads should work.
 * - -S says to print stats to stderr before exiting the program.
 * 
 * We set the worker threads to work logging events using posix_log_printf
 * (which in turn calls posix_log_vprintf).
 */

#define MAX_THREADS (PTHREAD_THREADS_MAX-2)

struct workerStats {
	pthread_t	tid;
	int	nEventsLogged;
	int	finished;
	time_t	startTime;
	time_t	endTime;
};

static const char *progName;
static struct workerStats *workerStats = NULL;
static int nWorkers;
static int nSeconds;
static int printStats = 0;
static int exitStatus;

static void
usage()
{
	fprintf(stderr, "Usage: %s -w workers -s seconds [-S]\n",
		progName);
	exit(1);
}

static int
parseInt(const char *s, long *val)
{
	char *end;
	*val = strtol(s, &end, 0);
	if (end == s || *end != '\0') {
		return -1;
	}
	return 0;
}

static void
parseArgs(int argc, char **argv)
{
	int c;
	extern char *optarg;
	extern int optind;
	int sflag = 0, wflag = 0;
	int status;
	long n;
	char *qstring = NULL;
	char errbuf[100];

	progName = argv[0];
	while ((c = getopt(argc, argv, "s:w:S")) != -1) {
		switch (c) {
		case 's':
			if (sflag) {
				usage();
			}
			sflag++;
			if (parseInt(optarg, &n) == 0) {
				nSeconds = (int) n;
			} else {
				usage();
			}
			if (n < 1) {
				fprintf(stderr,
"%s: Test duration must be 1 second or more.\n", progName);
				exit(1);
			}
			break;
		case 'w':
			if (wflag) {
				usage();
			}
			wflag++;
			if (parseInt(optarg, &n) == 0) {
				nWorkers = (int) n;
			} else {
				usage();
			}
			if (n < 1 || n > MAX_THREADS) {
				fprintf(stderr,
"%s: Number of worker threads must be in the range 1-%d.\n",
					progName, MAX_THREADS);
				exit(1);
			}
			break;
		case 'S':
			printStats = 1;
			break;
		case '?':
			usage();
		}
	}


	if (!sflag || !wflag) {
		usage();
	}
	if (optind < argc) {
		usage();
	}

	workerStats = (struct workerStats*) calloc(nWorkers,
		sizeof(struct workerStats));
	assert(workerStats != NULL);
}

/* This is the start_routine for a worker thread. */
static void *
work(void *pvWorkerIndex)
{
	/* Note: s390x requires double cast here. */
	int self = (int) (long) pvWorkerIndex;
	struct workerStats *myStats = workerStats + self;
	int status;
	time_t now, quitTime;

	fprintf(stderr, "Worker #%d has pid=%d\n", self, getpid());

	now = time(NULL);
	myStats->startTime = now;
	quitTime = now + nSeconds;
	myStats->nEventsLogged = 0;

	do {
		posix_log_printf(LOG_USER, 3, LOG_DEBUG, 0x0,
			"Worker #%d logging event #%d", self,
			myStats->nEventsLogged++);
		now = time(NULL);
	} while (now < quitTime);

	myStats->endTime = now;
	myStats->finished = 1;

	/* end of thread */
	return NULL;
}

/*
 * This is run by the main thread.  Start all the worker threads; then wait
 * until they have completed.
 */
static void
runWorkers()
{
	int status;
	int w;

	for (w = 0; w < nWorkers; w++) {
		/* Note: s390x requires double cast here. */
		status = pthread_create(&workerStats[w].tid, NULL, work,
			(void*) (long) w);
		assert(status == 0);
	}

	for (w = 0; w < nWorkers; w++) {
		status = pthread_join(workerStats[w].tid, NULL);
		assert(status == 0 || status == ESRCH);
	}
}

static void
reportStats()
{
	int w;
	struct workerStats *ws;

	fprintf(stderr, "worker threads: %d\n", nWorkers);
	fprintf(stderr, "duration: %d seconds\n", nSeconds);
	for (w = 0; w < nWorkers; w++) {
		ws = workerStats + w;
		fprintf(stderr, "worker #%d:\n", w);
		fprintf(stderr, "\tstart time: %s", ctime(&ws->startTime));
		fprintf(stderr, "\tend time:   %s", ctime(&ws->endTime));
		fprintf(stderr, "\tevents logged: %d\n", ws->nEventsLogged);
	}
}

static void
finish()
{
	if (printStats || exitStatus != 0) {
		reportStats();
	}
}

main(int argc, char **argv)
{
	parseArgs(argc, argv);
	runWorkers();
	finish();
	exit(exitStatus);
}
