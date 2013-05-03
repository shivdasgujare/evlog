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
#include "evl_template.h"

/*
 * Test of thread safety in the template application API.
 * 
 * Usage: threadTest -f query -w workers -s seconds [-S]
 * - query is as for evlview.
 * - workers is the integer number of worker threads to run.
 * - seconds is the number of seconds the worker threads should work.
 * - -S says to print stats to stderr before exiting the program.
 * 
 * We read all the records in the event log into memory, then set the threads
 * to work counting up the records that match the query.  This should
 * theoretically cause multiple instances of posix_log_query_match()
 * to be running in parallel.  And, if the query specifies one or more
 * non-standard attributes, it should cause some concurrent template fussing
 * in lib/query/evaluate.c.
 * 
 * To increase contention, we have all the threads test all the records.
 * When a thread finishes its pass through the log, it starts over.
 * 
 */

#define MAX_THREADS (sysconf(_SC_THREAD_THREADS_MAX)-2)

struct workerStats {
	pthread_t	tid;
	int	nQueryMatches;	/* Should be same for each complete pass */
	int	nCompletePasses;
	int	finished;
	time_t	startTime;
	time_t	endTime;
};

struct logRecord {
	struct posix_log_entry	*entry;
	void			*data;
};

static const char *progName;
static struct workerStats *workerStats = NULL;
static int nWorkers;
static int nSeconds;
static int printStats = 0;
static posix_log_query_t query;
static posix_logd_t logdes;
static struct logRecord **log;
static int nRecords;
static int exitStatus;

static void
usage()
{
	fprintf(stderr, "Usage: %s -f query -w workers -s seconds [-S]\n",
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
	int fflag = 0, sflag = 0, wflag = 0;
	int status;
	long n;
	char *qstring = NULL;
	char errbuf[100];

	progName = argv[0];
	while ((c = getopt(argc, argv, "f:s:w:S")) != -1) {
		switch (c) {
		case 'f':
			if (fflag) {
				usage();
			}
			fflag++;
			qstring = optarg;
			break;
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


	if (!fflag || !sflag || !wflag) {
		usage();
	}
	if (optind < argc) {
		usage();
	}

	status = posix_log_query_create(qstring,
		POSIX_LOG_PRPS_SEEK | EVL_PRPS_TEMPLATE, &query, errbuf, 100);
	if (status != 0) {
		errno = status;
		perror("posix_log_query_create");
		fprintf(stderr, errbuf);
		exit(2);
	}

	workerStats = (struct workerStats*) calloc(nWorkers,
		sizeof(struct workerStats));
	assert(workerStats != NULL);
}

static void
readLogIntoMemory()
{
	int status;
	int maxRecid;
	char data[POSIX_LOG_ENTRY_MAXLEN];
	struct posix_log_entry entry;

	status = posix_log_open(&logdes, NULL);
	if (status != 0) {
		errno = status;
		perror("posix_log_open");
		exit(2);
	}

	/*
	 * Get the record ID from the last record.  Take that (plus 1, in case
	 * there's a record ID of 0) to be the max number of records in the log.
	 */
	status = posix_log_seek(logdes, NULL, POSIX_LOG_SEEK_LAST);
	if (status != 0) {
		errno = status;
		perror("posix_log_seek to last");
		exit(2);
	}

	status = posix_log_read(logdes, &entry, data, POSIX_LOG_ENTRY_MAXLEN);
	if (status != 0) {
		errno = status;
		perror("posix_log_read");
		exit(2);
	}
	maxRecid = entry.log_recid;

	/* Allocate an array to hold pointers to all the records. */
	log = (struct logRecord**) malloc((maxRecid+1) *
		sizeof(struct logRecord*));
	assert(log != NULL);

	/* Now read the log into the array. */
	status = posix_log_seek(logdes, NULL, POSIX_LOG_SEEK_START);
	if (status != 0) {
		errno = status;
		perror("posix_log_seek to start");
		exit(2);
	}

	nRecords = 0;
	do {
		struct logRecord *rec;

		status = posix_log_read(logdes, &entry, data,
			POSIX_LOG_ENTRY_MAXLEN);
		if (status == EAGAIN) {
			/* unexpected EOF */
			break;
		} else if (status != 0) {
			errno = status;
			perror("posix_log_read");
			exit(2);
		}

		rec = (struct logRecord*) malloc(sizeof(struct logRecord));
		assert(rec != NULL);
		rec->entry = (struct posix_log_entry*) malloc(sizeof(struct
			posix_log_entry));
		assert(rec->entry != NULL);
		(void) memcpy(rec->entry, &entry,
			sizeof(struct posix_log_entry));
		if (entry.log_size == 0) {
			rec->data = NULL;
		} else {
			rec->data = malloc(entry.log_size);
			assert(rec->data != NULL);
			(void) memcpy(rec->data, data, entry.log_size);
		}
		log[nRecords++] = rec;
	} while (entry.log_recid < maxRecid);

	assert(nRecords > 0);
	status = posix_log_close(logdes);
	assert(status == 0);
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

	now = time(NULL);
	myStats->startTime = now;
	quitTime = now + nSeconds;

	do {
		int i;
		int match;
		int nMatches = 0;
		for (i = 0; i < nRecords; i++) {
			status = posix_log_query_match(&query, log[i]->entry,
				log[i]->data, &match);
			assert(status == 0);
			nMatches += match;
		}

		myStats->nCompletePasses++;
		if (myStats->nCompletePasses == 1) {
			myStats->nQueryMatches = nMatches;
		} else {
			if (myStats->nQueryMatches != nMatches) {
				fprintf(stderr,
"%s: worker %d found %d matches on pass %d,\n"
"but % matches on previous pass(es)\n",
					progName, self, nMatches,
					myStats->nQueryMatches);
				exit(2);
			}
		}

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
	fprintf(stderr, "log records: %d\n", nRecords);
	for (w = 0; w < nWorkers; w++) {
		ws = workerStats + w;
		fprintf(stderr, "worker #%d:\n", w);
		fprintf(stderr, "\tstart time: %s", ctime(&ws->startTime));
		fprintf(stderr, "\tend time:   %s", ctime(&ws->endTime));
		fprintf(stderr, "\tcomplete passes: %d\n", ws->nCompletePasses);
		fprintf(stderr, "\tmatches per pass: %d\n", ws->nQueryMatches);
	}
}

static void
finish()
{
	int w;
	struct workerStats *ws;
	int nQueryMatches = workerStats[0].nQueryMatches;

	exitStatus = 0;
	for (w = 1; w < nWorkers; w++) {
		if (nQueryMatches != workerStats[w].nQueryMatches) {
			fprintf(stderr,
"%s: Workers disagree on number of matching records.\n", progName);
			exitStatus = 3;
			break;
		}
	}

	if (printStats || exitStatus != 0) {
		reportStats();
	}
}

main(int argc, char **argv)
{
	parseArgs(argc, argv);
	readLogIntoMemory();
	runWorkers();
	finish();
	exit(exitStatus);
}
