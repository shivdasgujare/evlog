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
	schar c;
};
struct b2 {
	schar c1:4;
	schar c2:4;
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

	event_type = 53001;
    {
	struct rec_53001 {
		int	bf1_i:2;
		short	bf1_s:2;
		schar	bf1_c:2;
	};
	struct rec_53001 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53001);
    }
	event_type = 53002;
    {
	struct rec_53002 {
		schar	bf1_c:2;
		short	bf1_s:2;
		int	bf1_i:2;
	};
	struct rec_53002 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53002);
    }
	event_type = 53003;
    {
	struct rec_53003 {
		schar	bf1_c:2;
		schar	bf1_c2:2;
	};
	struct rec_53003 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53003);
    }
	event_type = 53004;
    {
	struct rec_53004 {
		schar	bf1_c:2;
		schar	bf1_c2:2;
		schar	bf1_c3:2;
		schar	bf1_c4:2;
		schar	bf1_c5:2;
	};
	struct rec_53004 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53004);
    }
	event_type = 53005;
    {
	struct rec_53005 {
		int	bf1_i:2;
		int	bf1_i2:2;
		int	bf1_i3:2;
		int	bf1_i4:2;
		int	bf1_i5:2;
	};
	struct rec_53005 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53005);
    }
	event_type = 53006;
    {
	struct rec_53006 {
		short	bf1_s:2;
		schar	bf1_c2:2;
		schar	bf1_c3:2;
		schar	bf1_c4:2;
	};
	struct rec_53006 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53006);
    }
	event_type = 53007;
    {
	struct rec_53007 {
		schar	bf1_c2:2;
		schar	bf1_c3:2;
		schar	bf1_c4:2;
		short	bf1_s:2;
	};
	struct rec_53007 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53007);
    }
	event_type = 53008;
    {
	struct rec_53008 {
		schar	bf1_c:7;
		short	bf1_s:8;
		int	bf1_i:16;
	};
	struct rec_53008 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53008);
    }
	event_type = 53010;
    {
	struct rec_53010 {
		schar	bf1_c1:3;
		schar	bf1_c2:3;
		schar	bf1_c3:3;
		schar	bf1_c4:3;
		schar	bf1_c5:3;
	};
	struct rec_53010 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53010);
    }
	event_type = 53011;
    {
	struct rec_53011 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_53011 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53011);
    }
	event_type = 53012;
    {
	struct rec_53012 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		schar	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_53012 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53012);
    }
	event_type = 53013;
    {
	struct rec_53013 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_53013 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53013);
    }
	event_type = 53014;
    {
	struct rec_53014 {
		short	bf1_s1:3;
		schar	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_53014 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53014);
    }
	event_type = 53015;
    {
	struct rec_53015 {
		schar	bf1_c1:3;
		short	:0;
		schar	bf1_c2:3;
		schar	bf1_c3:3;
	};
	struct rec_53015 v1 = {-1, -1, -1};
	v1.bf1_c2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53015);
    }
	event_type = 53016;
    {
	struct rec_53016 {
		short	bf1_s1:3;
		schar	:0;
		schar	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_53016 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53016);
    }
	event_type = 53017;
    {
	struct rec_53017 {
		short	bf1_s1:3;
		short	:0;
		schar	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_53017 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53017);
    }
	event_type = 53018;
    {
	struct rec_53018 {
		short	bf1_s1:3;
		schar	:0;
		short	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_53018 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53018);
    }
	event_type = 53019;
    {
	struct rec_53019 {
		int	bf1_i:3;
		schar	:0;
		int	bf1_i2:3;
		short	:0;
		int	bf1_i3:3;
	};
	struct rec_53019 v1 = {-1, -1, -1};
	v1.bf1_i2 = -1;	/* gcc bug 5157 workaround */
	v1.bf1_i3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53019);
    }
	event_type = 53020;
    {
	struct rec_53020 {
		schar	bf1_c:8;
		schar	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_53020 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53020);
    }
	event_type = 53021;
    {
	struct rec_53021 {
		short bf1_s1:4;
		schar bf1_c1:4;
		schar bf1_c2:4;
		schar bf1_c3:4;
		schar bf1_c4:4;
		schar c1;
	};
	struct rec_53021 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53021);
    }
	event_type = 53121;
    {
	struct rec_53121 {
		short bf1_s1:4;
		schar bf1_c1:4;
		schar bf1_c2:4;
		schar bf1_c3:4;
		schar bf1_c4:4;
		schar c1:8;
	};
	struct rec_53121 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53121);
    }
	event_type = 53022;
    {
	struct rec_53022 {
		int	bf1_i:16;
		short	bf1_s:7;
		short	bf1_c:9;
	};
	struct rec_53022 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53022);
    }
	event_type = 53023;
    {
	struct rec_53023 {
		schar	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
	};
	struct rec_53023 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53023);
    }
	event_type = 53024;
    {
	struct rec_53024 {
		schar	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
		schar	c;
		schar	bf1_c2:7;
		short	bf1_s2:10;
		int	bf1_i2:23;
	};
	struct rec_53024 v1 = {-1, -1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53024);
    }
	event_type = 53124;
    {
	struct rec_53124 {
		schar	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
		schar	c:8;
		schar	bf1_c2:7;
		short	bf1_s2:10;
		int	bf1_i2:23;
	};
	struct rec_53124 v1 = {-1, -1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53124);
    }
	event_type = 53025;
    {
	struct rec_53025 {
		int	bf1_i:15;
		schar	c1;
		schar	c2;
	};
	struct rec_53025 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53025);
    }
	event_type = 53125;
    {
	struct rec_53125 {
		int	bf1_i:15;
		schar	c1:8;
		schar	c2:8;
	};
	struct rec_53125 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53125);
    }
	event_type = 53026;
    {
	struct rec_53026 {
		int	bf1_i:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c;
	};
	struct rec_53026 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53026);
    }
	event_type = 53126;
    {
	struct rec_53126 {
		int	bf1_i:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c:8;
	};
	struct rec_53126 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53126);
    }
	event_type = 53027;
    {
	struct rec_53027 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c;
	};
	struct rec_53027 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53027);
    }
	event_type = 53127;
    {
	struct rec_53127 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c:8;
	};
	struct rec_53127 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53127);
    }
	event_type = 53028;
    {
	struct rec_53028 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c;
		short	s1;
	};
	struct rec_53028 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53028);
    }
	event_type = 53128;
    {
	struct rec_53128 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c:8;
		short	s1:16;
	};
	struct rec_53128 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53128);
    }
	event_type = 53029;
    {
	struct rec_53029 {
		ulonglong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c;
		short	s1;
	};
	struct rec_53029 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53029);
    }
	event_type = 53129;
    {
	struct rec_53129 {
		ulonglong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c:8;
		short	s1:16;
	};
	struct rec_53129 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53129);
    }
	event_type = 53030;
    {
	struct rec_53030 {
		longlong	bf1_ll:2;
		schar	c;
		short	s1;
		longlong	ll1;
		int	i;
	};
	struct rec_53030 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53030);
    }
	event_type = 53130;
    {
	struct rec_53130 {
		longlong	bf1_ll:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:64;
		int	i:32;
	};
	struct rec_53130 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53130);
    }
	event_type = 53031;
    {
	struct rec_53031 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		longlong	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_53031 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53031);
    }
	event_type = 53032;
    {
	struct rec_53032 {
		xint	bf1_i:2;
	};
	struct rec_53032 v1 = {-1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53032);
    }
	event_type = 53033;
    {
	struct rec_53033 {
		int :2;
	};
	struct rec_53033 v1;
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53033);
    }
	event_type = 53035;
    {
	struct rec_53035 {
		schar bf1_c1:2;
		int :2;
		schar bf1_c2:2;
	};
	struct rec_53035 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53035);
    }
	event_type = 53036;
    {
	struct rec_53036 {
		longlong bf1_ll:40;
		short bf1_s:2;
	};
	struct rec_53036 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53036);
    }
	event_type = 53037;
    {
	struct rec_53037 {
		longlong bf1_ll:40;
		short s;
	};
	struct rec_53037 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53037);
    }
	event_type = 53137;
    {
	struct rec_53137 {
		longlong bf1_ll:40;
		short s:16;
	};
	struct rec_53137 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53137);
    }
	event_type = 53038;
    {
	struct rec_53038 {
		schar c;
		schar bf1_c1:3;
		short bf1_s1:3;
		int bf1_i1:3;
		short s;
	};
	struct rec_53038 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53038);
    }
	event_type = 53138;
    {
	struct rec_53138 {
		schar c:8;
		schar bf1_c1:3;
		short bf1_s1:3;
		int bf1_i1:3;
		short s:16;
	};
	struct rec_53138 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53138);
    }
	event_type = 53039;
    {
	struct rec_53039 {
		schar c;
		int bf1_i1:3;
		short bf1_s1:3;
		schar bf1_c1:3;
		short s;
	};
	struct rec_53039 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53039);
    }
	event_type = 53139;
    {
	struct rec_53139 {
		schar c:8;
		int bf1_i1:3;
		short bf1_s1:3;
		schar bf1_c1:3;
		short s:16;
	};
	struct rec_53139 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53139);
    }
	event_type = 53040;
    {
	struct rec_53040 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		schar	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
		schar	c;
	};
	struct rec_53040 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53040);
    }
	event_type = 53140;
    {
	struct rec_53140 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		schar	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
		schar	c:8;
	};
	struct rec_53140 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53140);
    }
	event_type = 53041;
    {
	struct rec_53041 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:2;
		schar	c;
	};
	struct rec_53041 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53041);
    }
	event_type = 53141;
    {
	struct rec_53141 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:2;
		schar	c:8;
	};
	struct rec_53141 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53141);
    }
	event_type = 53042;
    {
	struct rec_53042 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		schar	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_53042 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53042);
    }
	event_type = 53043;
    {
	struct rec_53043 {
		int	bf1_s1:3;
		int	bf1_s2:3;
		schar	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_53043 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53043);
    }
	event_type = 53044;
    {
	struct rec_53044 {
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_53044 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53044);
    }
	event_type = 53045;
    {
	struct rec_53045 {
		schar	c;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_53045 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53045);
    }
	event_type = 53145;
    {
	struct rec_53145 {
		schar	c:8;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_53145 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53145);
    }
	event_type = 53046;
    {
	struct rec_53046 {
		int	i;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_53046 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53046);
    }
	event_type = 53146;
    {
	struct rec_53146 {
		int	i:32;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_53146 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53146);
    }
	event_type = 53047;
    {
	struct rec_53047 {
		schar	bf1_c:7;
		short	bf1_s:9;
		int	bf1_i:23;
	};
	struct rec_53047 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53047);
    }
	event_type = 53052;
    {
	struct rec_53052 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		short	bf1_s3:3;
		schar	:0;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_53052 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s4 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53052);
    }
	event_type = 53053;
    {
	struct rec_53053 {
		schar	c;
		schar	bf1_c1:4;
		short	:0;
		int	bf1_i1:4;
	};
	struct rec_53053 v1 = {-1, -1, -1};
	v1.bf1_i1 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53053);
    }
	event_type = 53153;
    {
	struct rec_53153 {
		schar	c:8;
		schar	bf1_c1:4;
		short	:0;
		int	bf1_i1:4;
	};
	struct rec_53153 v1 = {-1, -1, -1};
	v1.bf1_i1 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53153);
    }
	event_type = 53054;
    {
	struct rec_53054 {
		schar	c[3]ARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_53054 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53054);
    }
	event_type = 53055;
    {
	struct rec_53055 {
		schar	c[3]ARRAY_FMT;
		int	bf1_i1:10;
	};
	struct rec_53055 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53055);
    }
	event_type = 53056;
    {
	struct rec_53056 {
		struct b1	b1;
		int	bf1_i1:10;
	};
	struct rec_53056 v1 = {{-1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53056);
    }
	event_type = 53057;
    {
	struct rec_53057 {
		struct b1	b1[3]STRARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_53057 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53057);
    }
	event_type = 53058;
    {
	struct rec_53058 {
		struct b2	b1[3]STRARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_53058 v1 = {{{-1,-1}, {-1,-1}, {-1,-1}}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53058);
    }
	event_type = 53059;
    {
	struct rec_53059 {
		longlong bf1_ll:40;
		int bf1_i:2;
	};
	struct rec_53059 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53059);
    }
	event_type = 53060;
    {
	struct rec_53060 {
		longlong bf1_ll:40;
		int bf1_i:8;
	};
	struct rec_53060 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53060);
    }
	event_type = 53061;
    {
	struct rec_53061 {
		longlong bf1_ll:40;
		short x:15;
	};
	struct rec_53061 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53061);
    }
	event_type = 53062;
    {
	struct rec_53062 {
		longlong bf1_ll:32;
		int :8;
		short bf1_s:2;
	};
	struct rec_53062 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53062);
    }
	event_type = 53063;
    {
	struct rec_53063 {
		longlong bf1_ll:32;
		int :8;
		short s;
	};
	struct rec_53063 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53063);
    }
	event_type = 53163;
    {
	struct rec_53163 {
		longlong bf1_ll:32;
		int :8;
		short s:16;
	};
	struct rec_53163 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53163);
    }
	event_type = 53064;
    {
	struct rec_53064 {
		longlong bf1_ll:32;
		int :8;
		int bf1_i:2;
	};
	struct rec_53064 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53064);
    }
	event_type = 53065;
    {
	struct rec_53065 {
		longlong bf1_ll:32;
		int :8;
		short x:15;
	};
	struct rec_53065 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53065);
    }
	event_type = 53066;
    {
	struct rec_53066 {
		longlong bf1_ll:40;
		int bf1_i:25;
	};
	struct rec_53066 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53066);
    }
	event_type = 53067;
    {
	struct rec_53067 {
		longlong	bf1_ll:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:2;
		int	i:32;
	};
	struct rec_53067 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53067);
    }
	event_type = 53068;
    {
	struct rec_53068 {
		longlong	bf1_ll:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:32;
	};
	struct rec_53068 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53068);
    }
	event_type = 53069;
    {
	struct rec_53069 {
		schar	c:5;
		short	s:9;
		int	i:17;
	};
	struct rec_53069 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53069);
    }
	event_type = 53070;
    {
	struct rec_53070 {
		longlong	bf1_ll:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:7;
	};
	struct rec_53070 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53070);
    }
	event_type = 53071;
    {
	struct rec_53071 {
		int		i:2;
		longlong	ll:33;
	};
	struct rec_53071 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53071);
    }
	event_type = 53072;
    {
	struct rec_53072 {
		longlong	bf1_ll:2;
		schar	c:6;
		short	s1:8;
		longlong	ll1:33;
		int	i:10;
	};
	struct rec_53072 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53072);
    }
	event_type = 53073;
    {
	struct rec_53073 {
		int	i1:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:32;
	};
	struct rec_53073 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53073);
    }
	event_type = 53074;
    {
	struct rec_53074 {
		schar	c1;
		schar	c2;
		schar	c3;
		longlong	ll1:33;
	};
	struct rec_53074 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53074);
    }
	event_type = 53075;
    {
	struct rec_53075 {
		schar	c1;
		schar	c2;
		schar	c3;
		schar	c4;
		longlong	ll1:33;
	};
	struct rec_53075 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53075);
    }
	event_type = 53076;
    {
	struct rec_53076 {
		schar	c1;
		schar	c2;
		schar	c3;
		schar	c4;
		schar	c5;
		longlong	ll1:32;
	};
	struct rec_53076 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 53076);
    }

	exit(exitStatus);
}
