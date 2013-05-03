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

	event_type = 33001;
    {
	struct rec_33001 {
		uint	bf1_i:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
	};
	struct rec_33001 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33001);
    }
	event_type = 33002;
    {
	struct rec_33002 {
		uchar	bf1_c:2;
		ushort	bf1_s:2;
		uint	bf1_i:2;
	};
	struct rec_33002 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33002);
    }
	event_type = 33003;
    {
	struct rec_33003 {
		uchar	bf1_c:2;
		uchar	bf1_c2:2;
	};
	struct rec_33003 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33003);
    }
	event_type = 33004;
    {
	struct rec_33004 {
		uchar	bf1_c:2;
		uchar	bf1_c2:2;
		uchar	bf1_c3:2;
		uchar	bf1_c4:2;
		uchar	bf1_c5:2;
	};
	struct rec_33004 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33004);
    }
	event_type = 33005;
    {
	struct rec_33005 {
		uint	bf1_i:2;
		uint	bf1_i2:2;
		uint	bf1_i3:2;
		uint	bf1_i4:2;
		uint	bf1_i5:2;
	};
	struct rec_33005 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33005);
    }
	event_type = 33006;
    {
	struct rec_33006 {
		ushort	bf1_s:2;
		uchar	bf1_c2:2;
		uchar	bf1_c3:2;
		uchar	bf1_c4:2;
	};
	struct rec_33006 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33006);
    }
	event_type = 33007;
    {
	struct rec_33007 {
		uchar	bf1_c2:2;
		uchar	bf1_c3:2;
		uchar	bf1_c4:2;
		ushort	bf1_s:2;
	};
	struct rec_33007 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33007);
    }
	event_type = 33008;
    {
	struct rec_33008 {
		uchar	bf1_c:7;
		ushort	bf1_s:8;
		uint	bf1_i:16;
	};
	struct rec_33008 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33008);
    }
	event_type = 33010;
    {
	struct rec_33010 {
		uchar	bf1_c1:3;
		uchar	bf1_c2:3;
		uchar	bf1_c3:3;
		uchar	bf1_c4:3;
		uchar	bf1_c5:3;
	};
	struct rec_33010 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33010);
    }
	event_type = 33011;
    {
	struct rec_33011 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_33011 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33011);
    }
	event_type = 33012;
    {
	struct rec_33012 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uchar	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_33012 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33012);
    }
	event_type = 33013;
    {
	struct rec_33013 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uint	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_33013 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33013);
    }
	event_type = 33014;
    {
	struct rec_33014 {
		ushort	bf1_s1:3;
		uchar	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_33014 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33014);
    }
	event_type = 33015;
    {
	struct rec_33015 {
		uchar	bf1_c1:3;
		ushort	:0;
		uchar	bf1_c2:3;
		uchar	bf1_c3:3;
	};
	struct rec_33015 v1 = {-1, -1, -1};
	v1.bf1_c2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33015);
    }
	event_type = 33016;
    {
	struct rec_33016 {
		ushort	bf1_s1:3;
		uchar	:0;
		uchar	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_33016 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33016);
    }
	event_type = 33017;
    {
	struct rec_33017 {
		ushort	bf1_s1:3;
		ushort	:0;
		uchar	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_33017 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33017);
    }
	event_type = 33018;
    {
	struct rec_33018 {
		ushort	bf1_s1:3;
		uchar	:0;
		ushort	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_33018 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33018);
    }
	event_type = 33019;
    {
	struct rec_33019 {
		uint	bf1_i:3;
		uchar	:0;
		uint	bf1_i2:3;
		ushort	:0;
		uint	bf1_i3:3;
	};
	struct rec_33019 v1 = {-1, -1, -1};
	v1.bf1_i2 = -1;	/* gcc bug 5157 workaround */
	v1.bf1_i3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33019);
    }
	event_type = 33020;
    {
	struct rec_33020 {
		uchar	bf1_c:8;
		uchar	:0;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
	};
	struct rec_33020 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33020);
    }
	event_type = 33021;
    {
	struct rec_33021 {
		ushort bf1_s1:4;
		uchar bf1_c1:4;
		uchar bf1_c2:4;
		uchar bf1_c3:4;
		uchar bf1_c4:4;
		uchar c1;
	};
	struct rec_33021 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33021);
    }
	event_type = 33121;
    {
	struct rec_33121 {
		ushort bf1_s1:4;
		uchar bf1_c1:4;
		uchar bf1_c2:4;
		uchar bf1_c3:4;
		uchar bf1_c4:4;
		uchar c1:8;
	};
	struct rec_33121 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33121);
    }
	event_type = 33022;
    {
	struct rec_33022 {
		uint	bf1_i:16;
		ushort	bf1_s:7;
		ushort	bf1_c:9;
	};
	struct rec_33022 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33022);
    }
	event_type = 33023;
    {
	struct rec_33023 {
		uchar	bf1_c:7;
		ushort	bf1_s:10;
		uint	bf1_i:23;
	};
	struct rec_33023 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33023);
    }
	event_type = 33024;
    {
	struct rec_33024 {
		uchar	bf1_c:7;
		ushort	bf1_s:10;
		uint	bf1_i:23;
		uchar	c;
		uchar	bf1_c2:7;
		ushort	bf1_s2:10;
		uint	bf1_i2:23;
	};
	struct rec_33024 v1 = {-1, -1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33024);
    }
	event_type = 33124;
    {
	struct rec_33124 {
		uchar	bf1_c:7;
		ushort	bf1_s:10;
		uint	bf1_i:23;
		uchar	c:8;
		uchar	bf1_c2:7;
		ushort	bf1_s2:10;
		uint	bf1_i2:23;
	};
	struct rec_33124 v1 = {-1, -1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33124);
    }
	event_type = 33025;
    {
	struct rec_33025 {
		uint	bf1_i:15;
		uchar	c1;
		uchar	c2;
	};
	struct rec_33025 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33025);
    }
	event_type = 33125;
    {
	struct rec_33125 {
		uint	bf1_i:15;
		uchar	c1:8;
		uchar	c2:8;
	};
	struct rec_33125 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33125);
    }
	event_type = 33026;
    {
	struct rec_33026 {
		uint	bf1_i:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c;
	};
	struct rec_33026 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33026);
    }
	event_type = 33126;
    {
	struct rec_33126 {
		uint	bf1_i:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c:8;
	};
	struct rec_33126 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33126);
    }
	event_type = 33027;
    {
	struct rec_33027 {
		ulonglong	bf1_ll:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c;
	};
	struct rec_33027 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33027);
    }
	event_type = 33127;
    {
	struct rec_33127 {
		ulonglong	bf1_ll:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c:8;
	};
	struct rec_33127 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33127);
    }
	event_type = 33028;
    {
	struct rec_33028 {
		ulonglong	bf1_ll:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c;
		ushort	s1;
	};
	struct rec_33028 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33028);
    }
	event_type = 33128;
    {
	struct rec_33128 {
		ulonglong	bf1_ll:2;
		ushort	bf1_s:2;
		uchar	bf1_c:2;
		uchar	c:8;
		ushort	s1:16;
	};
	struct rec_33128 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33128);
    }
	event_type = 33030;
    {
	struct rec_33030 {
		ulonglong	bf1_ll:2;
		uchar	c;
		ushort	s1;
		ulonglong	ll1;
		uint	i;
	};
	struct rec_33030 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33030);
    }
	event_type = 33130;
    {
	struct rec_33130 {
		ulonglong	bf1_ll:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:64;
		uint	i:32;
	};
	struct rec_33130 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33130);
    }
	event_type = 33031;
    {
	struct rec_33031 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		ulonglong	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_33031 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33031);
    }
	event_type = 33032;
    {
	struct rec_33032 {
		uxint	bf1_i:2;
	};
	struct rec_33032 v1 = {-1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33032);
    }
	event_type = 33033;
    {
	struct rec_33033 {
		uint :2;
	};
	struct rec_33033 v1;
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33033);
    }
	event_type = 33035;
    {
	struct rec_33035 {
		uchar bf1_c1:2;
		uint :2;
		uchar bf1_c2:2;
	};
	struct rec_33035 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33035);
    }
	event_type = 33036;
    {
	struct rec_33036 {
		ulonglong bf1_ll:40;
		ushort bf1_s:2;
	};
	struct rec_33036 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33036);
    }
	event_type = 33037;
    {
	struct rec_33037 {
		ulonglong bf1_ll:40;
		ushort s;
	};
	struct rec_33037 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33037);
    }
	event_type = 33137;
    {
	struct rec_33137 {
		ulonglong bf1_ll:40;
		ushort s:16;
	};
	struct rec_33137 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33137);
    }
	event_type = 33038;
    {
	struct rec_33038 {
		uchar c;
		uchar bf1_c1:3;
		ushort bf1_s1:3;
		uint bf1_i1:3;
		ushort s;
	};
	struct rec_33038 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33038);
    }
	event_type = 33138;
    {
	struct rec_33138 {
		uchar c:8;
		uchar bf1_c1:3;
		ushort bf1_s1:3;
		uint bf1_i1:3;
		ushort s:16;
	};
	struct rec_33138 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33138);
    }
	event_type = 33039;
    {
	struct rec_33039 {
		uchar c;
		uint bf1_i1:3;
		ushort bf1_s1:3;
		uchar bf1_c1:3;
		ushort s;
	};
	struct rec_33039 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33039);
    }
	event_type = 33139;
    {
	struct rec_33139 {
		uchar c:8;
		uint bf1_i1:3;
		ushort bf1_s1:3;
		uchar bf1_c1:3;
		ushort s:16;
	};
	struct rec_33139 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33139);
    }
	event_type = 33040;
    {
	struct rec_33040 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uchar	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
		uchar	c;
	};
	struct rec_33040 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33040);
    }
	event_type = 33140;
    {
	struct rec_33140 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uchar	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
		uchar	c:8;
	};
	struct rec_33140 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33140);
    }
	event_type = 33041;
    {
	struct rec_33041 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uint	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:2;
		uchar	c;
	};
	struct rec_33041 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33041);
    }
	event_type = 33141;
    {
	struct rec_33141 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uint	:0;
		ushort	bf1_s3:3;
		ushort	bf1_s4:3;
		ushort	bf1_s5:2;
		uchar	c:8;
	};
	struct rec_33141 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33141);
    }
	event_type = 33042;
    {
	struct rec_33042 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		uchar	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_33042 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33042);
    }
	event_type = 33043;
    {
	struct rec_33043 {
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		uchar	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_33043 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33043);
    }
	event_type = 33044;
    {
	struct rec_33044 {
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_33044 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33044);
    }
	event_type = 33045;
    {
	struct rec_33045 {
		uchar	c;
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_33045 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33045);
    }
	event_type = 33145;
    {
	struct rec_33145 {
		uchar	c:8;
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_33145 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33145);
    }
	event_type = 33046;
    {
	struct rec_33046 {
		uint	i;
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_33046 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33046);
    }
	event_type = 33146;
    {
	struct rec_33146 {
		uint	i:32;
		uint	bf1_s1:3;
		uint	bf1_s2:3;
		ulonglong	:0;
		uint	bf1_s3:3;
		uint	bf1_s4:3;
		uint	bf1_s5:3;
	};
	struct rec_33146 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33146);
    }
	event_type = 33047;
    {
	struct rec_33047 {
		uchar	bf1_c:7;
		ushort	bf1_s:9;
		uint	bf1_i:23;
	};
	struct rec_33047 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33047);
    }
	event_type = 33052;
    {
	struct rec_33052 {
		ushort	bf1_s1:3;
		ushort	bf1_s2:3;
		ushort	bf1_s3:3;
		uchar	:0;
		ushort	bf1_s4:3;
		ushort	bf1_s5:3;
	};
	struct rec_33052 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s4 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33052);
    }
	event_type = 33053;
    {
	struct rec_33053 {
		uchar	c;
		uchar	bf1_c1:4;
		ushort	:0;
		uint	bf1_i1:4;
	};
	struct rec_33053 v1 = {-1, -1, -1};
	v1.bf1_i1 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33053);
    }
	event_type = 33153;
    {
	struct rec_33153 {
		uchar	c:8;
		uchar	bf1_c1:4;
		ushort	:0;
		uint	bf1_i1:4;
	};
	struct rec_33153 v1 = {-1, -1, -1};
	v1.bf1_i1 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33153);
    }
	event_type = 33054;
    {
	struct rec_33054 {
		uchar	c[3]ARRAY_FMT;
		uint	bf1_i1:4;
	};
	struct rec_33054 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33054);
    }
	event_type = 33055;
    {
	struct rec_33055 {
		uchar	c[3]ARRAY_FMT;
		uint	bf1_i1:10;
	};
	struct rec_33055 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33055);
    }
	event_type = 33056;
    {
	struct rec_33056 {
		struct b1	b1;
		uint	bf1_i1:10;
	};
	struct rec_33056 v1 = {{-1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33056);
    }
	event_type = 33057;
    {
	struct rec_33057 {
		struct b1	b1[3]STRARRAY_FMT;
		uint	bf1_i1:4;
	};
	struct rec_33057 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33057);
    }
	event_type = 33058;
    {
	struct rec_33058 {
		struct b2	b1[3]STRARRAY_FMT;
		uint	bf1_i1:4;
	};
	struct rec_33058 v1 = {{{-1,-1}, {-1,-1}, {-1,-1}}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33058);
    }
	event_type = 33059;
    {
	struct rec_33059 {
		ulonglong bf1_ll:40;
		uint bf1_i:2;
	};
	struct rec_33059 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33059);
    }
	event_type = 33060;
    {
	struct rec_33060 {
		ulonglong bf1_ll:40;
		uint bf1_i:8;
	};
	struct rec_33060 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33060);
    }
	event_type = 33061;
    {
	struct rec_33061 {
		ulonglong bf1_ll:40;
		ushort x:15;
	};
	struct rec_33061 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33061);
    }
	event_type = 33062;
    {
	struct rec_33062 {
		ulonglong bf1_ll:32;
		uint :8;
		ushort bf1_s:2;
	};
	struct rec_33062 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33062);
    }
	event_type = 33063;
    {
	struct rec_33063 {
		ulonglong bf1_ll:32;
		uint :8;
		ushort s;
	};
	struct rec_33063 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33063);
    }
	event_type = 33163;
    {
	struct rec_33163 {
		ulonglong bf1_ll:32;
		uint :8;
		ushort s:16;
	};
	struct rec_33163 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33163);
    }
	event_type = 33064;
    {
	struct rec_33064 {
		ulonglong bf1_ll:32;
		uint :8;
		uint bf1_i:2;
	};
	struct rec_33064 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33064);
    }
	event_type = 33065;
    {
	struct rec_33065 {
		ulonglong bf1_ll:32;
		uint :8;
		ushort x:15;
	};
	struct rec_33065 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33065);
    }
	event_type = 33066;
    {
	struct rec_33066 {
		ulonglong bf1_ll:40;
		uint bf1_i:25;
	};
	struct rec_33066 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33066);
    }
	event_type = 33067;
    {
	struct rec_33067 {
		ulonglong	bf1_ll:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:2;
		uint	i:32;
	};
	struct rec_33067 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33067);
    }
	event_type = 33068;
    {
	struct rec_33068 {
		ulonglong	bf1_ll:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:33;
		uint	i:32;
	};
	struct rec_33068 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33068);
    }
	event_type = 33069;
    {
	struct rec_33069 {
		uchar	c:5;
		ushort	s:9;
		uint	i:17;
	};
	struct rec_33069 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33069);
    }
	event_type = 33070;
    {
	struct rec_33070 {
		ulonglong	bf1_ll:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:33;
		uint	i:7;
	};
	struct rec_33070 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33070);
    }
	event_type = 33071;
    {
	struct rec_33071 {
		uint		i:2;
		ulonglong	ll:33;
	};
	struct rec_33071 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33071);
    }
	event_type = 33072;
    {
	struct rec_33072 {
		ulonglong	bf1_ll:2;
		uchar	c:6;
		ushort	s1:8;
		ulonglong	ll1:33;
		uint	i:10;
	};
	struct rec_33072 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33072);
    }
	event_type = 33073;
    {
	struct rec_33073 {
		uint	i1:2;
		uchar	c:8;
		ushort	s1:16;
		ulonglong	ll1:33;
		uint	i:32;
	};
	struct rec_33073 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33073);
    }
	event_type = 33074;
    {
	struct rec_33074 {
		uchar	c1;
		uchar	c2;
		uchar	c3;
		ulonglong	ll1:33;
	};
	struct rec_33074 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33074);
    }
	event_type = 33075;
    {
	struct rec_33075 {
		uchar	c1;
		uchar	c2;
		uchar	c3;
		uchar	c4;
		ulonglong	ll1:33;
	};
	struct rec_33075 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33075);
    }
	event_type = 33076;
    {
	struct rec_33076 {
		uchar	c1;
		uchar	c2;
		uchar	c3;
		uchar	c4;
		uchar	c5;
		ulonglong	ll1:32;
	};
	struct rec_33076 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 33076);
    }

	exit(exitStatus);
}
