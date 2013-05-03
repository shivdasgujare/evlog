#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "posix_evlog.h"
#include "evl_template.h"

/*
 * Test of the template application API.  The command line specifies a
 * facility name or code, an event type, and an attribute name.  Read the
 * last record in the event log with that facility and event type.
 * In that record, access the named attribute, which must be an array of
 * structs.  Print the values of all the members of each element of that
 * array.
 *
 * Check aostest.t for some sample records.
 */

static const char *progName;

static void
usage()
{
	fprintf(stderr, "Usage: %s facility event_type attname\n", progName);
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

main(int argc, char **argv)
{
	long n;
	posix_log_facility_t fac;
	int evtype;
	const char *attName;
	posix_log_query_t query;
	char qstring[100];
	char errbuf[100];
	int status;
	posix_logd_t logdes;
	struct posix_log_entry entry;
	char data[POSIX_LOG_ENTRY_MAXLEN];
	evltemplate_t *tmpl;
	const evltemplate_t *structElement;
	evlattribute_t *att;
	evlatt_info_t attInfo;
	int i, nel;

	progName = argv[0];
	if (argc != 4) {
		usage();
	}

	if (parseInt(argv[1], &n) == 0) {
		fac = (posix_log_facility_t) n;
	} else if (posix_log_strtofac(argv[1], &fac) != 0) {
		usage();
	}

	if (parseInt(argv[2], &n) == 0) {
		evtype = (int) n;
	} else {
		usage();
	}

	attName = argv[3];

	snprintf(qstring, sizeof(qstring), "facility=%d && event_type=%d", fac, evtype);
	status = posix_log_query_create(qstring, POSIX_LOG_PRPS_SEEK, &query,
		errbuf, 100);
	if (status != 0) {
		errno = status;
		perror("posix_log_query_create");
		fprintf(stderr, errbuf);
		exit(2);
	}

	status = posix_log_open(&logdes, NULL);
	if (status != 0) {
		errno = status;
		perror("posix_log_open");
		exit(2);
	}

	status = posix_log_seek(logdes, &query, POSIX_LOG_SEEK_LAST);
	if (status != 0) {
		errno = status;
		perror("posix_log_seek");
		exit(2);
	}

	status = posix_log_read(logdes, &entry, data, POSIX_LOG_ENTRY_MAXLEN);
	if (status != 0) {
		errno = status;
		perror("posix_log_read");
		exit(2);
	}

	status = evl_gettemplate(&entry, data, &tmpl);
	if (status != 0) {
		errno = status;
		perror("evl_gettemplate");
		exit(2);
	}

	status = evl_populatetemplate(tmpl, &entry, data);
	if (status != 0) {
		errno = status;
		perror("evl_populatetemplate");
		exit(2);
	}

	status = evltemplate_getatt(tmpl, attName, &att);
	if (status != 0) {
		errno = status;
		perror("evltemplate_getatt");
		exit(2);
	}

	status = evlatt_getinfo(att, &attInfo);
	if (status != 0) {
		errno = status;
		perror("evlatt_getinfo");
		exit(2);
	}

	if (attInfo.att_type != TY_STRUCT || !attInfo.att_isarray) {
		fprintf(stderr, "%s is not an array of structs.\n", attName);
		exit(2);
	}

	nel = attInfo.att_dimpop;
	printf("%d elements\n", nel);
	for (i = 0; i < nel; i++) {
#define MAX_STRUCT_FIELDS 100
		evlattribute_t *fields[MAX_STRUCT_FIELDS];
		unsigned int nfields;
		int j;

		status = evlatt_getstructfromarray(att, i, &structElement);
		if (status != 0) {
			fprintf(stderr,
				"Couldn't get at element %d of array %s\n",
				i, attName);
			errno = status;
			perror("evlatt_getstructfromarray");
			exit(2);
		}

		printf("Element %d:\n", i);
		status = evltemplate_getatts(structElement, fields,
			MAX_STRUCT_FIELDS, &nfields);
		if (status != 0) {
			errno = status;
			perror("evltemplate_getatts");
			exit(2);
		}

		for (j = 0; j < nfields; j++) {
			char fmtbuf[1024];
			evlatt_info_t fieldInfo;

			status = evlatt_getstring(fields[j], fmtbuf, 1024);
			if (status != 0) {
				errno = status;
				perror("evlatt_getstring");
				exit(2);
			}

			status = evlatt_getinfo(fields[j], &fieldInfo);
			if (status != 0) {
				errno = status;
				perror("evlatt_getinfo for struct's field");
				exit(2);
			}

			printf("%s = %s\n", fieldInfo.att_name, fmtbuf);
		}
	}

	exit(0);
}
