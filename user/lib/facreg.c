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

/*
 * Facility registry:
 * TODO: Update this comment to reflect newly added, optional fields such
 * as "kernel" and filter.
 *
 * A text file with three fields.  The first field is the numeric facility
 * code (which may be decimal, hex, or octal).  The second field is the
 * facility name, which is truncated to POSIX_LOG_MEMSTR_MAXLEN characters,
 * including the trailing null.  The third field, which may be omitted,
 * specifies access restrictions on records logged by this facility.
 *
 * White space (spaces and/or tabs) separate the fields.  For each line
 * in the registry file, leading and trailing white space is ignored.
 *
 * Currently, the only recognized access restriction is the word "private",
 * which indicates that records for that facility will be logged to a
 * read-protected log file.
 *
 * If a facility name contains spaces or tabs, it must be included in
 * double quotes -- e.g.,
 *	45  "Bob's Volume Manager"  private
 */

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_common.h"


#define FACREGPATH  LOG_EVLOG_CONF_DIR "/facility_registry"
#define INVALID_FAC_NAME ""

/* An empty hash slot has a facility code of EVL_INVALID_FACILITY. */
#define EMPTY EVL_INVALID_FACILITY

/*
 * These variables define the facility registry:
 * facilities is the current in-core image of the registry.
 * faCapacity is the size (in slots) to be used when we allocate the hash array
 *	for a new registry.  The initial value of this number is not critical,
 *	since it's OK if it's too big and we bump it up if it's too small...
 *	although name lookups get slower as the array gets bigger.
 * lastMod is the timestamp of the current rev of the registry file.
 * lastSize is the size, in bytes, of the current rev of the registry file.
 *	(We compare both timestamp and size to determine whether the file
 *	has changed, because it's possible for evlogd to receive and
 *	execute multiple facilitation requests in a single second.)
 * nextCheck is the time of the next scheduled check of the registry file's
 *	last-modified time and size.
 * evlCheckInterval is the interval, in seconds, between checks.  If
 *	evlCheckInterval is zero, we check the file every time.
 *
 * facRegMutex is the lock that controls access to the current in-core
 *	registry.  Unless otherwise stated, the static functions in this file
 *	assume that they are operating on a registry that is either under
 *	construction (and therefore not generally accessible) or locked.
 */
static _evlFacilityRegistry *facilities = NULL;
static int faCapacity = 50;
static time_t lastMod = 0;
static off_t lastSize = 0;
static time_t nextCheck = 0;
long evlCheckInterval = 5;	/* default interval = 5 seconds */
char *evlFacilityRegistryPath = FACREGPATH;

/*
 * Appropriate error reporting depends on the circumstances under which the
 * facility registry is read.  For example, we don't need a full list of
 * errors every time evlview is run.  These variables and the functions
 * _evlFacRegErrSet, facRegErrReset, and badFac control this.
 */
static int nErrors = 0;
static int maxErrors = 5;
static const char *firstErrorMsg =
"Warning: facility registry %s contains errors\n"
"and will not be used in its current state.\n";

#ifdef _POSIX_THREADS
static pthread_mutex_t facRegMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Note that _evlLockFacReg and _evlUnlockFacReg lock the data structure,
 * but not the facility registry file.  We lock the file separately.
 */
void
_evlLockFacReg()
{
#ifdef _POSIX_THREADS
	int status = pthread_mutex_lock(&facRegMutex);
	assert(status == 0);
#endif
}

void
_evlUnlockFacReg()
{
#ifdef _POSIX_THREADS
	(void) pthread_mutex_unlock(&facRegMutex);
#endif
}

/* Block signals while reading the registry, since fgets isn't signal-safe. */
int
_evlBlockSignals(sigset_t *oldset)
{
	int status;
	sigset_t allSigs;

	(void) sigfillset(&allSigs);
#ifdef _POSIX_THREADS
	status = pthread_sigmask(SIG_BLOCK, &allSigs, oldset);
	if (status != 0) {
		errno = status;
		perror("pthread_sigmask");
		return -1;
	}
#else
	status = sigprocmask(SIG_BLOCK, &allSigs, oldset);
	if (status != 0) {
		perror("sigprocmask");
		return -1;
	}
#endif
	return 0;
}

void
_evlRestoreSignals(sigset_t *oldset)
{
	int status;
#ifdef _POSIX_THREADS
	status = pthread_sigmask(SIG_SETMASK, oldset, NULL);
	if (status != 0) {
		errno = status;
		perror("pthread_sigmask (unblock)");
	}
#else
	status = sigprocmask(SIG_SETMASK, oldset, NULL);
	if (status != 0) {
		perror("sigprocmask (unblock)");
	}
#endif
}

static void
badFac(const char *file, int lineNum, const char *fmt, ...)
{
	va_list args;

	nErrors++;
	if (nErrors == 1 && firstErrorMsg) {
		fprintf(stderr, firstErrorMsg, file);
	}

	if (maxErrors >= 0) {
		/* There's limit to the number of errors we'll report. */
		if (nErrors > maxErrors) {
			if (nErrors == maxErrors+1) {
				fprintf(stderr, "...\n");
			}
			return;
		}
	}

	va_start(args, fmt);
	fprintf(stderr, "%s:%d: ", file, lineNum);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

static void
facRegErrReset()
{
	nErrors = 0;
}

void
_evlFacRegErrSet(int newMaxErrors, const char *newFirstErrorMsg)
{
	_evlLockFacReg();
	maxErrors = newMaxErrors;
	firstErrorMsg = newFirstErrorMsg;
	_evlUnlockFacReg();
}

/*
 * Returns 1 if n is prime.
 * Not very efficient, but rarely used.  Note that we avoid using sqrt
 * because that would pull in the math library, and the performance gain
 * (if any) would be tiny given the small values of n we expect.
 */
static int
isPrime(unsigned int n)
{
	int i;
	if (n == 0) {
		return 0;
	}
	for (i = 2; i*i <= n; i++) {
		if (n % i == 0) {
			return 0;
		}
	}
	return 1;
}

static unsigned int
nextPrimeUp(unsigned int n)
{
	while (!isPrime(n)) {
		n++;
	}
	return n;
}

/*
 * If the indicated facility is already in the hash, return its slot index.
 * Otherwise return the index of the slot where it would be added.  Returns -1
 * if the hash is full and fac isn't in it.
 */
static int
findHashSlotFor(posix_log_facility_t fac, _evlFacility *hash, int hashSize)
{
	int bestSlot = (unsigned int) fac % hashSize;
	int slot = bestSlot;
	posix_log_facility_t foundFac;
	do {
		foundFac = hash[slot].faCode;
		if (foundFac == fac || foundFac == EMPTY) {
			return slot;
		}
		slot++;
		slot %= hashSize;	/* wrap to 0 if off the end */
	} while (slot != bestSlot);

	/* fac is not in the hash, and there's no room to add it. */
	return -1;
}

/*
 * The old hash table, oldHash, is too small.  Allocate a new hash table
 * with size newSize, and copy all the entries from oldHash into it.
 * Free oldHash.
 * Note: oldHash may be NULL, in which case oldSize must be zero.
 */
static _evlFacility *
reallocFacHash(_evlFacility *oldHash, int oldSize, int newSize)
{
	_evlFacility *newHash;
	int i;
	assert(newSize >= oldSize);
	assert(isPrime(newSize));

	newHash = (_evlFacility *) malloc(newSize * sizeof(_evlFacility));
	assert(newHash != NULL);

	/* First mark all new slots as unused. */
	for (i = 0; i < newSize; i++) {
		newHash[i].faCode = EMPTY;
	}
	for (i = 0; i < oldSize; i++) {
		posix_log_facility_t fac = oldHash[i].faCode;
		if (fac != EMPTY) {
			int j = findHashSlotFor(fac, newHash, newSize);
			assert(0 <= j && j < newSize);
			(void) memcpy(&newHash[j], &oldHash[i],
				sizeof(_evlFacility));
		}
	}
	free(oldHash);
	return newHash;
}

/*
 * Add the facility with the specified code, name, and access flags to
 * facility registry facReg, expanding facReg's hash table if necessary.
 */
static void
addFacToHash(_evlFacilityRegistry *facReg, posix_log_facility_t facNum,
	char *facName, int accessFlags, char * filter, posix_log_query_t *query)
{
	_evlFacility *fac;
	int slot = findHashSlotFor(facNum, facReg->frHash, facReg->frHashSize);
	if (slot == -1) {
		/* Out of room.  Allocate a larger hash. */
		int oldSize = facReg->frHashSize;
		int newSize = nextPrimeUp(oldSize*2);
		facReg->frHash = reallocFacHash(facReg->frHash, oldSize,
			newSize);
		facReg->frHashSize = newSize;

		/* Try again. */
		slot = findHashSlotFor(facNum, facReg->frHash, newSize);
		assert(slot != -1);
	}
	fac = &(facReg->frHash[slot]);
	fac->faCode = facNum;
	fac->faName = facName;
	fac->faAccessFlags = accessFlags;
	fac->faFilter = filter;
	fac->faQuery = query;
	facReg->frNFacilities++;

	/*
	 * TODO: Consider enlarging the hash array when it gets (say) 75%
	 * full rather than 100%.
	 */
}

/*
 * Allocate an empty facility registry, using the specified capacity for its
 * hash table.  (Actually, we use the next-higher prime if the capacity is
 * not a prime number.)
 */
static _evlFacilityRegistry *
allocFacReg(int capacity)
{
	
	_evlFacilityRegistry *facReg = (_evlFacilityRegistry *)
		malloc(sizeof(_evlFacilityRegistry));
	assert(facReg != NULL);
	facReg->frNFacilities = 0;
	facReg->frHashSize = nextPrimeUp(capacity);
	facReg->frHash = reallocFacHash(NULL, 0, facReg->frHashSize);
	return facReg;
}

static const char *
getFacNameByCode(posix_log_facility_t facNum, _evlFacilityRegistry *facReg)
{
	int i = findHashSlotFor(facNum, facReg->frHash, facReg->frHashSize);
	_evlFacility *fac;
	if (i == -1) {
		/* Full hash but no facNum */
		return NULL;
	}
	fac = &(facReg->frHash[i]);
	if (fac->faCode == EMPTY) {
		return NULL;
	} else {
		return fac->faName;
	}
}

const char *
_evlGetFacNameByCode(posix_log_facility_t facNum, _evlFacilityRegistry *facReg)
{
	return getFacNameByCode(facNum, facReg);
}
/*
 * Return the facility code of the facility with the given name in the given
 * hash table.  Comparison is case-insensitive.  Returns EVL_INVALID_FACILITY
 * if no match.
 *
 * Does a linear search through all slots, which is not very efficient;
 * but this function is not likely to be called nearly as often as
 * getFacNameByCode().
 */
static posix_log_facility_t
getFacCodeByCIName(const char *name, _evlFacilityRegistry *facReg)
{
	_evlFacility *fac, *hashEnd = facReg->frHash + facReg->frHashSize;

	if (!name) {
		return EVL_INVALID_FACILITY;
	}
	for (fac = facReg->frHash; fac < hashEnd; fac++) {
		if (fac->faCode != EMPTY
		    && !_evlCIStrcmp(fac->faName, name)) {
			return fac->faCode;
		}
	}
	return EVL_INVALID_FACILITY;
}

posix_log_facility_t
_evlGetFacCodeByCIName(const char *name, _evlFacilityRegistry *facReg)
{
	return getFacCodeByCIName(name, facReg);
}
/*
 * facNum and facName are from a registry entry.  If it has a non-POSIX name
 * for a POSIX code or a non-POSIX code for a POSIX name, print an appropriate
 * error message and return -1.  Else return 0.
 *
 * Note: This function now performs this check for ALL "standard" Linux
 * facilities, including those (such as AUTHPRIV) that are not part of the
 * POSIX standard.
 */
static int
checkPosixFacility(posix_log_facility_t facNum, const char *facName,
	const char *facRegPath, int lineNum)
{
	posix_log_facility_t posixFacNum;
	char posixFacName[POSIX_LOG_MEMSTR_MAXLEN];
	int isPosixName, isPosixNum;

	posixFacNum = _evlGetValueByCIName(_evlLinuxFacilities, facName,
		EVL_INVALID_FACILITY);
	isPosixName = (posixFacNum != EVL_INVALID_FACILITY);

	_evlGetNameByValue(_evlLinuxFacilities, facNum, posixFacName, 
						sizeof(posixFacName), "");
	isPosixNum = (strcmp(posixFacName, "") != 0);

	if (isPosixName) {
		if (facNum != posixFacNum) {
			badFac(facRegPath, lineNum,
"Standard facility %s has nonstandard code %u (expected %u)",
				facName, facNum, posixFacNum);
			return -1;
		}
	} else if (isPosixNum) {
		badFac(facRegPath, lineNum,
"Standard facility code %u has nonstandard name %s (expected %s)",
			facNum, facName, posixFacName);
		return -1;
	}
	return 0;
}

/*
 * Verify that every POSIX facility shows up in the facReg registry.
 * For each missing facility, print a message.
 * Note that we've already verified that each POSIX name in this registry
 * has the right code, and each POSIX code has the right name.
 *
 * Note: This function now performs this check for ALL "standard" Linux
 * facilities, including those (such as AUTHPRIV) that are not part of the
 * POSIX standard.
 *
 * Note that this function and checkPosixFacility() can yield redundant
 * messages about the same facility -- e.g.,
 *	Standard facility KERN has nonstandard code 1 (expected 0)
 * and later
 *	No entry in registry for standard facility KERN with code 0
 */
static void
checkMissingPosixFacilities(_evlFacilityRegistry *facReg,
	const char *facRegPath, int lineNum)
{
	const struct _evlNvPair *px;
	int i;
	for (px = _evlLinuxFacilities; px->nv_name; px++) {
		i = findHashSlotFor(px->nv_value, facReg->frHash,
			facReg->frHashSize);
		if (i == -1
		    || facReg->frHash[i].faCode == EMPTY) {
			badFac(facRegPath, lineNum,
"No entry in registry for standard facility %s with code %u",
				px->nv_name, px->nv_value);
		}
	}
}

/* Deallocate the indicated facility registry. */
static void
freeFacReg(_evlFacilityRegistry *facReg)
{
	_evlFacility *fac, *hashEnd;

	if (!facReg) {
		return;
	}
	hashEnd = facReg->frHash + facReg->frHashSize;
	for (fac = facReg->frHash; fac < hashEnd; fac++) {
		if (fac->faCode != EMPTY) {
			free(fac->faName);
			free(fac->faFilter);
			if (fac->faQuery) {
				posix_log_query_destroy(fac->faQuery);
				free(fac->faQuery);
			}
		}
	}

	free(facReg);
}

void _evlFreeFacReg(_evlFacilityRegistry *facReg)
{
	freeFacReg(facReg);
}

/*
 * tokens[] is the null-terminated array of tokens from an entry in the
 * facility registry.  tokens[0] and tokens[1] are the facility code and
 * facility name, respectively.  We parse the remainder of the tokens.
 * "private" and/or "kernel" flags are stored to *flags, and a pointer
 * to the filter string, if any, is stored to *filter.
 *
 * Returns -1 if there are any tokens we cam't account for, or 0 otherwise.
 *
 * This function must not write any error messages, because it can be called
 * by clients other than _evlReadFacilities().
 */
int
_evlCollectOptionalFacParams(char **tokens, int *flags, char **filter)
{
	int i;
	assert(tokens[0] != NULL);
	assert(tokens[1] != NULL);
	assert(flags != NULL);
	assert(filter != NULL);
	*flags = 0;
	*filter = NULL;

	i = 2;
	if (!tokens[i]) {
		return 0;
	}
	if (!strcmp(tokens[i], "private")) {
		*flags |= EVL_FACC_PRIVATE;
		i++;
		if (!tokens[i]) {
			return 0;
		}
	}
	if (!strcmp(tokens[i], "kernel")) {
		*flags |= EVL_FACC_KERN;
		i++;
		if (!tokens[i]) {
			return 0;
		}
	}
	*filter = tokens[i];
	return (tokens[i+1] ? -1 : 0);
}

/*
 * Read the entries of the facility registry file into an _evlFacilityRegistry
 * object.  Return a pointer to that object if there are no errors; else return
 * NULL.
 */
_evlFacilityRegistry *
_evlReadFacilities(const char *facRegPath)
{
	char line[200];
	
	int lineNum = 0;
	int nFacs = 0;
	const char *dupFacName;
	sigset_t oldset;
	int fd;
	FILE *f;
	int sigsBlocked;
	struct stat st;
	_evlFacilityRegistry *newFacReg;

	/* Reset count for badFac(). */
	facRegErrReset();

	fd = open(facRegPath, O_RDONLY);
	if (fd < 0) {
		perror(facRegPath);
		return NULL;
	}

	f = fdopen(fd, "r");
	if (f == NULL) {
		perror("fdopen");
		(void) close(fd);
		return NULL;
	}

	/* Update the time stamp. */
	if (stat(facRegPath, &st) < 0) {
		perror("stat");
		(void) fclose(f);	/* closes fd */
		return NULL;
	} else {
		lastMod = st.st_mtime;
		lastSize = st.st_size;
	}

	_evlLockFd(fd, F_RDLCK);
	sigsBlocked = (_evlBlockSignals(&oldset) == 0);

	newFacReg = allocFacReg(faCapacity);

	while (fgets(line, 200, f)) {
		char *s, *facName;
		long facNum;
		int accessFlags = 0;
		char *filter = NULL;
		posix_log_query_t *query = NULL;
		char *tokens[5+1];			/* maximum 5 tokens */
		int ntoken;
		
		lineNum++;
		if (line[0] == '#') {
			continue;
		}
		if (line[strlen(line) -1] == '\n') {
			line[strlen(line) -1] = '\0';
		}
		
		ntoken = _evlSplitCmd(line, 5+1, tokens);
		if (ntoken < 2 || ntoken > 5) {
			badFac(facRegPath, lineNum,
				"Facility entry cannot have fewer than 2 fields or more than 5");
			goto badFacility;
		}
		tokens[ntoken] = NULL;
		

		facNum = strtoul(tokens[0], &s, 0);
		if (s == line) {
			badFac(facRegPath, lineNum, "Missing facility number");
			goto badFacility;
		}
		if (facNum == EVL_INVALID_FACILITY) {
			badFac(facRegPath, lineNum,
				"Facility number %u is reserved", facNum);
			goto badFacility;
		}

		facName = tokens[1];
		if (facName == s) {
			/* e.g., 8USER */
			badFac(facRegPath, lineNum, "Malformed facility entry");
			goto badFacility;
		}

		if (strlen(facName) >= POSIX_LOG_MEMSTR_MAXLEN-1) {
			/* Truncate silently. */
			facName[POSIX_LOG_MEMSTR_MAXLEN-1] = '\0';
		}

		if (_evlCollectOptionalFacParams(tokens, &accessFlags, &filter)
		    < 0) {
			badFac(facRegPath, lineNum,
				"Syntax error in facility entry");
			goto badFacility;
		}

		/* Looks like a good entry.  Check for duplicates. */
		dupFacName = getFacNameByCode(facNum, newFacReg);
		if (dupFacName) {
			badFac(facRegPath, lineNum,
				"Facility code %u previously used for %s",
				facNum, dupFacName);
			goto badFacility;
		}
		if (getFacCodeByCIName(facName, newFacReg)
		    != EVL_INVALID_FACILITY) {
			badFac(facRegPath, lineNum,
				"Facility name %s used previously", facName);
			goto badFacility;
		}

		/*
		 * Not a duplicate.  If it's a POSIX name or number, make sure
		 * the corresponding number or name also matches.
		 */
		if (checkPosixFacility(facNum, facName, facRegPath, lineNum) < 0) {
			goto badFacility;
		}
		
		/* Looks good.  Add it to the hash table. */
		facName = strdup(facName);
		assert(facName != NULL);
		
		if (filter != NULL) {
			char error_str[200];
			int ret;
			filter = strdup(filter);
			assert(filter != NULL);
			/* If I can get the lock, I will lazily create the query later */
			
		    query = (posix_log_query_t *) malloc(sizeof(posix_log_query_t));
		    assert(query != NULL);
			if ((ret = posix_log_query_create(filter, EVL_PRPS_RESTRICTED,
             								query, error_str, 200)) != 0) {
             		
             	free(query);
				query = NULL;						
             	if (ret != EBUSY) {
					fprintf(stderr, "Could not create query! Error message: \n   %s.\n", error_str);		
					goto badFacility;
				}
			}
			
		}
			
		addFacToHash(newFacReg, facNum, facName, accessFlags, filter, query);
		continue;
badFacility:
		/* Keep scanning for more errors. */
		continue;
	}
	
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	_evlUnlockFd(fd);
	(void) fclose(f);	/* closes fd */

	/* Verify that all POSIX facilities are still defined. */
	checkMissingPosixFacilities(newFacReg, facRegPath, ++lineNum);

	if (nErrors == 0) {
		faCapacity = newFacReg->frHashSize;
		return newFacReg;
	} else {
		freeFacReg(newFacReg);
		return NULL;
	}
}

/*
 * Set up a facility registry using the entries in _evlLinuxFacilities,
 * and return a pointer to it.
 */
static _evlFacilityRegistry *
setUpDefaultRegistry()
{
	_evlFacilityRegistry *newFacReg = allocFacReg(faCapacity);
	const struct _evlNvPair *lx;
	char *facName;
	posix_log_facility_t facNum;

	for (lx = _evlLinuxFacilities; lx->nv_name; lx++) {
		/* strdup here because we may free later. */
		facName = strdup(lx->nv_name);
		facNum = lx->nv_value;
		assert(facName != NULL);
		addFacToHash(newFacReg, facNum, facName,
			(facNum == LOG_AUTHPRIV ? EVL_FACC_PRIVATE : 0), NULL, NULL);
	}
	return newFacReg;
}


/*
 * Has the registry file changed since we last read it?  1 = up to date (no filter
 * change)
 */
static int
facRegUpToDate(int force)
{
	struct stat st;
	if (!force && evlCheckInterval > 0) {
		/*
		 * Check the file's last-modified time only as scheduled.
		 * For performance considerations, we're counting on time()
		 * being faster than stat() -- about 4x by our calculations.
		 */
		time_t now = time(NULL);
		if (now <= nextCheck) {
			return 1;
		}
		nextCheck = now + evlCheckInterval;
	}

	/* Time to do a check. */
	if (stat(evlFacilityRegistryPath, &st) < 0) {
		/*
		 * File not accessible, so we certainly can't update our
		 * image from it.
		 */
		return 1;
	}
	return (lastMod >= st.st_mtime && lastSize == st.st_size);
}

/*
 * Ensures that we have an appropriate image of the facility registry.
 * Assumes the facility-registry lock is held.
 */
static _evlFacilityRegistry *
syncFacilities()
{
	_evlFacilityRegistry *newFacilities;

	if (facilities && facRegUpToDate(0)) {
		/* The usual case */
		return facilities;
	}

	if ((newFacilities = _evlReadFacilities(evlFacilityRegistryPath))
	    == NULL) {
		/*
		 * Couldn't find the facility registry file, or it contained
		 * errors.  Stay with the old copy, if any, or go with the
		 * hard-coded Linux facility list.
		 */
		if (!facilities) {
			facilities = setUpDefaultRegistry();
		}
	} else {
		/*
		 * Got a new image OK.  Discard the old one, if any.
		 */
		if (facilities) {
			freeFacReg(facilities);
		}
		facilities = newFacilities;
	}
	return facilities;
}

/*
 * Find name in the facility registry using a case-insensitive lookup,
 * and return the corresponding facility code, or EVL_INVALID_FACILITY
 * if no match.
 * Call this with the registry UNlocked.
 */
posix_log_facility_t
_evlGetFacilityCodeByCIName(const char *name)
{
	_evlFacilityRegistry *fr;
	posix_log_facility_t facNum;

	_evlLockFacReg();
	fr = syncFacilities();
	facNum = getFacCodeByCIName(name, fr);
	_evlUnlockFacReg();

	return facNum;
}

/*
 * Find the name associated with facility code facNum, store it in buf,
 * and also return a pointer to buf.  buf is assumed to be big enough to
 * hold the name (POSIX_LOG_MEMSTR_MAXLEN is guaranteed to be big enough).
 * If there's no match, buf is unchanged and return NULL.
 * Call this with the registry UNlocked.
 */
char *
_evlGetFacilityName(posix_log_facility_t facNum, char *buf)
{
	_evlFacilityRegistry *fr;
	const char *facName;

	assert (buf != NULL);
	_evlLockFacReg();
	fr = syncFacilities();
	facName = getFacNameByCode(facNum, fr);
	if (facName) {
		(void) strcpy(buf, facName);
	}
	_evlUnlockFacReg();

	return (facName ? buf : NULL);
}

/*
 * Returns the access information for facility facNum, or -1 if there is
 * no such facility in the registry.
 * Call this with the registry UNlocked.
 */
int
_evlGetFacilityAccess(posix_log_facility_t facNum)
{
	_evlFacilityRegistry *fr;
	_evlFacility *fac;
	int i, result;

	_evlLockFacReg();
	fr = syncFacilities();
	i = findHashSlotFor(facNum, fr->frHash, fr->frHashSize);
	if (i == -1) {
		/* Full hash but no facNum */
		result = -1;
	} else {
		fac = &(fr->frHash[i]);
		if (fac->faCode == EMPTY) {
			result = -1;
		} else {
			result = fac->faAccessFlags;
		}
	}
	_evlUnlockFacReg();
	return result;
}

/*
 * Returns the query and access information for facility facNum, or -1 if
 * there is no such facility in the registry.
 * Call this with the registry UNlocked.
 */
posix_log_query_t * _evlGetFacilityAccessQuery(posix_log_facility_t facNum, int * acc)
{
	_evlFacilityRegistry *fr;
	_evlFacility *fac;
	int i;
    posix_log_query_t * result;
	_evlLockFacReg();
	
	fr = syncFacilities();
	i = findHashSlotFor(facNum, fr->frHash, fr->frHashSize);
	if (i == -1) {
		/* Full hash but no facNum */
		*acc = -1;
		result = NULL;
	} else {
		fac = &(fr->frHash[i]);
		if (fac->faCode == EMPTY) {
			*acc = -1;
			result = NULL;
		} else {
			/* If the faFilter is not NULL, but the faQuery is NULL
			 * then the query is not created earlier
			 * and expect to be created now. The faFilter should be
			 * a good filter string
			 */
			if (fac->faFilter && !fac->faQuery) {
				char error_str[200];
				posix_log_query_t * query;
				query = (posix_log_query_t *) malloc(sizeof(posix_log_query_t));
				assert(query != NULL);
					
				if (posix_log_query_create(fac->faFilter, EVL_PRPS_RESTRICTED,
             								query, error_str, 200) != 0) {
					fprintf(stderr, "Could not create query! Error message: \n   %s.\n", error_str);
					free(query);
					query = NULL;
				}
				fac->faQuery = query;
			}	
			*acc = fac->faAccessFlags;
			result = fac->faQuery;
		}
	}
	_evlUnlockFacReg();
	return result;
}
/*
 * malloc an array and fill it with all the facility name-value pairs in the
 * registry.  Return a pointer to the null-terminated array.  Note that the
 * names are strdup-ed, and so should be freed by the caller when the caller
 * frees the array itself.
 *
 * This function is provided to support existing code that likes to use this
 * info in this form and doesn't care whether the contents of the registry
 * change subsequently.
 */
struct _evlNvPair *
_evlSnapshotFacilities()
{
	_evlFacilityRegistry *fr;
	_evlFacility *fac, *hashEnd;
	struct _evlNvPair *facArray;
	int nFac = 0;

	_evlLockFacReg();
	fr = syncFacilities();
	facArray = (struct _evlNvPair *) malloc(
		(fr->frNFacilities+1) * sizeof(struct _evlNvPair));
	assert(facArray != NULL);

	hashEnd = fr->frHash + fr->frHashSize;
	for (fac = fr->frHash; fac < hashEnd; fac++) {
		if (fac->faCode == EMPTY) {
			continue;
		}
		facArray[nFac].nv_name = strdup(fac->faName);
		assert(facArray[nFac].nv_name != NULL);
		facArray[nFac].nv_value = fac->faCode;
		nFac++;
		if (nFac >= fr->frNFacilities) {
			break;
		}
	}
	_evlUnlockFacReg();
	facArray[nFac].nv_name = NULL;
	return facArray;
}

/*
 * Copy facName to canonical, converting characters as necessary so that
 * canonical is the canonical version of facility name facName.  Returns
 * EINVAL if facName is null or empty, or if canonical is null; 0 otherwise.
 *
 * Here are the rules for forming a canonical name:
 * 1. The following bytes are passed through unchanged: ASCII digits,
 * ASCII lowercase letters, period, underscore, and any bytes outside
 * the ASCII code set.
 * 2. Uppercase ASCII letters are converted to lowercase.
 * 3. A space character is converted to an underscore.
 * 4. Any other ASCII character is converted to a period.
 */
int
_evlGenCanonicalFacilityName(const unsigned char *facName,
	unsigned char *canonical)
{
	const unsigned char *f;
	unsigned char *c;

	if (!facName || !canonical || facName[0] == '\0') {
		return EINVAL;
	}

	for (f=facName, c=canonical; *f; f++, c++) {
		unsigned int uf = *f;
		if ('A' <= uf && uf <= 'Z') {
			*c = uf | 0x20;		/* ASCII toupper(uf) */
		} else if (uf > 0x7f
		    || ('a' <= uf && uf <= 'z')
		    || ('0' <= uf && uf <= '9')
		    || uf == '.'
		    || uf == '_') {
			*c = uf;
		} else if (uf == ' ') {
			*c = '_';
		} else {
			*c = '.';
		}
	}
	*c = '\0';

	/* "." and ".." are reserved directory names, so we convert them. */
	if (!strcmp(canonical, ".") || !strcmp(canonical, "..")) {
		canonical[0] = '_';
	}
	return 0;
}

/*
 * This is the code for CRC algorithm #1 from
 * http://www.cl.cam.ac.uk/Research/SRG/bluebook/21/crc/node6.html.
 * It was chosen for simplicity rather than efficiency, since we don't expect
 * it to be called much.
 */

#define QUOTIENT 0x04c11db7

static unsigned int
crc32(const unsigned char *data, int len)
{
	unsigned int	result;
	int		i,j;
	unsigned char	octet;
    
	result = -1;
    
	for (i=0; i<len; i++) {
		octet = *(data++);
		for (j=0; j<8; j++) {
			if ((octet >> 7) ^ (result >> 31)) {
				result = (result << 1) ^ QUOTIENT;
			} else {
				result = (result << 1);
			}
			octet <<= 1;
		}
	}
 
	return ~result;	/* The complement of the remainder */
}

unsigned int
_evlCrc32(const unsigned char *data, int len)
{
	return crc32(data, len);
}

/*
 * Set *fcode to the canonical facility code that corresponds to the facility
 * name fname.  Return 0 on success.
 *
 * If fname is the upper or lower case name of a standard Linux facility,
 * return the corresponding facility code.
 *
 * Otherwise, compute the facility code as the CRC of the name.  If the
 * resulting code is the same as a reserved code (either a Linux code or
 * EVL_INVALID_FACILITY), we return EEXIST.
 */
int
evl_gen_facility_code(const char *fname, posix_log_facility_t *fcode)
{
	posix_log_facility_t facNum;
	size_t nameLen;
	char canonical[POSIX_LOG_MEMSTR_MAXLEN];
	char linuxName[POSIX_LOG_MEMSTR_MAXLEN];
	int status;

	if (!fname || !fcode) {
		return EINVAL;
	}

	nameLen = strlen(fname);
	if (nameLen == 0 || nameLen >= POSIX_LOG_MEMSTR_MAXLEN) {
		return EINVAL;
	}

	facNum = _evlGetValueByCIName(_evlLinuxFacilities, fname,
		EVL_INVALID_FACILITY);
	if (facNum != EVL_INVALID_FACILITY) {
		/* fname names a standard Linux facility. */
		*fcode = facNum;
		return 0;
	}

	status = _evlGenCanonicalFacilityName(fname, canonical);
	assert(status == 0);
	facNum = crc32(canonical, strlen(canonical));
	if (facNum == EVL_INVALID_FACILITY) {
		/* CRC happens to match EVL_INVALID_FACILITY */
		return EEXIST;
	}
	_evlGetNameByValue(_evlLinuxFacilities, facNum, linuxName, sizeof(linuxName), "");
	if (strcmp(linuxName, "") != 0) {
		/*
		 * CRC happens to match the Linux facility whose name is
		 * posixName.
		 */
		return EEXIST;
	}
	*fcode = facNum;
	return 0;
}

/*
 * Fork and exec the evlfacility command needed to add the indicated
 * facility name with the indicated flags and/or filter.  We leave all
 * error checking to the evlfacility command.  Returns:
 *	0 on success
 *	EINVAL if evlfacility runs but returns a non-zero status
 *	EAGAIN if we fail to fork, exec, or wait for the evlfacility command
 */
int
_evlAddFacilityToRegistry(const char *fname, unsigned int flags,
	const char *filter)
{
	const char *argv[20];
	const char *cmd = "/sbin/evlfacility";
	pid_t kid;
	int status;
	int i = 0;

	argv[i++] = cmd;
	argv[i++] = "--add";
	argv[i++] = fname;
	if ((flags & EVL_FACC_PRIVATE) != 0) {
		argv[i++] = "--private";
	}
	if ((flags & EVL_FACC_KERN) != 0) {
		argv[i++] = "--kernel";
	}
	if (filter) {
		argv[i++] = "--filter";
		argv[i++] = filter;
	}
	argv[i] = NULL;

	kid = fork();
	if (kid == -1) {
		return EAGAIN;
	} else if (kid == 0) {
		/* Child -- Note than we don't redirect fds 0/1/2 */
		execv(cmd, (char *const*) argv);
		perror("execv of evlfacility in _evlAddFacilityToRegistry()");
		return EAGAIN;
	}

	/* Parent */
	for (;;) {
		if (waitpid(kid, &status, 0) == -1) {
			if (errno != EINTR) {
				perror("waitpid for evlfacility in _evlAddFacilityToRegistry()");
				return EAGAIN;
			}
		} else {
			return (status == 0 ? 0 : EINVAL);
		}
	}
	/* NOTREACHED */
}

/*
 * If the registry already contains a facility whose name is fname, set
 * *fcode to the corresponding code and return success.
 * 
 * Otherwise, compute the facility code as the CRC of fname.  If there's already
 * a facility in the registry with that code, return EEXIST.
 * 
 * Otherwise we need to add the facility to the registry.  If we're not root,
 * return EPERM.  Otherwise run the "evlfacility -a" command to add the
 * facility.
 */
int
evl_register_facility(const char *fname, posix_log_facility_t *fcode)
{
	int status;

	if (!fname || !fcode) {
		return EINVAL;
	}

	if (posix_log_strtofac(fname, fcode) == 0) {
		/* fname is already in registry. */
		return 0;
	}
	
	status = evl_gen_facility_code(fname, fcode);
	if (status != 0) {
		/* fname is somehow unacceptable */
		return status;
	}

	if (geteuid() != 0) {
		return EPERM;
	}
	status = _evlAddFacilityToRegistry(fname, 0, "uid=root");
	if (status != 0) {
		return EAGAIN;
	}

	/* *fcode was set by evl_gen_facility_code(). */
	return 0;
}
