#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <wchar.h>

#include "posix_evlog.h"

static int exitStatus = 0;

static void
checkStatus(int status, int evty)
{
	if (status != 0) {
		fprintf(stderr, "write of event type %d failed:\n", evty);
		errno = status;
		perror("evl_log_printb");
		exitStatus = 1;
	}
}

main()
{
	int status = 0;
	posix_log_severity_t sev = LOG_INFO;
	posix_log_facility_t fac = LOG_USER;
	double pi = 3.1415926535897932384626433;
	wchar_t *wc, wstring[40];
	const char *c, *string = "Here is a wide string.";

	status = posix_log_printb(fac, 7000, sev, 0,
		"My name is %s %s, and I was born in %d.",
		"Abe", "Lincoln", 1809);
	checkStatus(status, 7000);

	status = posix_log_printb(fac, 7001, sev, 0,
		"%-15s %-15s %4u %4u-%4u",
		"Lincoln", "Abraham", 1809, 1861, 1865);
	checkStatus(status, 7001);

	status = posix_log_printb(fac, 7001, sev, 0,
		"%-15s %-15s %4u %4u-%4u",
		"Washington", "George", 1732, 1789, 1797);
	checkStatus(status, 7001);

	status = posix_log_printb(fac, 7002, sev, 0,
		"a: %a\n"
		"A: %A\n"
		"e: %e\n"
		"E: %E\n"
		"f: %f\n"
		"F: %F\n"
		"g: %g\n"
		"G: %G\n"
		".10f: %.10f",
		pi, pi, pi, pi, pi, pi, pi, pi, pi);
	checkStatus(status, 7002);

	for (c = string, wc = wstring; *wc++ = *c++; )
		;
	status = posix_log_printb(fac, 7003, sev, 0,
		"ls: %ls\n"
		"S: %S\n"
		"lc: %lc\n"
		"C: %C",
		wstring, wstring, (wchar_t) 'W', (wchar_t) 'W');
	checkStatus(status, 7003);

	exit(exitStatus);
}
