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

/* Note: This source is much more legible with tab spacing set to 4. */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>     	
#include <fcntl.h>
#include <sys/klog.h>
#include <sys/un.h>
#include <signal.h>           	
#include <errno.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"
#include "evl_util.h"
#include "evl_list.h"
#include "evl_common.h"

extern int _evlBlockSignals(sigset_t *oldset);

#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stderr, fmt, ##args)
#else
#define TRACE(fmt, args...)		/* fprintf(stderr, fmt, ##args) */
#endif

#define REMOVE_FAC			0
#define MODIFY_FAC			1
#define EVLFAC_REG_FILE			LOG_EVLOG_CONF_DIR "/facility_registry"
#define EVLFAC_REG_FILE_BAK		LOG_EVLOG_CONF_DIR "/facility_registry.bak"

#define max(a,b) (((a)>(b))?(a):(b))


/* Function prototypes */
int processFacilityCmd();
int addFacility(char * facName, unsigned int flags, char * filter);
int removeFacility(char * facName);
int modifyFacility(const char *modFacName, unsigned int flags,
	unsigned int canceledFlags, const char *filterStr);
int listFacilities();
int replaceRegistryFile(char const *replaceFile);
int verifyRegistryFile(char const *registryFile);
int writeFacility(int fd, char *facName, posix_log_facility_t facVal,
	unsigned int flags, char *filter);
int updateFacRegFile(const char * facName, const char *filter,
	unsigned int flags, unsigned int canceledFlags, int optype);
static void encodeEntry(char *newEntry, size_t size, const char *facName,
	posix_log_facility_t facVal, unsigned int flags, const char *filter);


char *progName = 0;

/* String args */
char *filterStr = NULL;
char *facilityName = NULL;
char *replaceFile = NULL;
char *verifyFile = NULL;
char *delFacName = NULL;
char *modFacName = NULL;


/* Boolean args */
int listFlag = 0;
int delFlag = 0;
int kernFlag = 0;
int userFlag = 0;
int privateFlag = 0;
int noprivateFlag = 0;
int forceFlag = 0;
int somethingBesidesHelp = 0;

#define BOOLARG 	1
#define STRINGARG 	2
#define INTARG 		3
#define	INTARRAYARG	4
#define OPTSTRINGARG 5

#define OPTSTRING_OMITTED "Optional String Omitted"

struct argInfo {
	char	*argName;
	char 	*shrtName;
	void	*argVal;
	int	argType;
	int	argSeen;
};

struct argInfo argInfo[] = {
	{ "--list",		"-l",		&listFlag,		BOOLARG,	0},
	{ "--filter",	"-f",		&filterStr,		STRINGARG,	0},
	{ "--add",		"-a",		&facilityName,	STRINGARG,	0},
	{ "--change",	"-c",		&modFacName,	STRINGARG,	0},
	{ "--replace",	"-r",		&replaceFile,	STRINGARG,	0},
	{ "--verify",	"-v",		&verifyFile,	OPTSTRINGARG,	0},
	{ "--delete",	"-d",		&delFacName,	STRINGARG,	0},
	{ "--kernel",	"-k",		&kernFlag,		BOOLARG,	0},
	{ "--user",		"-u",		&userFlag,		BOOLARG,	0},
	{ "--private",	"-p",		&privateFlag,	BOOLARG,	0},
	{ "--noprivate","-n",		&noprivateFlag,	BOOLARG,	0},
	{ "--force",	"-F",		&forceFlag,	BOOLARG,	0},
	{ NULL,			NULL,		NULL,			0,			0}
};


void usage()
{
	fprintf(stderr, "Usage:\n"
					"evlfacility -l | --list\n"
					"evlfacility -v | --verify [<file>]\n"
					"evlfacility -r | --replace <file>\n"
					"evlfacility -a | --add <facility-name> [-k | --kernel] [-F | --force]\n"
					"           [-p | --private] [-f | --filter <filter>]\n"
					"evlfacility -d | --delete <facility-name>\n"
					"evlfacility -c | --change <facility-name> [-k | --kernel] |  [-u | --user]\n"
					"           [-p | --private] | [-n | --noprivate] \n"
					"           [-f | --filter <filter> | nofilter] \n");
	exit(1);
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
 * etc.  No real semantic checks here -- e.g., --add with --delete.
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

		if (ai->argType == OPTSTRINGARG) {
			argv++;
			arg = *argv;
			if (!arg || arg[0] == '-') {
				argv--;
				arg = OPTSTRING_OMITTED;
			}
		} else if (ai->argType != BOOLARG) {
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
			case OPTSTRINGARG:
    		{
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
 * The -p, -u, -n, and -k options make no sense in connection with the
 * indicated option.
 */
static void
nopunk(const char *opt)
{
	badOptionCombo(opt, "--private");
	badOptionCombo(opt, "--user");
	badOptionCombo(opt, "--noprivate");
	badOptionCombo(opt, "--kernel");
}

/*
 * The command line has been parsed.  Verify that the selected combination
 * of options, and the values of those options, make sense.
 */
void
checkSemantics()
{
	int status;
    uid_t uid = getuid();
	if (uid != 0 && !listFlag && !verifyFile) {
		fprintf(stderr, "%s: Only root is allowed to add, delete, change, or replace.\n", progName);
		exit(1);
	}

	badOptionCombo("--private", "--noprivate");
	badOptionCombo("--kernel", "--user");

	if (forceFlag && !facilityName) {
		fprintf(stderr, "%s: --force requires --add.\n", progName);
		exit(1);
	}

	if (verifyFile) {
		badOptionCombo("--verify", "--list");
		badOptionCombo("--verify", "--add");
		badOptionCombo("--verify", "--change");
		badOptionCombo("--verify", "--delete");
    	badOptionCombo("--verify", "--replace");
		nopunk("--verify");

		/* --verify defaults to current registry. */
		if (!strcmp(verifyFile, OPTSTRING_OMITTED)) {
			verifyFile = EVLFAC_REG_FILE;
		}
	}	
	else if (listFlag) {
		badOptionCombo("--list", "--add");
		badOptionCombo("--list", "--change");
		badOptionCombo("--list", "--delete");
    	badOptionCombo("--list", "--replace");
		nopunk("--list");
	}	
	else if (replaceFile) {
		badOptionCombo("--replace", "--add");
		badOptionCombo("--replace", "--change");
		badOptionCombo("--replace", "--delete");
		nopunk("--replace");
	}		
	else if (facilityName) {
		badOptionCombo("--add", "--change");
		badOptionCombo("--add", "--delete");
		/* p, u, n, k options OK with --add */
	}
	else if (delFacName) {
		badOptionCombo("--delete", "--change");		
		nopunk("--delete");
	}
	else if (modFacName) {
		/* p, u, n, k options OK with --change */
	}
	else {
		usage();
	}			
	
	/* Validate the filter, if any */
	if (filterStr && strcmp(filterStr, "nofilter") != 0) {
		posix_log_query_t query;
		char error_str[200];
		
		if (posix_log_query_create(filterStr, EVL_PRPS_RESTRICTED,
          &query, error_str, 200) != 0) {
			fprintf(stderr, "%s: Error in filter:\n   %s\n", progName,
				error_str);
			exit(1);
		} else {
			(void) posix_log_query_destroy(&query);
		}
	}
}


int
main (int argc, char *argv[])
{
	progName = argv[0];

	/* Parse command line option */
	if (argc < 2) {
		usage();
	}
	processArgs(argc, argv);
	checkSemantics();
	//test_read();
	
	(void) _evlBlockSignals(NULL);
	if (processFacilityCmd() < 0) {
		exit(1);
	}
	
	exit(0);
} /* end main */


/* some test routine */
void
test_read()
{
	int nFac = 0;
	_evlFacility *fac, *hashEnd;
	_evlFacilityRegistry *facreg;
	facreg = _evlReadFacilities(EVLFAC_REG_FILE);

	hashEnd = facreg->frHash + facreg->frHashSize;
	for (fac = facreg->frHash; fac < hashEnd; fac++) {
		if (fac->faCode == EVL_INVALID_FACILITY) {
			continue;
		}
		printf("%i : %s : %i : %i : %s\n", fac->faCode, fac->faName,
											  fac->faAccessFlags & 1,
											  (fac->faAccessFlags & 2) >> 1,
											  fac->faQuery? fac->faQuery->qu_expr:"(null)");
		
		nFac++;
		if (nFac >= facreg->frNFacilities) {
			break;
		}
	}
	_evlFreeFacReg(facreg);
}
	
/*
 * FUNCTION	: processFacilityCmd
 *
 * PURPOSE	: process each command (add, remove, change, list)
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
int
processFacilityCmd()
{
	unsigned int flags = 0;
	unsigned int canceledFlags = 0;
	int ret = 0;
	
	if (facilityName) {
		/* --add */
		if (privateFlag) {
			flags |= EVL_FACC_PRIVATE;
		}
		if (kernFlag) {
			flags |= EVL_FACC_KERN;
		}
		ret = addFacility(facilityName, flags, filterStr);
	} else if (delFacName) {
		/* --delete */
	    ret = removeFacility(delFacName);
	} else if (modFacName) {				
		/* --change */
		if (noprivateFlag) {
			canceledFlags |= EVL_FACC_PRIVATE;
		}
		else if (privateFlag) {
			flags |= EVL_FACC_PRIVATE;
		}
		if (userFlag) {
			canceledFlags |= EVL_FACC_KERN;
		}
		else if (kernFlag) {
			flags |= EVL_FACC_KERN;
		}
		ret = modifyFacility(modFacName, flags, canceledFlags, filterStr);
	} else if (listFlag) {
		/* --list */
		ret = listFacilities();
	} else if (replaceFile) {
		/* --replace */
		ret = replaceRegistryFile(replaceFile);
	} else if (verifyFile) {
		/* --verify */
		ret = verifyRegistryFile(verifyFile);
	} else {
		usage();
		/* NOTREACHED */
	}
	return ret;
}

/*
 * For --add, --delete, and --change, we save a copy of the old registry
 * file, and then rewrite the file in place.  We keep the file locked only
 * while we're actually modifying it.
 *
 * For --replace, we save a copy of the old registry, then copy the new
 * version over the old.
 *
 * In all cases, we validate the new entry(ies) before making any changes.
 */

/*
 * cp fromPath toPath
 * We assume signals are blocked.
 * Return 0 on success or errno on failure.  If complainIfAbsent = 0
 * and fromPath doesn't exist, we don't complain but still return ENOENT.
 */
static int
copyFile(const char *fromPath, const char *toPath, int complainIfAbsent)
{
	int fromFd, toFd;
	int nBytesRead, nBytesWritten;
	int status = 0;
/* Note: SSIZE_MAX seems to be too big for an auto array. */
#define COPYBUFSZ (8*1024)
	char buf[COPYBUFSZ];

	assert(COPYBUFSZ <= SSIZE_MAX);

	fromFd = open(fromPath, O_RDONLY);
	if (fromFd < 0) {
		if (complainIfAbsent || errno != ENOENT) {
			fprintf(stderr, "%s: Cannot open %s for reading.\n",
				progName, fromPath);
			perror(fromPath);
		}
		return errno;
	}
	_evlLockFd(fromFd, F_RDLCK);
	
	toFd = open(toPath, O_CREAT | O_WRONLY | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH /* 0644 */);
	if (toFd < 0) {
		fprintf(stderr, "%s: Cannot open %s for writing.\n",
			progName, toPath);
		perror(toPath);
		goto fail;
	}
	_evlLockFd(toFd, F_WRLCK);

	do {
		nBytesRead = read(fromFd, buf, COPYBUFSZ);
		if (nBytesRead < 0) {
			perror(fromPath);
			goto fail;
		} else if (nBytesRead > 0) {
			nBytesWritten = write(toFd, buf, nBytesRead);
			if (nBytesWritten != nBytesRead) {
				perror(toPath);
				goto fail;
			}
		}
	} while (nBytesRead > 0);

	_evlUnlockFd(fromFd);
	close(fromFd);
	_evlUnlockFd(toFd);
	close(toFd);
	return 0;

fail:
	status = errno;
	if (fromFd >= 0) {
		_evlUnlockFd(fromFd);
		close(fromFd);
	}
	if (toFd >= 0) {
		_evlUnlockFd(toFd);
		close(toFd);
	}
	return status;
}

static int
saveOldRegistry(int complainIfAbsent)
{
	int status = copyFile(EVLFAC_REG_FILE, EVLFAC_REG_FILE_BAK,
		complainIfAbsent);
	if (status != 0 && complainIfAbsent) {
		fprintf(stderr, "%s: Internal error: Could not back up %s to %s\n",
			progName, EVLFAC_REG_FILE, EVLFAC_REG_FILE_BAK);
	}
	return status;
}

static void
restoreOldRegistry()
{
	if (copyFile(EVLFAC_REG_FILE_BAK, EVLFAC_REG_FILE, 1) != 0) {
		fprintf(stderr, "%s: Internal error: Could not restore %s from %s.\n",
			progName, EVLFAC_REG_FILE, EVLFAC_REG_FILE_BAK);
	}
}

static void
oldRegistryIsBad(const char *action)
{
	fprintf(stderr, "%s: Existing registry (%s) contains errors.\n",
		progName, EVLFAC_REG_FILE);
	fprintf(stderr, "Cannot %s existing registry.\n", action);
	fprintf(stderr, "Correct the existing registry with a text editor, and try again.\n");
}

/*
 * FUNCTION	: addFacility
 *
 * PURPOSE	: Add a new facility to the facility registry
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
int
addFacility(char * facName, unsigned int flags, char * filter)
{
	int fd;
	int ret = 0;
	posix_log_facility_t facVal, oldFacVal;
	_evlFacilityRegistry *facreg;
	int status;
	const char *dupFacName;
	int facNameExists = 0;

	facreg = _evlReadFacilities(EVLFAC_REG_FILE);
	if (!facreg) {
		oldRegistryIsBad("add entry to");
		return -1;
	}
	
	/* Check for duplicate facility name. */
	oldFacVal = _evlGetFacCodeByCIName(facName, facreg);
	if (oldFacVal != EVL_INVALID_FACILITY) {
		if (!forceFlag) {
			fprintf(stderr, "%s: Facility name \"%s\" already in use\n",
				progName, facName);
			_evlFreeFacReg(facreg);
			return -1;
		}
		facNameExists = 1;
	}

	status = evl_gen_facility_code(facName, &facVal);
	if (status != 0) {
		fprintf(stderr,
"%s: Cannot generate unique facility code for facility \"%s\"\n",
			progName, facName);
		if (status == EEXIST) {
			fprintf(stderr,
"Generated code matches a standard Linux facility.\n"
"Choose a different name and try again.\n");
		} else {
			errno = status;
			perror(facName);
		}
		_evlFreeFacReg(facreg);
		return -1;
	}

	if (facNameExists) {
		/*
		 * --force specified, name exists in registry.  Just
		 * verify that the code in the registry matches the one
		 * we would generate.
		 */
		status = 0;
		if (facVal != oldFacVal) {
			fprintf(stderr,
"%s: Facility \"%s\" already exists in registry, but with\n"
"non-standard facility code 0x%x (expected 0x%x).\n",
				progName, facName, oldFacVal, facVal);
			status = -1;
		}
		_evlFreeFacReg(facreg);
		return status;
	}

	/* Check for duplicate facility code. */
	dupFacName = _evlGetFacNameByCode(facVal, facreg);
	if (dupFacName != NULL) {
		fprintf(stderr,
"%s: Cannot generate unique facility code for facility \"%s\"\n",
			progName, facName);
		fprintf(stderr,
"Generated code matches existing facility \"%s\".\n"
"Choose a different name and try again.\n",
			dupFacName);
		_evlFreeFacReg(facreg);
		return -1;
	}
	_evlFreeFacReg(facreg);
	
	/* Things look good; write to registry */
	if (saveOldRegistry(1) != 0) {
		return -1;
	}

	fd = open(EVLFAC_REG_FILE, O_RDWR);
	if ( fd < 0) {
		perror(EVLFAC_REG_FILE);
		return -1;
	}
	_evlLockFd(fd, F_WRLCK);
	lseek(fd, 0, SEEK_END);
	
	if(writeFacility(fd, facName, facVal, flags, filter) != 0) {
		restoreOldRegistry();
		ret = -1;
	}

	_evlUnlockFd(fd);
	(void) close(fd);
	return ret;
}

/*
 * FUNCTION	: removeFacility
 *
 * PURPOSE	: remove facility from the facility registry
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
int
removeFacility(char * facName)
{
	_evlFacilityRegistry *facreg;

	if (_evlGetValueByCIName(_evlLinuxFacilities, facName,
	  EVL_INVALID_FACILITY) != EVL_INVALID_FACILITY) {
		fprintf(stderr, "%s: Cannot delete standard Linux facility \"%s\"\n",
			progName, facName);
		return -1;
	}

	facreg = _evlReadFacilities(EVLFAC_REG_FILE);
	if (!facreg) {
		oldRegistryIsBad("delete entry from");
		return -1;
	}
	
	if (_evlGetFacCodeByCIName(facName, facreg)== EVL_INVALID_FACILITY) {
		fprintf(stderr, "%s: Facility \"%s\" is not in registry.\n", progName,
			facName);
		return -1;
	}
	_evlFreeFacReg(facreg);

	return updateFacRegFile(facName, NULL, 0, 0, REMOVE_FAC);
}

/*
 * FUNCTION	: modifyFacility
 *
 * PURPOSE	: change (modify) a facility attributes
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
int
modifyFacility(const char *modFacName, unsigned int flags,
	unsigned int canceledFlags, const char *filterStr)
{
	_evlFacilityRegistry *facreg;
	facreg = _evlReadFacilities(EVLFAC_REG_FILE);
	if (!facreg) {
		oldRegistryIsBad("modify entry in");
		return -1;
	}
	
	if (_evlGetFacCodeByCIName(modFacName, facreg)== EVL_INVALID_FACILITY) {
		fprintf(stderr, "%s: Facility \"%s\" is not in registry.\n", progName,
			modFacName);
		return -1;
	}
	_evlFreeFacReg(facreg);
		
	return updateFacRegFile(modFacName, filterStr, flags, canceledFlags,
		MODIFY_FAC);
}

/*
 * FUNCTION	: listFacilities
 *
 * PURPOSE	: list all facilities to the facility registry
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
int
listFacilities()
{
	int fd;
	FILE *f;
	char line[512];
	
	fd = open(EVLFAC_REG_FILE, O_RDONLY);
	if (fd < 0) {
		perror(EVLFAC_REG_FILE);
		return -1;
	}
	if ((f = fdopen(fd, "r")) == NULL) {
		perror(EVLFAC_REG_FILE);
		close(fd);
		return -1;
	}
	_evlLockFd(fd, F_RDLCK);	
	
	while (fgets(line, 512, f)) {
		if (line[0] == '#' || line[0] == '\n' || strcspn(line, " ") == 0) {
			/* Skip line starts with #, empty line, blank line */
			continue;
		}
		
		fprintf(stdout, "%s", line);
	}
	_evlUnlockFd(fd);
	fclose(f);	/* closes fd */
	return 0;
}

/*
 * FUNCTION	: verifyRegistryFile
 *
 * PURPOSE	: verify that the indicated registry file will be accepted by
 *			  _evlReadFacilities()
 *
 * ARGS		: registryFile -- the pathname of the registry file to check
 *
 * RETURN	: -1 if there are errors, or 0 otherwise
 *
 */
int
verifyRegistryFile(const char *registryFile)
{
	int ret = 0;
	_evlFacility *fac, *hashEnd;
	_evlFacilityRegistry *newfacreg;
	posix_log_facility_t facCode;

	newfacreg = _evlReadFacilities(registryFile);
	if (newfacreg == NULL) {
		/* _evlReadFacilities will have printed the error message(s). */
		return -1;
	}

	/*
	 * Under circumstances we should NOT encounter here, _evlReadFacilities()
	 * can fail to create the query objects for some or all facilities.
	 * (They are created later as needed.)  Although this problem "can't happen"
	 * here, we go ahead and do the check anyway.
	 */
	hashEnd = newfacreg->frHash + newfacreg->frHashSize;
	for (fac = newfacreg->frHash; fac < hashEnd; fac++) {
		if (fac->faCode == EVL_INVALID_FACILITY) {
			continue;
		}
	    if (fac->faFilter && !fac->faQuery) {
			/* This shouldn't happen! */
			posix_log_query_t query;
			char error_str[200];
			int status;

			status = posix_log_query_create(fac->faFilter, EVL_PRPS_RESTRICTED,
				&query, error_str, 200);
			if (status != 0) {
				assert(status != EBUSY);
				fprintf(stderr,
"%s: Invalid filter for facility \"%s\":\n   %s\n",
					progName, fac->faName, error_str);
				ret = -1;
			} else {
				(void) posix_log_query_destroy(&query);
			}
		}
	}
	/* End of check for problems that can't happen. :-) */

	for (fac = newfacreg->frHash; fac < hashEnd; fac++) {
		if (fac->faCode == EVL_INVALID_FACILITY) {
			continue;
		}
		if (evl_gen_facility_code(fac->faName, &facCode) == 0) {
			if (fac->faCode != facCode) {
				fprintf(stderr,
"Note: The portable code for the facility with name \"%s\" is 0x%x, not 0x%x\n",
					fac->faName, facCode, fac->faCode);
			}
		} else {
			/*
			 * The algorithm for generating a portable code for this name
			 * fails.  Presumably, the code that's in the registry file
			 * is better than nothing.  Do nothing.
			 */
		}
	}

	_evlFreeFacReg(newfacreg);
	return ret;
}

/*
 * FUNCTION	: replaceRegistryFile
 *
 * PURPOSE	: replace the facility registry file with a
 *            new facility registry file
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
int
replaceRegistryFile(char const *replaceFile)
{
	int replacingFromBak = 0;
	int savedOldRegistry = 0;
	int status;

	if (!strcmp(replaceFile, EVLFAC_REG_FILE)) {
		fprintf(stderr, "%s: Cannot replace %s with itself!\n", progName,
			replaceFile);
		return -1;
	}

	if (!strcmp(replaceFile, EVLFAC_REG_FILE_BAK)) {
		replacingFromBak = 1;
	}

	if (verifyRegistryFile(replaceFile) != 0) {
		fprintf(stderr,
			"%s: Keeping old registry due to errors in new registry file %s\n",
			progName, replaceFile);
		return -1;
	}

	if (!replacingFromBak) {
		status = saveOldRegistry(0);
		if (status == ENOENT) {
			savedOldRegistry = 0;
		} else if (status == 0) {
			savedOldRegistry = 1;
		} else {
			return -1;
		}
	}

	status = copyFile(replaceFile, EVLFAC_REG_FILE, 1);
	if (status != 0) {
		fprintf(stderr, "%s: Could not copy %s over %s.\n", progName,
			replaceFile, EVLFAC_REG_FILE);
		if (savedOldRegistry) {
			restoreOldRegistry();
		} else if (!replacingFromBak) {
			fprintf(stderr, "%s: %s did not exist previously, so no copy was saved.\n",
				progName, EVLFAC_REG_FILE);
		}
		return -1;
	}

	return 0;
}

/*
 * FUNCTION	: writeFacility
 *
 * PURPOSE	: write a new facility to the facility registry
 *
 * ARGS		:	
 *
 * RETURN	:
 *
 */
int
writeFacility(int fd, char *facName, posix_log_facility_t facVal,
	unsigned int flags, char *filter)
{
	char newEntry[512];
	int nBytes;

	if (filter && !strcmp(filter, "nofilter")) {
		filter = NULL;
	}

	encodeEntry(newEntry, sizeof(newEntry), facName, facVal, flags, filter);
	nBytes = strlen(newEntry);
	
	if (write(fd, newEntry, nBytes) != nBytes) {
		perror("write of new facility entry");
		return -1;
	}
	return 0;												
}

/*
 * FUNCTION	: updateFacRegFile
 *
 * PURPOSE	: Update the facility file in place by either modifying (optype
 * = MODIFY_FAC) or removing (optype = REMOVE_FAC) the entry specified by
 * facName.
 * 
 * When we find the entry in question, we read the rest of the registry file
 * (beyond that entry) into memory, then tack it on after the entry has been
 * modified or deleted.
 * 
 * Before modifying the registry, we save a copy to a backup file.  If the
 * modification fails partway through, we restore the registry from that
 * backup copy.
 *
 * ARGS		:	
 *	facName:	The name of the facility whose entry is to be modified or
 *		deleted.
 *	filter, newflags, canceledFlags:	The new parameters of the entry to
 *		be modified, if optype = MODIFY_FAC.
 *	optype: MODIFY_FAC or REMOVE_FAC
 * RETURN	:
 *
 */
int
updateFacRegFile(const char *facName, const char *filter, unsigned int newflags,
	unsigned int canceledFlags, int optype)
{
	int fd;
	FILE *fp;
	char line[512];
	char *mov_buf;
	size_t cur_off, move_size;
	struct stat st;
    int line_len = 0;
	int line_no = 0;
	unsigned int oldflags = 0;
	char *oldFilter = NULL;

	if (saveOldRegistry(1) != 0) {
		return -1;
	}

	fd = open(EVLFAC_REG_FILE, O_RDWR);
	if (fd < 0) {
		perror("Cannot open registry for writing");
		return -1;
	}
	if ((fp = fdopen(fd, "r+")) == NULL) {
		perror("fdopen");
		close(fd);
		return -1;
	}
	_evlLockFd(fd, F_WRLCK);

	while (fgets(line, 512, fp)) {
		char *tokens[5+1];			/* maximum 5 tokens + terminate */
		int ntoken;
		unsigned int oldflags = 0;
		char *oldFilter;
		posix_log_facility_t facVal;
		
		line_no++;
		if (line[0] == '#' || line[0] == '\n' || strcspn(line, " ") == 0) {
			continue;
		}
		
		line_len = strlen(line);
		if (line[line_len-1] == '\n') {
			line[line_len-1] = '\0';
		}
		
		ntoken = _evlSplitCmd(line, 5+1, tokens);
		if (ntoken < 2 || ntoken > 5) {
			goto entryCorrupted;
		}
		tokens[ntoken] = NULL;
		
		
		if (!strcasecmp(tokens[1], facName)) {
			/* This is the entry we want to modify or remove. */

			if (optype == MODIFY_FAC) {
				/* Check for corrupt entry before hacking up the file. */
				char *s;
				if (_evlCollectOptionalFacParams(tokens, &oldflags, &oldFilter)
				  != 0) {
					goto entryCorrupted;
				}
				facVal = (posix_log_facility_t) strtoul(tokens[0], &s, 0);
				if (*s != '\0') {
					goto entryCorrupted;
				}
			}

			/* save the rest of the file into mov_buf */
			cur_off = ftell(fp);
			(void)fstat(fd, &st);
			move_size = st.st_size - cur_off;
			if ( move_size > 0) {
				mov_buf = (char *)malloc(move_size + 1);
				assert(mov_buf != NULL);

				if (fread(mov_buf, move_size, 1, fp) != 1) {
					perror("fread");
					goto giveUp;
				}
			}

			/*
			 * Move file pointer back to the end of previous line,
			 * and truncate the file at that point. We will write
			 * the portion of file that is saved in mov_buff after
			 * the modification takes place, or if it is a deletion
			 * operation we just write the mov_buff then return.
			 */
			if (fseek(fp, -(move_size + line_len), SEEK_CUR) != 0) {
				perror("fseek");
				goto giveUp;
			}
			if (ftruncate(fd, ftell(fp)) != 0) {
				perror("ftruncate");
				goto registryCorrupted;
			}
			
			if (optype == MODIFY_FAC) {
				char newEntry[512];

				newflags |= oldflags;
				newflags &= (~canceledFlags);

				/*
				 * filter -- Preserve the existing filter if none was
				 * specified, unless "nofilter" was specified.
				 */
				if (filter) {
					if (!strcmp(filter, "nofilter")) {
						filter = NULL;
					}
				} else if (oldFilter) {
					filter = oldFilter;
				}

				/* Write out the new entry. */
				encodeEntry(newEntry, sizeof(newEntry), facName, facVal, newflags, filter);
				if (fwrite(newEntry, strlen(newEntry), 1, fp) != 1) {
					perror("fwrite of new entry");
					goto registryCorrupted;
				}
			}

			/* Tack on the later entries. */
			if (move_size > 0) {
				if (fwrite(mov_buf, move_size, 1, fp) != 1) {
					perror("fwrite of later entries");
					goto registryCorrupted;
				}
				free(mov_buf);
			}
			
			/* Success */
			_evlUnlockFd(fd);
			fclose(fp);	/* closes fd */
			return 0;
		}
	}

	fprintf(stderr, "%s: Internal error: facility \"%s\" has disappeared from registry\n",
		progName, facName);
	goto giveUp;

entryCorrupted:
	fprintf(stderr, "%s: Corrupted facility entry at line %d\n", progName,
		line_no);
	oldRegistryIsBad("modify");
	/* FALLTHROUGH */
giveUp:
	_evlUnlockFd(fd);
	fclose(fp);	/* closes fd */
	return -1;

registryCorrupted:
	_evlUnlockFd(fd);
	fclose(fp);	/* closes fd */
	restoreOldRegistry();
	return -1;
}

/*
 * Add quote characters to the string quoteMe.  The resulting quoted string
 * is stored in buf.  Returns a pointer to buf.  As in libevl's splitCmd(),
 * there is no special handling of quotes or backslashes within the string.
 */
static char *
quoteUnconditionally(const char *quoteMe, char quote, char *buf, size_t size)
{
	snprintf(buf, size, "%c%s%c", quote, quoteMe, quote);
	return buf;
}

/*
 * Enclose string quoteMe in quotes if quoteMe contains any spaces or tabs.
 * The resulting string (quoted or not) is stored in buf.  Returns a pointer
 * to buf.  As in libevl's splitCmd(), there is no special handling of quotes
 * or backslashes within the string.
 */
static char *
quoteAsNeeded(const char *quoteMe, char quote, char *buf, size_t size)
{
	if (strchr(quoteMe, ' ') || strchr(quoteMe, '\t')) {
		snprintf(buf, size, "%c%s%c", quote, quoteMe, quote);
	} else {
		(void) strcpy(buf, quoteMe);
	}
	return buf;
}

/*
 * Encode the indicated info into a newline-terminated entry for the
 * facility registry.
 */
static void
encodeEntry(char *newEntry, size_t size, const char *facName, posix_log_facility_t facVal,
	unsigned int flags, const char *filter)
{
	char quoteBuf[512];

	if (facVal > 9999) {
		snprintf(newEntry, size, "0x%x ", facVal);
	} else {
		snprintf(newEntry, size, "%u ", facVal);
	}
	
	(void) strcat(newEntry, quoteAsNeeded(facName, '"', quoteBuf, sizeof(quoteBuf)));

	if ((flags & EVL_FACC_PRIVATE) != 0) {
		(void) strcat(newEntry, " private");
	}
	if ((flags & EVL_FACC_KERN) != 0) {
		(void) strcat(newEntry, " kernel");
	}

	if (filter) {
		(void) strcat(newEntry, " ");
		(void) strcat(newEntry, quoteUnconditionally(filter, '\'', quoteBuf, 
						sizeof(quoteBuf)));
	}

	(void) strcat(newEntry, "\n");
}
