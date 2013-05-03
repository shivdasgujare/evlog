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

	event_type = 43001;
    {
	struct rec_43001 {
		int	bf1_i:2;
		short	bf1_s:2;
		schar	bf1_c:2;
	};
	struct rec_43001 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43001);
    }
	event_type = 43002;
    {
	struct rec_43002 {
		schar	bf1_c:2;
		short	bf1_s:2;
		int	bf1_i:2;
	};
	struct rec_43002 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43002);
    }
	event_type = 43003;
    {
	struct rec_43003 {
		schar	bf1_c:2;
		schar	bf1_c2:2;
	};
	struct rec_43003 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43003);
    }
	event_type = 43004;
    {
	struct rec_43004 {
		schar	bf1_c:2;
		schar	bf1_c2:2;
		schar	bf1_c3:2;
		schar	bf1_c4:2;
		schar	bf1_c5:2;
	};
	struct rec_43004 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43004);
    }
	event_type = 43005;
    {
	struct rec_43005 {
		int	bf1_i:2;
		int	bf1_i2:2;
		int	bf1_i3:2;
		int	bf1_i4:2;
		int	bf1_i5:2;
	};
	struct rec_43005 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43005);
    }
	event_type = 43006;
    {
	struct rec_43006 {
		short	bf1_s:2;
		schar	bf1_c2:2;
		schar	bf1_c3:2;
		schar	bf1_c4:2;
	};
	struct rec_43006 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43006);
    }
	event_type = 43007;
    {
	struct rec_43007 {
		schar	bf1_c2:2;
		schar	bf1_c3:2;
		schar	bf1_c4:2;
		short	bf1_s:2;
	};
	struct rec_43007 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43007);
    }
	event_type = 43008;
    {
	struct rec_43008 {
		schar	bf1_c:7;
		short	bf1_s:8;
		int	bf1_i:16;
	};
	struct rec_43008 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43008);
    }
	event_type = 43010;
    {
	struct rec_43010 {
		schar	bf1_c1:3;
		schar	bf1_c2:3;
		schar	bf1_c3:3;
		schar	bf1_c4:3;
		schar	bf1_c5:3;
	};
	struct rec_43010 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43010);
    }
	event_type = 43011;
    {
	struct rec_43011 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_43011 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43011);
    }
	event_type = 43012;
    {
	struct rec_43012 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		schar	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_43012 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43012);
    }
	event_type = 43013;
    {
	struct rec_43013 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_43013 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43013);
    }
	event_type = 43014;
    {
	struct rec_43014 {
		short	bf1_s1:3;
		schar	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_43014 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43014);
    }
	event_type = 43015;
    {
	struct rec_43015 {
		schar	bf1_c1:3;
		short	:0;
		schar	bf1_c2:3;
		schar	bf1_c3:3;
	};
	struct rec_43015 v1 = {1, 1, 1};
	v1.bf1_c2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43015);
    }
	event_type = 43016;
    {
	struct rec_43016 {
		short	bf1_s1:3;
		schar	:0;
		schar	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_43016 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43016);
    }
	event_type = 43017;
    {
	struct rec_43017 {
		short	bf1_s1:3;
		short	:0;
		schar	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_43017 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43017);
    }
	event_type = 43018;
    {
	struct rec_43018 {
		short	bf1_s1:3;
		schar	:0;
		short	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_43018 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43018);
    }
	event_type = 43019;
    {
	struct rec_43019 {
		int	bf1_i:3;
		schar	:0;
		int	bf1_i2:3;
		short	:0;
		int	bf1_i3:3;
	};
	struct rec_43019 v1 = {1, 1, 1};
	v1.bf1_i2 = 1;	/* gcc bug 5157 workaround */
	v1.bf1_i3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43019);
    }
	event_type = 43020;
    {
	struct rec_43020 {
		schar	bf1_c:8;
		schar	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_43020 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43020);
    }
	event_type = 43021;
    {
	struct rec_43021 {
		short bf1_s1:4;
		schar bf1_c1:4;
		schar bf1_c2:4;
		schar bf1_c3:4;
		schar bf1_c4:4;
		schar c1;
	};
	struct rec_43021 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43021);
    }
	event_type = 43121;
    {
	struct rec_43121 {
		short bf1_s1:4;
		schar bf1_c1:4;
		schar bf1_c2:4;
		schar bf1_c3:4;
		schar bf1_c4:4;
		schar c1:8;
	};
	struct rec_43121 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43121);
    }
	event_type = 43022;
    {
	struct rec_43022 {
		int	bf1_i:16;
		short	bf1_s:7;
		short	bf1_c:9;
	};
	struct rec_43022 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43022);
    }
	event_type = 43023;
    {
	struct rec_43023 {
		schar	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
	};
	struct rec_43023 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43023);
    }
	event_type = 43024;
    {
	struct rec_43024 {
		schar	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
		schar	c;
		schar	bf1_c2:7;
		short	bf1_s2:10;
		int	bf1_i2:23;
	};
	struct rec_43024 v1 = {1, 1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43024);
    }
	event_type = 43124;
    {
	struct rec_43124 {
		schar	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
		schar	c:8;
		schar	bf1_c2:7;
		short	bf1_s2:10;
		int	bf1_i2:23;
	};
	struct rec_43124 v1 = {1, 1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43124);
    }
	event_type = 43025;
    {
	struct rec_43025 {
		int	bf1_i:15;
		schar	c1;
		schar	c2;
	};
	struct rec_43025 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43025);
    }
	event_type = 43125;
    {
	struct rec_43125 {
		int	bf1_i:15;
		schar	c1:8;
		schar	c2:8;
	};
	struct rec_43125 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43125);
    }
	event_type = 43026;
    {
	struct rec_43026 {
		int	bf1_i:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c;
	};
	struct rec_43026 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43026);
    }
	event_type = 43126;
    {
	struct rec_43126 {
		int	bf1_i:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c:8;
	};
	struct rec_43126 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43126);
    }
	event_type = 43027;
    {
	struct rec_43027 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c;
	};
	struct rec_43027 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43027);
    }
	event_type = 43127;
    {
	struct rec_43127 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c:8;
	};
	struct rec_43127 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43127);
    }
	event_type = 43028;
    {
	struct rec_43028 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c;
		short	s1;
	};
	struct rec_43028 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43028);
    }
	event_type = 43128;
    {
	struct rec_43128 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c:8;
		short	s1:16;
	};
	struct rec_43128 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43128);
    }
	event_type = 43029;
    {
	struct rec_43029 {
		ulonglong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c;
		short	s1;
	};
	struct rec_43029 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43029);
    }
	event_type = 43129;
    {
	struct rec_43129 {
		ulonglong	bf1_ll:2;
		short	bf1_s:2;
		schar	bf1_c:2;
		schar	c:8;
		short	s1:16;
	};
	struct rec_43129 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43129);
    }
	event_type = 43030;
    {
	struct rec_43030 {
		longlong	bf1_ll:2;
		schar	c;
		short	s1;
		longlong	ll1;
		int	i;
	};
	struct rec_43030 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43030);
    }
	event_type = 43130;
    {
	struct rec_43130 {
		longlong	bf1_ll:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:64;
		int	i:32;
	};
	struct rec_43130 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43130);
    }
	event_type = 43031;
    {
	struct rec_43031 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		longlong	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_43031 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43031);
    }
	event_type = 43032;
    {
	struct rec_43032 {
		xint	bf1_i:2;
	};
	struct rec_43032 v1 = {1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43032);
    }
	event_type = 43033;
    {
	struct rec_43033 {
		int :2;
	};
	struct rec_43033 v1;
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43033);
    }
	event_type = 43035;
    {
	struct rec_43035 {
		schar bf1_c1:2;
		int :2;
		schar bf1_c2:2;
	};
	struct rec_43035 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43035);
    }
	event_type = 43036;
    {
	struct rec_43036 {
		longlong bf1_ll:40;
		short bf1_s:2;
	};
	struct rec_43036 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43036);
    }
	event_type = 43037;
    {
	struct rec_43037 {
		longlong bf1_ll:40;
		short s;
	};
	struct rec_43037 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43037);
    }
	event_type = 43137;
    {
	struct rec_43137 {
		longlong bf1_ll:40;
		short s:16;
	};
	struct rec_43137 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43137);
    }
	event_type = 43038;
    {
	struct rec_43038 {
		schar c;
		schar bf1_c1:3;
		short bf1_s1:3;
		int bf1_i1:3;
		short s;
	};
	struct rec_43038 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43038);
    }
	event_type = 43138;
    {
	struct rec_43138 {
		schar c:8;
		schar bf1_c1:3;
		short bf1_s1:3;
		int bf1_i1:3;
		short s:16;
	};
	struct rec_43138 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43138);
    }
	event_type = 43039;
    {
	struct rec_43039 {
		schar c;
		int bf1_i1:3;
		short bf1_s1:3;
		schar bf1_c1:3;
		short s;
	};
	struct rec_43039 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43039);
    }
	event_type = 43139;
    {
	struct rec_43139 {
		schar c:8;
		int bf1_i1:3;
		short bf1_s1:3;
		schar bf1_c1:3;
		short s:16;
	};
	struct rec_43139 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43139);
    }
	event_type = 43040;
    {
	struct rec_43040 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		schar	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
		schar	c;
	};
	struct rec_43040 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43040);
    }
	event_type = 43140;
    {
	struct rec_43140 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		schar	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
		schar	c:8;
	};
	struct rec_43140 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43140);
    }
	event_type = 43041;
    {
	struct rec_43041 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:2;
		schar	c;
	};
	struct rec_43041 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43041);
    }
	event_type = 43141;
    {
	struct rec_43141 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:2;
		schar	c:8;
	};
	struct rec_43141 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43141);
    }
	event_type = 43042;
    {
	struct rec_43042 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		schar	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_43042 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43042);
    }
	event_type = 43043;
    {
	struct rec_43043 {
		int	bf1_s1:3;
		int	bf1_s2:3;
		schar	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_43043 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43043);
    }
	event_type = 43044;
    {
	struct rec_43044 {
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_43044 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43044);
    }
	event_type = 43045;
    {
	struct rec_43045 {
		schar	c;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_43045 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43045);
    }
	event_type = 43145;
    {
	struct rec_43145 {
		schar	c:8;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_43145 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43145);
    }
	event_type = 43046;
    {
	struct rec_43046 {
		int	i;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_43046 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43046);
    }
	event_type = 43146;
    {
	struct rec_43146 {
		int	i:32;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_43146 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43146);
    }
	event_type = 43047;
    {
	struct rec_43047 {
		schar	bf1_c:7;
		short	bf1_s:9;
		int	bf1_i:23;
	};
	struct rec_43047 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43047);
    }
	event_type = 43052;
    {
	struct rec_43052 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		short	bf1_s3:3;
		schar	:0;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_43052 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s4 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43052);
    }
	event_type = 43053;
    {
	struct rec_43053 {
		schar	c;
		schar	bf1_c1:4;
		short	:0;
		int	bf1_i1:4;
	};
	struct rec_43053 v1 = {1, 1, 1};
	v1.bf1_i1 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43053);
    }
	event_type = 43153;
    {
	struct rec_43153 {
		schar	c:8;
		schar	bf1_c1:4;
		short	:0;
		int	bf1_i1:4;
	};
	struct rec_43153 v1 = {1, 1, 1};
	v1.bf1_i1 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43153);
    }
	event_type = 43054;
    {
	struct rec_43054 {
		schar	c[3]ARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_43054 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43054);
    }
	event_type = 43055;
    {
	struct rec_43055 {
		schar	c[3]ARRAY_FMT;
		int	bf1_i1:10;
	};
	struct rec_43055 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43055);
    }
	event_type = 43056;
    {
	struct rec_43056 {
		struct b1	b1;
		int	bf1_i1:10;
	};
	struct rec_43056 v1 = {{1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43056);
    }
	event_type = 43057;
    {
	struct rec_43057 {
		struct b1	b1[3]STRARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_43057 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43057);
    }
	event_type = 43058;
    {
	struct rec_43058 {
		struct b2	b1[3]STRARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_43058 v1 = {{{1,1}, {1,1}, {1,1}}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43058);
    }
	event_type = 43059;
    {
	struct rec_43059 {
		longlong bf1_ll:40;
		int bf1_i:2;
	};
	struct rec_43059 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43059);
    }
	event_type = 43060;
    {
	struct rec_43060 {
		longlong bf1_ll:40;
		int bf1_i:8;
	};
	struct rec_43060 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43060);
    }
	event_type = 43061;
    {
	struct rec_43061 {
		longlong bf1_ll:40;
		short x:15;
	};
	struct rec_43061 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43061);
    }
	event_type = 43062;
    {
	struct rec_43062 {
		longlong bf1_ll:32;
		int :8;
		short bf1_s:2;
	};
	struct rec_43062 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43062);
    }
	event_type = 43063;
    {
	struct rec_43063 {
		longlong bf1_ll:32;
		int :8;
		short s;
	};
	struct rec_43063 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43063);
    }
	event_type = 43163;
    {
	struct rec_43163 {
		longlong bf1_ll:32;
		int :8;
		short s:16;
	};
	struct rec_43163 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43163);
    }
	event_type = 43064;
    {
	struct rec_43064 {
		longlong bf1_ll:32;
		int :8;
		int bf1_i:2;
	};
	struct rec_43064 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43064);
    }
	event_type = 43065;
    {
	struct rec_43065 {
		longlong bf1_ll:32;
		int :8;
		short x:15;
	};
	struct rec_43065 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43065);
    }
	event_type = 43066;
    {
	struct rec_43066 {
		longlong bf1_ll:40;
		int bf1_i:25;
	};
	struct rec_43066 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43066);
    }
	event_type = 43067;
    {
	struct rec_43067 {
		longlong	bf1_ll:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:2;
		int	i:32;
	};
	struct rec_43067 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43067);
    }
	event_type = 43068;
    {
	struct rec_43068 {
		longlong	bf1_ll:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:32;
	};
	struct rec_43068 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43068);
    }
	event_type = 43069;
    {
	struct rec_43069 {
		schar	c:5;
		short	s:9;
		int	i:17;
	};
	struct rec_43069 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43069);
    }
	event_type = 43070;
    {
	struct rec_43070 {
		longlong	bf1_ll:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:7;
	};
	struct rec_43070 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43070);
    }
	event_type = 43071;
    {
	struct rec_43071 {
		int		i:2;
		longlong	ll:33;
	};
	struct rec_43071 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43071);
    }
	event_type = 43072;
    {
	struct rec_43072 {
		longlong	bf1_ll:2;
		schar	c:6;
		short	s1:8;
		longlong	ll1:33;
		int	i:10;
	};
	struct rec_43072 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43072);
    }
	event_type = 43073;
    {
	struct rec_43073 {
		int	i1:2;
		schar	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:32;
	};
	struct rec_43073 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43073);
    }
	event_type = 43074;
    {
	struct rec_43074 {
		schar	c1;
		schar	c2;
		schar	c3;
		longlong	ll1:33;
	};
	struct rec_43074 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43074);
    }
	event_type = 43075;
    {
	struct rec_43075 {
		schar	c1;
		schar	c2;
		schar	c3;
		schar	c4;
		longlong	ll1:33;
	};
	struct rec_43075 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43075);
    }
	event_type = 43076;
    {
	struct rec_43076 {
		schar	c1;
		schar	c2;
		schar	c3;
		schar	c4;
		schar	c5;
		longlong	ll1:32;
	};
	struct rec_43076 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 43076);
    }

	exit(exitStatus);
}
