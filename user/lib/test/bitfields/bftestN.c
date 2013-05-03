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

	event_type = 13001;
    {
	struct rec_13001 {
		int	bf1_i:2;
		short	bf1_s:2;
		char	bf1_c:2;
	};
	struct rec_13001 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13001);
    }
	event_type = 13002;
    {
	struct rec_13002 {
		char	bf1_c:2;
		short	bf1_s:2;
		int	bf1_i:2;
	};
	struct rec_13002 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13002);
    }
	event_type = 13003;
    {
	struct rec_13003 {
		char	bf1_c:2;
		char	bf1_c2:2;
	};
	struct rec_13003 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13003);
    }
	event_type = 13004;
    {
	struct rec_13004 {
		char	bf1_c:2;
		char	bf1_c2:2;
		char	bf1_c3:2;
		char	bf1_c4:2;
		char	bf1_c5:2;
	};
	struct rec_13004 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13004);
    }
	event_type = 13005;
    {
	struct rec_13005 {
		int	bf1_i:2;
		int	bf1_i2:2;
		int	bf1_i3:2;
		int	bf1_i4:2;
		int	bf1_i5:2;
	};
	struct rec_13005 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13005);
    }
	event_type = 13006;
    {
	struct rec_13006 {
		short	bf1_s:2;
		char	bf1_c2:2;
		char	bf1_c3:2;
		char	bf1_c4:2;
	};
	struct rec_13006 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13006);
    }
	event_type = 13007;
    {
	struct rec_13007 {
		char	bf1_c2:2;
		char	bf1_c3:2;
		char	bf1_c4:2;
		short	bf1_s:2;
	};
	struct rec_13007 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13007);
    }
	event_type = 13008;
    {
	struct rec_13008 {
		char	bf1_c:7;
		short	bf1_s:8;
		int	bf1_i:16;
	};
	struct rec_13008 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13008);
    }
	event_type = 13010;
    {
	struct rec_13010 {
		char	bf1_c1:3;
		char	bf1_c2:3;
		char	bf1_c3:3;
		char	bf1_c4:3;
		char	bf1_c5:3;
	};
	struct rec_13010 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13010);
    }
	event_type = 13011;
    {
	struct rec_13011 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_13011 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13011);
    }
	event_type = 13012;
    {
	struct rec_13012 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		char	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_13012 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13012);
    }
	event_type = 13013;
    {
	struct rec_13013 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_13013 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13013);
    }
	event_type = 13014;
    {
	struct rec_13014 {
		short	bf1_s1:3;
		char	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_13014 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13014);
    }
	event_type = 13015;
    {
	struct rec_13015 {
		char	bf1_c1:3;
		short	:0;
		char	bf1_c2:3;
		char	bf1_c3:3;
	};
	struct rec_13015 v1 = {-1, -1, -1};
	v1.bf1_c2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13015);
    }
	event_type = 13016;
    {
	struct rec_13016 {
		short	bf1_s1:3;
		char	:0;
		char	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_13016 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13016);
    }
	event_type = 13017;
    {
	struct rec_13017 {
		short	bf1_s1:3;
		short	:0;
		char	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_13017 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13017);
    }
	event_type = 13018;
    {
	struct rec_13018 {
		short	bf1_s1:3;
		char	:0;
		short	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_13018 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13018);
    }
	event_type = 13019;
    {
	struct rec_13019 {
		int	bf1_i:3;
		char	:0;
		int	bf1_i2:3;
		short	:0;
		int	bf1_i3:3;
	};
	struct rec_13019 v1 = {-1, -1, -1};
	v1.bf1_i2 = -1;	/* gcc bug 5157 workaround */
	v1.bf1_i3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13019);
    }
	event_type = 13020;
    {
	struct rec_13020 {
		char	bf1_c:8;
		char	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_13020 v1 = {-1, -1, -1};
	v1.bf1_s2 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13020);
    }
	event_type = 13021;
    {
	struct rec_13021 {
		short bf1_s1:4;
		char bf1_c1:4;
		char bf1_c2:4;
		char bf1_c3:4;
		char bf1_c4:4;
		char c1;
	};
	struct rec_13021 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13021);
    }
	event_type = 13121;
    {
	struct rec_13121 {
		short bf1_s1:4;
		char bf1_c1:4;
		char bf1_c2:4;
		char bf1_c3:4;
		char bf1_c4:4;
		char c1:8;
	};
	struct rec_13121 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13121);
    }
	event_type = 13022;
    {
	struct rec_13022 {
		int	bf1_i:16;
		short	bf1_s:7;
		short	bf1_c:9;
	};
	struct rec_13022 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13022);
    }
	event_type = 13023;
    {
	struct rec_13023 {
		char	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
	};
	struct rec_13023 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13023);
    }
	event_type = 13024;
    {
	struct rec_13024 {
		char	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
		char	c;
		char	bf1_c2:7;
		short	bf1_s2:10;
		int	bf1_i2:23;
	};
	struct rec_13024 v1 = {-1, -1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13024);
    }
	event_type = 13124;
    {
	struct rec_13124 {
		char	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
		char	c:8;
		char	bf1_c2:7;
		short	bf1_s2:10;
		int	bf1_i2:23;
	};
	struct rec_13124 v1 = {-1, -1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13124);
    }
	event_type = 13025;
    {
	struct rec_13025 {
		int	bf1_i:15;
		char	c1;
		char	c2;
	};
	struct rec_13025 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13025);
    }
	event_type = 13125;
    {
	struct rec_13125 {
		int	bf1_i:15;
		char	c1:8;
		char	c2:8;
	};
	struct rec_13125 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13125);
    }
	event_type = 13026;
    {
	struct rec_13026 {
		int	bf1_i:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c;
	};
	struct rec_13026 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13026);
    }
	event_type = 13126;
    {
	struct rec_13126 {
		int	bf1_i:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c:8;
	};
	struct rec_13126 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13126);
    }
	event_type = 13027;
    {
	struct rec_13027 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c;
	};
	struct rec_13027 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13027);
    }
	event_type = 13127;
    {
	struct rec_13127 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c:8;
	};
	struct rec_13127 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13127);
    }
	event_type = 13028;
    {
	struct rec_13028 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c;
		short	s1;
	};
	struct rec_13028 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13028);
    }
	event_type = 13128;
    {
	struct rec_13128 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c:8;
		short	s1:16;
	};
	struct rec_13128 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13128);
    }
	event_type = 13029;
    {
	struct rec_13029 {
		ulonglong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c;
		short	s1;
	};
	struct rec_13029 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13029);
    }
	event_type = 13129;
    {
	struct rec_13129 {
		ulonglong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c:8;
		short	s1:16;
	};
	struct rec_13129 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13129);
    }
	event_type = 13030;
    {
	struct rec_13030 {
		longlong	bf1_ll:2;
		char	c;
		short	s1;
		longlong	ll1;
		int	i;
	};
	struct rec_13030 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13030);
    }
	event_type = 13130;
    {
	struct rec_13130 {
		longlong	bf1_ll:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:64;
		int	i:32;
	};
	struct rec_13130 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13130);
    }
	event_type = 13031;
    {
	struct rec_13031 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		longlong	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_13031 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13031);
    }
	event_type = 13032;
    {
	struct rec_13032 {
		xint	bf1_i:2;
	};
	struct rec_13032 v1 = {-1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13032);
    }
	event_type = 13033;
    {
	struct rec_13033 {
		int :2;
	};
	struct rec_13033 v1;
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13033);
    }
	event_type = 13035;
    {
	struct rec_13035 {
		char bf1_c1:2;
		int :2;
		char bf1_c2:2;
	};
	struct rec_13035 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13035);
    }
	event_type = 13036;
    {
	struct rec_13036 {
		longlong bf1_ll:40;
		short bf1_s:2;
	};
	struct rec_13036 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13036);
    }
	event_type = 13037;
    {
	struct rec_13037 {
		longlong bf1_ll:40;
		short s;
	};
	struct rec_13037 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13037);
    }
	event_type = 13137;
    {
	struct rec_13137 {
		longlong bf1_ll:40;
		short s:16;
	};
	struct rec_13137 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13137);
    }
	event_type = 13038;
    {
	struct rec_13038 {
		char c;
		char bf1_c1:3;
		short bf1_s1:3;
		int bf1_i1:3;
		short s;
	};
	struct rec_13038 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13038);
    }
	event_type = 13138;
    {
	struct rec_13138 {
		char c:8;
		char bf1_c1:3;
		short bf1_s1:3;
		int bf1_i1:3;
		short s:16;
	};
	struct rec_13138 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13138);
    }
	event_type = 13039;
    {
	struct rec_13039 {
		char c;
		int bf1_i1:3;
		short bf1_s1:3;
		char bf1_c1:3;
		short s;
	};
	struct rec_13039 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13039);
    }
	event_type = 13139;
    {
	struct rec_13139 {
		char c:8;
		int bf1_i1:3;
		short bf1_s1:3;
		char bf1_c1:3;
		short s:16;
	};
	struct rec_13139 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13139);
    }
	event_type = 13040;
    {
	struct rec_13040 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		char	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
		char	c;
	};
	struct rec_13040 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13040);
    }
	event_type = 13140;
    {
	struct rec_13140 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		char	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
		char	c:8;
	};
	struct rec_13140 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13140);
    }
	event_type = 13041;
    {
	struct rec_13041 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:2;
		char	c;
	};
	struct rec_13041 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13041);
    }
	event_type = 13141;
    {
	struct rec_13141 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:2;
		char	c:8;
	};
	struct rec_13141 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13141);
    }
	event_type = 13042;
    {
	struct rec_13042 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		char	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_13042 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13042);
    }
	event_type = 13043;
    {
	struct rec_13043 {
		int	bf1_s1:3;
		int	bf1_s2:3;
		char	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_13043 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13043);
    }
	event_type = 13044;
    {
	struct rec_13044 {
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_13044 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13044);
    }
	event_type = 13045;
    {
	struct rec_13045 {
		char	c;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_13045 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13045);
    }
	event_type = 13145;
    {
	struct rec_13145 {
		char	c:8;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_13145 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13145);
    }
	event_type = 13046;
    {
	struct rec_13046 {
		int	i;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_13046 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13046);
    }
	event_type = 13146;
    {
	struct rec_13146 {
		int	i:32;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_13146 v1 = {-1, -1, -1, -1, -1, -1};
	v1.bf1_s3 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13146);
    }
	event_type = 13047;
    {
	struct rec_13047 {
		char	bf1_c:7;
		short	bf1_s:9;
		int	bf1_i:23;
	};
	struct rec_13047 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13047);
    }
	event_type = 13052;
    {
	struct rec_13052 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		short	bf1_s3:3;
		char	:0;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_13052 v1 = {-1, -1, -1, -1, -1};
	v1.bf1_s4 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13052);
    }
	event_type = 13053;
    {
	struct rec_13053 {
		char	c;
		char	bf1_c1:4;
		short	:0;
		int	bf1_i1:4;
	};
	struct rec_13053 v1 = {-1, -1, -1};
	v1.bf1_i1 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13053);
    }
	event_type = 13153;
    {
	struct rec_13153 {
		char	c:8;
		char	bf1_c1:4;
		short	:0;
		int	bf1_i1:4;
	};
	struct rec_13153 v1 = {-1, -1, -1};
	v1.bf1_i1 = -1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13153);
    }
	event_type = 13054;
    {
	struct rec_13054 {
		char	c[3]ARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_13054 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13054);
    }
	event_type = 13055;
    {
	struct rec_13055 {
		char	c[3]ARRAY_FMT;
		int	bf1_i1:10;
	};
	struct rec_13055 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13055);
    }
	event_type = 13056;
    {
	struct rec_13056 {
		struct b1	b1;
		int	bf1_i1:10;
	};
	struct rec_13056 v1 = {{-1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13056);
    }
	event_type = 13057;
    {
	struct rec_13057 {
		struct b1	b1[3]STRARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_13057 v1 = {{-1, -1, -1}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13057);
    }
	event_type = 13058;
    {
	struct rec_13058 {
		struct b2	b1[3]STRARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_13058 v1 = {{{-1,-1}, {-1,-1}, {-1,-1}}, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13058);
    }
	event_type = 13059;
    {
	struct rec_13059 {
		longlong bf1_ll:40;
		int bf1_i:2;
	};
	struct rec_13059 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13059);
    }
	event_type = 13060;
    {
	struct rec_13060 {
		longlong bf1_ll:40;
		int bf1_i:8;
	};
	struct rec_13060 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13060);
    }
	event_type = 13061;
    {
	struct rec_13061 {
		longlong bf1_ll:40;
		short x:15;
	};
	struct rec_13061 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13061);
    }
	event_type = 13062;
    {
	struct rec_13062 {
		longlong bf1_ll:32;
		int :8;
		short bf1_s:2;
	};
	struct rec_13062 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13062);
    }
	event_type = 13063;
    {
	struct rec_13063 {
		longlong bf1_ll:32;
		int :8;
		short s;
	};
	struct rec_13063 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13063);
    }
	event_type = 13163;
    {
	struct rec_13163 {
		longlong bf1_ll:32;
		int :8;
		short s:16;
	};
	struct rec_13163 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13163);
    }
	event_type = 13064;
    {
	struct rec_13064 {
		longlong bf1_ll:32;
		int :8;
		int bf1_i:2;
	};
	struct rec_13064 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13064);
    }
	event_type = 13065;
    {
	struct rec_13065 {
		longlong bf1_ll:32;
		int :8;
		short x:15;
	};
	struct rec_13065 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13065);
    }
	event_type = 13066;
    {
	struct rec_13066 {
		longlong bf1_ll:40;
		int bf1_i:25;
	};
	struct rec_13066 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13066);
    }
	event_type = 13067;
    {
	struct rec_13067 {
		longlong	bf1_ll:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:2;
		int	i:32;
	};
	struct rec_13067 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13067);
    }
	event_type = 13068;
    {
	struct rec_13068 {
		longlong	bf1_ll:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:32;
	};
	struct rec_13068 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13068);
    }
	event_type = 13069;
    {
	struct rec_13069 {
		char	c:5;
		short	s:9;
		int	i:17;
	};
	struct rec_13069 v1 = {-1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13069);
    }
	event_type = 13070;
    {
	struct rec_13070 {
		longlong	bf1_ll:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:7;
	};
	struct rec_13070 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13070);
    }
	event_type = 13071;
    {
	struct rec_13071 {
		int		i:2;
		longlong	ll:33;
	};
	struct rec_13071 v1 = {-1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13071);
    }
	event_type = 13072;
    {
	struct rec_13072 {
		longlong	bf1_ll:2;
		char	c:6;
		short	s1:8;
		longlong	ll1:33;
		int	i:10;
	};
	struct rec_13072 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13072);
    }
	event_type = 13073;
    {
	struct rec_13073 {
		int	i1:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:32;
	};
	struct rec_13073 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13073);
    }
	event_type = 13074;
    {
	struct rec_13074 {
		char	c1;
		char	c2;
		char	c3;
		longlong	ll1:33;
	};
	struct rec_13074 v1 = {-1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13074);
    }
	event_type = 13075;
    {
	struct rec_13075 {
		char	c1;
		char	c2;
		char	c3;
		char	c4;
		longlong	ll1:33;
	};
	struct rec_13075 v1 = {-1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13075);
    }
	event_type = 13076;
    {
	struct rec_13076 {
		char	c1;
		char	c2;
		char	c3;
		char	c4;
		char	c5;
		longlong	ll1:32;
	};
	struct rec_13076 v1 = {-1, -1, -1, -1, -1, -1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 13076);
    }

	exit(exitStatus);
}
