#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <wchar.h>

#include <posix_evlog.h>
#include <evlog.h>

#define ARRAY_FMT
#define STRARRAY_FMT

typedef unsigned char uchar;
typedef signed char schar;
typedef unsigned long long ulonglong;
typedef long long longlong;
typedef wchar_t wchar;

typedef int xint;
typedef uint uxint;

enum e { sun, mon, tue, wed, thu, fri, sat };
struct b1 {
	char c;
};
struct b2 {
	char c1:4;
	char c2:4;
};

int exitStatus = 0;

static void
checkStatus(int status, int evty)
{
	if (status != 0) {
		fprintf(stderr, "write of event type %d failed:\n", evty);
		errno = status;
		perror("posix_log_write:");
		exitStatus = 1;
	}
}

main()
{
	posix_log_facility_t facility = 8;
	int event_type;
	posix_log_severity_t severity = LOG_DEBUG;
	int status;

	event_type = 23001;
    {
	struct rec_23001 {
		uint	bf1_i:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
	};
	struct rec_23001 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23001);
    }
	event_type = 23002;
    {
	struct rec_23002 {
		uchar	bf1_c:2;
		ushort	bf1_s:2;
		uint	bf1_i:2;
	};
	struct rec_23002 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23002);
    }
	event_type = 23003;
    {
	struct rec_23003 {
		uchar	bf1_c:2;
		uchar	bf1_c2:2;
	};
	struct rec_23003 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23003);
    }
	event_type = 23004;
    {
	struct rec_23004 {
		uchar	bf1_c:2;
		uchar	bf1_c2:2;
		uchar	bf1_c3:2;
		uchar	bf1_c4:2;
		uchar	bf1_c5:2;
	};
	struct rec_23004 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23004);
    }
	event_type = 23005;
    {
	struct rec_23005 {
		uint	bf1_i:2;
		uint	bf1_i2:2;
		uint	bf1_i3:2;
		uint	bf1_i4:2;
		uint	bf1_i5:2;
	};
	struct rec_23005 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23005);
    }
	event_type = 23006;
    {
	struct rec_23006 {
		ushort	bf1_s:2;
		uchar	bf1_c2:2;
		uchar	bf1_c3:2;
		uchar	bf1_c4:2;
	};
	struct rec_23006 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23006);
    }
	event_type = 23007;
    {
	struct rec_23007 {
		uchar	bf1_c2:2;
		uchar	bf1_c3:2;
		uchar	bf1_c4:2;
		ushort	bf1_s:2;
	};
	struct rec_23007 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23007);
    }
	event_type = 23008;
    {
	struct rec_23008 {
		uchar	bf1_c:7;
		ushort	bf1_s:8;
		uint	bf1_i:16;
	};
	struct rec_23008 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23008);
    }
	event_type = 23010;
    {
	struct rec_23010 {
		uchar	bf1_c1:3;
		uchar	bf1_c2:3;
		uchar	bf1_c3:3;
		uchar	bf1_c4:3;
		uchar	bf1_c5:3;
	};
	struct rec_23010 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23010);
    }
	event_type = 23011;
    {
	struct rec_23011 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_23011 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23011);
    }
	event_type = 23012;
    {
	struct rec_23012 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uchar	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_23012 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23012);
    }
	event_type = 23013;
    {
	struct rec_23013 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uint	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_23013 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23013);
    }
	event_type = 23014;
    {
	struct rec_23014 {
		ushort	bf1_s1:3;
		uchar	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_23014 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23014);
    }
	event_type = 23015;
    {
	struct rec_23015 {
		uchar	bf1_c1:3;
		ushort	:0;
		uchar	bf1_c2:3;
		uchar	bf1_c3:3;
	};
	struct rec_23015 v1 = {1, 1, 1};
	v1.bf1_c2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23015);
    }
	event_type = 23016;
    {
	struct rec_23016 {
		ushort	bf1_s1:3;
		uchar	:0;
		uchar	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_23016 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23016);
    }
	event_type = 23017;
    {
	struct rec_23017 {
		ushort	bf1_s1:3;
		ushort	:0;
		uchar	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_23017 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23017);
    }
	event_type = 23018;
    {
	struct rec_23018 {
		ushort	bf1_s1:3;
		uchar	:0;
		ushort	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_23018 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23018);
    }
	event_type = 23019;
    {
	struct rec_23019 {
		uint	bf1_i:3;
		uchar	:0;
		uint	bf1_i2:3;
		ushort	:0;
		uint	bf1_i3:3;
	};
	struct rec_23019 v1 = {1, 1, 1};
	v1.bf1_i2 = 1;	/* gcc bug 5157 workaround */
	v1.bf1_i3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23019);
    }
	event_type = 23020;
    {
	struct rec_23020 {
		uchar	bf1_c:8;
		uchar	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_23020 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23020);
    }
	event_type = 23021;
    {
	struct rec_23021 {
		ushort bf1_s1:4;
		uchar bf1_c1:4;
		uchar bf1_c2:4;
		uchar bf1_c3:4;
		uchar bf1_c4:4;
		uchar c1;
	};
	struct rec_23021 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23021);
    }
	event_type = 23121;
    {
	struct rec_23121 {
		ushort bf1_s1:4;
		uchar bf1_c1:4;
		uchar bf1_c2:4;
		uchar bf1_c3:4;
		uchar bf1_c4:4;
		uchar c1:8;
	};
	struct rec_23121 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23121);
    }
	event_type = 23022;
    {
	struct rec_23022 {
		uint	bf1_i:16;
		ushort	bf1_s:7;
		ushort	bf1_c:9;
	};
	struct rec_23022 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23022);
    }
	event_type = 23023;
    {
	struct rec_23023 {
		uchar	bf1_c:7;
		ushort	bf1_s:10;
		uint	bf1_i:23;
	};
	struct rec_23023 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23023);
    }
	event_type = 23024;
    {
	struct rec_23024 {
		uchar	bf1_c:7;
		ushort	bf1_s:10;
		uint	bf1_i:23;
		uchar	c;
		uchar	bf1_c2:7;
		ushort	bf1_s2:10;
		uint	bf1_i2:23;
	};
	struct rec_23024 v1 = {1, 1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23024);
    }
	event_type = 23124;
    {
	struct rec_23124 {
		uchar	bf1_c:7;
		ushort	bf1_s:10;
		uint	bf1_i:23;
		uchar	c:8;
		uchar	bf1_c2:7;
		ushort	bf1_s2:10;
		uint	bf1_i2:23;
	};
	struct rec_23124 v1 = {1, 1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23124);
    }
	event_type = 23025;
    {
	struct rec_23025 {
		uint	bf1_i:15;
		uchar	c1;
		uchar	c2;
	};
	struct rec_23025 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23025);
    }
	event_type = 23125;
    {
	struct rec_23125 {
		uint	bf1_i:15;
		uchar	c1:8;
		uchar	c2:8;
	};
	struct rec_23125 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23125);
    }
	event_type = 23026;
    {
	struct rec_23026 {
		uint	bf1_i:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c;
	};
	struct rec_23026 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23026);
    }
	event_type = 23126;
    {
	struct rec_23126 {
		uint	bf1_i:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c:8;
	};
	struct rec_23126 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23126);
    }
	event_type = 23027;
    {
	struct rec_23027 {
		ulonglong	bf1_ll:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c;
	};
	struct rec_23027 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23027);
    }
	event_type = 23127;
    {
	struct rec_23127 {
		ulonglong	bf1_ll:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c:8;
	};
	struct rec_23127 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23127);
    }
	event_type = 23028;
    {
	struct rec_23028 {
		ulonglong	bf1_ll:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c;
		ushort	s1;
	};
	struct rec_23028 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23028);
    }
	event_type = 23128;
    {
	struct rec_23128 {
		ulonglong	bf1_ll:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c:8;
		ushort	s1:16;
	};
	struct rec_23128 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23128);
    }
	event_type = 23030;
    {
	struct rec_23030 {
		ulonglong	bf1_ll:2;
		uchar	c;
		ushort	s1;
		ulonglong	ll1;
		uint	i;
	};
	struct rec_23030 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23030);
    }
	event_type = 23130;
    {
	struct rec_23130 {
		ulonglong	bf1_ll:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:64;
		uint	i:32;
	};
	struct rec_23130 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23130);
    }
	event_type = 23031;
    {
	struct rec_23031 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		ulonglong	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_23031 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23031);
    }
	event_type = 23032;
    {
	struct rec_23032 {
		uxint	bf1_i:2;
	};
	struct rec_23032 v1 = {1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23032);
    }
	event_type = 23033;
    {
	struct rec_23033 {
		uint :2;
	};
	struct rec_23033 v1;
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23033);
    }
	event_type = 23035;
    {
	struct rec_23035 {
		uchar bf1_c1:2;
		uint :2;
		uchar bf1_c2:2;
	};
	struct rec_23035 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23035);
    }
	event_type = 23036;
    {
	struct rec_23036 {
		ulonglong bf1_ll:40;
		ushort bf1_s:2;
	};
	struct rec_23036 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23036);
    }
	event_type = 23037;
    {
	struct rec_23037 {
		ulonglong bf1_ll:40;
		ushort s;
	};
	struct rec_23037 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23037);
    }
	event_type = 23137;
    {
	struct rec_23137 {
		ulonglong bf1_ll:40;
		ushort s:16;
	};
	struct rec_23137 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23137);
    }
	event_type = 23038;
    {
	struct rec_23038 {
		uchar c;
		uchar bf1_c1:3;
		ushort bf1_s1:3;
		uint bf1_i1:3;
		ushort s;
	};
	struct rec_23038 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23038);
    }
	event_type = 23138;
    {
	struct rec_23138 {
		uchar c:8;
		uchar bf1_c1:3;
		ushort bf1_s1:3;
		uint bf1_i1:3;
		ushort s:16;
	};
	struct rec_23138 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23138);
    }
	event_type = 23039;
    {
	struct rec_23039 {
		uchar c;
		uint bf1_i1:3;
		ushort bf1_s1:3;
		uchar bf1_c1:3;
		ushort s;
	};
	struct rec_23039 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23039);
    }
	event_type = 23139;
    {
	struct rec_23139 {
		uchar c:8;
		uint bf1_i1:3;
		ushort bf1_s1:3;
		uchar bf1_c1:3;
		ushort s:16;
	};
	struct rec_23139 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23139);
    }
	event_type = 23040;
    {
	struct rec_23040 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uchar	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
		uchar	c;
	};
	struct rec_23040 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23040);
    }
	event_type = 23140;
    {
	struct rec_23140 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uchar	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
		uchar	c:8;
	};
	struct rec_23140 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23140);
    }
	event_type = 23041;
    {
	struct rec_23041 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uint	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:2;
		uchar	c;
	};
	struct rec_23041 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23041);
    }
	event_type = 23141;
    {
	struct rec_23141 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uint	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:2;
		uchar	c:8;
	};
	struct rec_23141 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23141);
    }
	event_type = 23042;
    {
	struct rec_23042 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uchar	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_23042 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23042);
    }
	event_type = 23043;
    {
	struct rec_23043 {
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		uchar	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_23043 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23043);
    }
	event_type = 23044;
    {
	struct rec_23044 {
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_23044 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23044);
    }
	event_type = 23045;
    {
	struct rec_23045 {
		uchar	c;
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_23045 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23045);
    }
	event_type = 23145;
    {
	struct rec_23145 {
		uchar	c:8;
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_23145 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23145);
    }
	event_type = 23046;
    {
	struct rec_23046 {
		uint	i;
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_23046 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23046);
    }
	event_type = 23146;
    {
	struct rec_23146 {
		uint	i:32;
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_23146 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23146);
    }
	event_type = 23047;
    {
	struct rec_23047 {
		uchar	bf1_c:7;
		ushort	bf1_s:9;
		uint	bf1_i:23;
	};
	struct rec_23047 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23047);
    }
	event_type = 23052;
    {
	struct rec_23052 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
		uchar	:0;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_23052 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s4 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23052);
    }
	event_type = 23053;
    {
	struct rec_23053 {
		uchar	c;
		uchar	bf1_c1:4;
		ushort	:0;
		uint	bf1_i1:4;
	};
	struct rec_23053 v1 = {1, 1, 1};
	v1.bf1_i1 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23053);
    }
	event_type = 23153;
    {
	struct rec_23153 {
		uchar	c:8;
		uchar	bf1_c1:4;
		ushort	:0;
		uint	bf1_i1:4;
	};
	struct rec_23153 v1 = {1, 1, 1};
	v1.bf1_i1 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23153);
    }
	event_type = 23054;
    {
	struct rec_23054 {
		uchar	c[3]ARRAY_FMT;
		uint	bf1_i1:4;
	};
	struct rec_23054 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23054);
    }
	event_type = 23055;
    {
	struct rec_23055 {
		uchar	c[3]ARRAY_FMT;
		uint	bf1_i1:10;
	};
	struct rec_23055 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23055);
    }
	event_type = 23056;
    {
	struct rec_23056 {
		struct b1	b1;
		uint	bf1_i1:10;
	};
	struct rec_23056 v1 = {{1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23056);
    }
	event_type = 23057;
    {
	struct rec_23057 {
		struct b1	b1[3]STRARRAY_FMT;
		uint	bf1_i1:4;
	};
	struct rec_23057 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23057);
    }
	event_type = 23058;
    {
	struct rec_23058 {
		struct b2	b1[3]STRARRAY_FMT;
		uint	bf1_i1:4;
	};
	struct rec_23058 v1 = {{{1,1}, {1,1}, {1,1}}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23058);
    }
	event_type = 23059;
    {
	struct rec_23059 {
		ulonglong bf1_ll:40;
		uint bf1_i:2;
	};
	struct rec_23059 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23059);
    }
	event_type = 23060;
    {
	struct rec_23060 {
		ulonglong bf1_ll:40;
		uint bf1_i:8;
	};
	struct rec_23060 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23060);
    }
	event_type = 23061;
    {
	struct rec_23061 {
		ulonglong bf1_ll:40;
		ushort x:15;
	};
	struct rec_23061 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23061);
    }
	event_type = 23062;
    {
	struct rec_23062 {
		ulonglong bf1_ll:32;
		uint :8;
		ushort bf1_s:2;
	};
	struct rec_23062 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23062);
    }
	event_type = 23063;
    {
	struct rec_23063 {
		ulonglong bf1_ll:32;
		uint :8;
		ushort s;
	};
	struct rec_23063 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23063);
    }
	event_type = 23163;
    {
	struct rec_23163 {
		ulonglong bf1_ll:32;
		uint :8;
		ushort s:16;
	};
	struct rec_23163 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23163);
    }
	event_type = 23064;
    {
	struct rec_23064 {
		ulonglong bf1_ll:32;
		uint :8;
		uint bf1_i:2;
	};
	struct rec_23064 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23064);
    }
	event_type = 23065;
    {
	struct rec_23065 {
		ulonglong bf1_ll:32;
		uint :8;
		ushort x:15;
	};
	struct rec_23065 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23065);
    }
	event_type = 23066;
    {
	struct rec_23066 {
		ulonglong bf1_ll:40;
		uint bf1_i:25;
	};
	struct rec_23066 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23066);
    }
	event_type = 23067;
    {
	struct rec_23067 {
		ulonglong	bf1_ll:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:2;
		uint	i:32;
	};
	struct rec_23067 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23067);
    }
	event_type = 23068;
    {
	struct rec_23068 {
		ulonglong	bf1_ll:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:33;
		uint	i:32;
	};
	struct rec_23068 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23068);
    }
	event_type = 23069;
    {
	struct rec_23069 {
		uchar	c:5;
		ushort	s:9;
		uint	i:17;
	};
	struct rec_23069 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23069);
    }
	event_type = 23070;
    {
	struct rec_23070 {
		ulonglong	bf1_ll:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:33;
		uint	i:7;
	};
	struct rec_23070 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23070);
    }
	event_type = 23071;
    {
	struct rec_23071 {
		uint		i:2;
		ulonglong	ll:33;
	};
	struct rec_23071 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23071);
    }
	event_type = 23072;
    {
	struct rec_23072 {
		ulonglong	bf1_ll:2;
		uchar	c:6;
		ushort	s1:8;
		ulonglong	ll1:33;
		uint	i:10;
	};
	struct rec_23072 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23072);
    }
	event_type = 23073;
    {
	struct rec_23073 {
		uint	i1:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:33;
		uint	i:32;
	};
	struct rec_23073 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23073);
    }
	event_type = 23074;
    {
	struct rec_23074 {
		uchar	c1;
		uchar	c2;
		uchar	c3;
		ulonglong	ll1:33;
	};
	struct rec_23074 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23074);
    }
	event_type = 23075;
    {
	struct rec_23075 {
		uchar	c1;
		uchar	c2;
		uchar	c3;
		uchar	c4;
		ulonglong	ll1:33;
	};
	struct rec_23075 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23075);
    }
	event_type = 23076;
    {
	struct rec_23076 {
		uchar	c1;
		uchar	c2;
		uchar	c3;
		uchar	c4;
		uchar	c5;
		ulonglong	ll1:32;
	};
	struct rec_23076 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 23076);
    }

	exit(exitStatus);
}
