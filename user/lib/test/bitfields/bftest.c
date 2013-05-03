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

	event_type = 3001;
    {
	struct rec_3001 {
		int	bf1_i:2;
		short	bf1_s:2;
		char	bf1_c:2;
	};
	struct rec_3001 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3001);
    }
	event_type = 3002;
    {
	struct rec_3002 {
		char	bf1_c:2;
		short	bf1_s:2;
		int	bf1_i:2;
	};
	struct rec_3002 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3002);
    }
	event_type = 3003;
    {
	struct rec_3003 {
		char	bf1_c:2;
		char	bf1_c2:2;
	};
	struct rec_3003 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3003);
    }
	event_type = 3004;
    {
	struct rec_3004 {
		char	bf1_c:2;
		char	bf1_c2:2;
		char	bf1_c3:2;
		char	bf1_c4:2;
		char	bf1_c5:2;
	};
	struct rec_3004 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3004);
    }
	event_type = 3005;
    {
	struct rec_3005 {
		int	bf1_i:2;
		int	bf1_i2:2;
		int	bf1_i3:2;
		int	bf1_i4:2;
		int	bf1_i5:2;
	};
	struct rec_3005 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3005);
    }
	event_type = 3006;
    {
	struct rec_3006 {
		short	bf1_s:2;
		char	bf1_c2:2;
		char	bf1_c3:2;
		char	bf1_c4:2;
	};
	struct rec_3006 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3006);
    }
	event_type = 3007;
    {
	struct rec_3007 {
		char	bf1_c2:2;
		char	bf1_c3:2;
		char	bf1_c4:2;
		short	bf1_s:2;
	};
	struct rec_3007 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3007);
    }
	event_type = 3008;
    {
	struct rec_3008 {
		char	bf1_c:7;
		short	bf1_s:8;
		int	bf1_i:16;
	};
	struct rec_3008 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3008);
    }
	event_type = 3010;
    {
	struct rec_3010 {
		char	bf1_c1:3;
		char	bf1_c2:3;
		char	bf1_c3:3;
		char	bf1_c4:3;
		char	bf1_c5:3;
	};
	struct rec_3010 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3010);
    }
	event_type = 3011;
    {
	struct rec_3011 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_3011 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3011);
    }
	event_type = 3012;
    {
	struct rec_3012 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		char	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_3012 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3012);
    }
	event_type = 3013;
    {
	struct rec_3013 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_3013 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3013);
    }
	event_type = 3014;
    {
	struct rec_3014 {
		short	bf1_s1:3;
		char	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_3014 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3014);
    }
	event_type = 3015;
    {
	struct rec_3015 {
		char	bf1_c1:3;
		short	:0;
		char	bf1_c2:3;
		char	bf1_c3:3;
	};
	struct rec_3015 v1 = {1, 1, 1};
	v1.bf1_c2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3015);
    }
	event_type = 3016;
    {
	struct rec_3016 {
		short	bf1_s1:3;
		char	:0;
		char	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_3016 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3016);
    }
	event_type = 3017;
    {
	struct rec_3017 {
		short	bf1_s1:3;
		short	:0;
		char	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_3017 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3017);
    }
	event_type = 3018;
    {
	struct rec_3018 {
		short	bf1_s1:3;
		char	:0;
		short	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_3018 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3018);
    }
	event_type = 3019;
    {
	struct rec_3019 {
		int	bf1_i:3;
		char	:0;
		int	bf1_i2:3;
		short	:0;
		int	bf1_i3:3;
	};
	struct rec_3019 v1 = {1, 1, 1};
	v1.bf1_i2 = 1;	/* gcc bug 5157 workaround */
	v1.bf1_i3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3019);
    }
	event_type = 3020;
    {
	struct rec_3020 {
		char	bf1_c:8;
		char	:0;
		short	bf1_s2:3;
		short	bf1_s3:3;
	};
	struct rec_3020 v1 = {1, 1, 1};
	v1.bf1_s2 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3020);
    }
	event_type = 3021;
    {
	struct rec_3021 {
		short bf1_s1:4;
		char bf1_c1:4;
		char bf1_c2:4;
		char bf1_c3:4;
		char bf1_c4:4;
		char c1;
	};
	struct rec_3021 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3021);
    }
	event_type = 3121;
    {
	struct rec_3121 {
		short bf1_s1:4;
		char bf1_c1:4;
		char bf1_c2:4;
		char bf1_c3:4;
		char bf1_c4:4;
		char c1:8;
	};
	struct rec_3121 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3121);
    }
	event_type = 3022;
    {
	struct rec_3022 {
		int	bf1_i:16;
		short	bf1_s:7;
		short	bf1_c:9;
	};
	struct rec_3022 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3022);
    }
	event_type = 3023;
    {
	struct rec_3023 {
		char	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
	};
	struct rec_3023 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3023);
    }
	event_type = 3024;
    {
	struct rec_3024 {
		char	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
		char	c;
		char	bf1_c2:7;
		short	bf1_s2:10;
		int	bf1_i2:23;
	};
	struct rec_3024 v1 = {1, 1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3024);
    }
	event_type = 3124;
    {
	struct rec_3124 {
		char	bf1_c:7;
		short	bf1_s:10;
		int	bf1_i:23;
		char	c:8;
		char	bf1_c2:7;
		short	bf1_s2:10;
		int	bf1_i2:23;
	};
	struct rec_3124 v1 = {1, 1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3124);
    }
	event_type = 3025;
    {
	struct rec_3025 {
		int	bf1_i:15;
		char	c1;
		char	c2;
	};
	struct rec_3025 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3025);
    }
	event_type = 3125;
    {
	struct rec_3125 {
		int	bf1_i:15;
		char	c1:8;
		char	c2:8;
	};
	struct rec_3125 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3125);
    }
	event_type = 3026;
    {
	struct rec_3026 {
		int	bf1_i:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c;
	};
	struct rec_3026 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3026);
    }
	event_type = 3126;
    {
	struct rec_3126 {
		int	bf1_i:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c:8;
	};
	struct rec_3126 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3126);
    }
	event_type = 3027;
    {
	struct rec_3027 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c;
	};
	struct rec_3027 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3027);
    }
	event_type = 3127;
    {
	struct rec_3127 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c:8;
	};
	struct rec_3127 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3127);
    }
	event_type = 3028;
    {
	struct rec_3028 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c;
		short	s1;
	};
	struct rec_3028 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3028);
    }
	event_type = 3128;
    {
	struct rec_3128 {
		longlong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c:8;
		short	s1:16;
	};
	struct rec_3128 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3128);
    }
	event_type = 3029;
    {
	struct rec_3029 {
		ulonglong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c;
		short	s1;
	};
	struct rec_3029 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3029);
    }
	event_type = 3129;
    {
	struct rec_3129 {
		ulonglong	bf1_ll:2;
		short	bf1_s:2;
		char	bf1_c:2;
		char	c:8;
		short	s1:16;
	};
	struct rec_3129 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3129);
    }
	event_type = 3030;
    {
	struct rec_3030 {
		longlong	bf1_ll:2;
		char	c;
		short	s1;
		longlong	ll1;
		int	i;
	};
	struct rec_3030 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3030);
    }
	event_type = 3130;
    {
	struct rec_3130 {
		longlong	bf1_ll:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:64;
		int	i:32;
	};
	struct rec_3130 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3130);
    }
	event_type = 3031;
    {
	struct rec_3031 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		longlong	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_3031 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3031);
    }
	event_type = 3032;
    {
	struct rec_3032 {
		xint	bf1_i:2;
	};
	struct rec_3032 v1 = {1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3032);
    }
	event_type = 3033;
    {
	struct rec_3033 {
		int :2;
	};
	struct rec_3033 v1;
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3033);
    }
	event_type = 3035;
    {
	struct rec_3035 {
		char bf1_c1:2;
		int :2;
		char bf1_c2:2;
	};
	struct rec_3035 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3035);
    }
	event_type = 3036;
    {
	struct rec_3036 {
		longlong bf1_ll:40;
		short bf1_s:2;
	};
	struct rec_3036 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3036);
    }
	event_type = 3037;
    {
	struct rec_3037 {
		longlong bf1_ll:40;
		short s;
	};
	struct rec_3037 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3037);
    }
	event_type = 3137;
    {
	struct rec_3137 {
		longlong bf1_ll:40;
		short s:16;
	};
	struct rec_3137 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3137);
    }
	event_type = 3038;
    {
	struct rec_3038 {
		char c;
		char bf1_c1:3;
		short bf1_s1:3;
		int bf1_i1:3;
		short s;
	};
	struct rec_3038 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3038);
    }
	event_type = 3138;
    {
	struct rec_3138 {
		char c:8;
		char bf1_c1:3;
		short bf1_s1:3;
		int bf1_i1:3;
		short s:16;
	};
	struct rec_3138 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3138);
    }
	event_type = 3039;
    {
	struct rec_3039 {
		char c;
		int bf1_i1:3;
		short bf1_s1:3;
		char bf1_c1:3;
		short s;
	};
	struct rec_3039 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3039);
    }
	event_type = 3139;
    {
	struct rec_3139 {
		char c:8;
		int bf1_i1:3;
		short bf1_s1:3;
		char bf1_c1:3;
		short s:16;
	};
	struct rec_3139 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3139);
    }
	event_type = 3040;
    {
	struct rec_3040 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		char	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
		char	c;
	};
	struct rec_3040 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3040);
    }
	event_type = 3140;
    {
	struct rec_3140 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		char	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:3;
		char	c:8;
	};
	struct rec_3140 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3140);
    }
	event_type = 3041;
    {
	struct rec_3041 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:2;
		char	c;
	};
	struct rec_3041 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3041);
    }
	event_type = 3141;
    {
	struct rec_3141 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		int	:0;
		short	bf1_s3:3;
		short	bf1_s4:3;
		short	bf1_s5:2;
		char	c:8;
	};
	struct rec_3141 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3141);
    }
	event_type = 3042;
    {
	struct rec_3042 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		char	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_3042 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3042);
    }
	event_type = 3043;
    {
	struct rec_3043 {
		int	bf1_s1:3;
		int	bf1_s2:3;
		char	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_3043 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3043);
    }
	event_type = 3044;
    {
	struct rec_3044 {
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_3044 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3044);
    }
	event_type = 3045;
    {
	struct rec_3045 {
		char	c;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_3045 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3045);
    }
	event_type = 3145;
    {
	struct rec_3145 {
		char	c:8;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_3145 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3145);
    }
	event_type = 3046;
    {
	struct rec_3046 {
		int	i;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_3046 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3046);
    }
	event_type = 3146;
    {
	struct rec_3146 {
		int	i:32;
		int	bf1_s1:3;
		int	bf1_s2:3;
		longlong	:0;
		int	bf1_s3:3;
		int	bf1_s4:3;
		int	bf1_s5:3;
	};
	struct rec_3146 v1 = {1, 1, 1, 1, 1, 1};
	v1.bf1_s3 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3146);
    }
	event_type = 3047;
    {
	struct rec_3047 {
		char	bf1_c:7;
		short	bf1_s:9;
		int	bf1_i:23;
	};
	struct rec_3047 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3047);
    }
	event_type = 3052;
    {
	struct rec_3052 {
		short	bf1_s1:3;
		short	bf1_s2:3;
		short	bf1_s3:3;
		char	:0;
		short	bf1_s4:3;
		short	bf1_s5:3;
	};
	struct rec_3052 v1 = {1, 1, 1, 1, 1};
	v1.bf1_s4 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3052);
    }
	event_type = 3053;
    {
	struct rec_3053 {
		char	c;
		char	bf1_c1:4;
		short	:0;
		int	bf1_i1:4;
	};
	struct rec_3053 v1 = {1, 1, 1};
	v1.bf1_i1 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3053);
    }
	event_type = 3153;
    {
	struct rec_3153 {
		char	c:8;
		char	bf1_c1:4;
		short	:0;
		int	bf1_i1:4;
	};
	struct rec_3153 v1 = {1, 1, 1};
	v1.bf1_i1 = 1;	/* gcc bug 5157 workaround */
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3153);
    }
	event_type = 3054;
    {
	struct rec_3054 {
		char	c[3]ARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_3054 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3054);
    }
	event_type = 3055;
    {
	struct rec_3055 {
		char	c[3]ARRAY_FMT;
		int	bf1_i1:10;
	};
	struct rec_3055 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3055);
    }
	event_type = 3056;
    {
	struct rec_3056 {
		struct b1	b1;
		int	bf1_i1:10;
	};
	struct rec_3056 v1 = {{1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3056);
    }
	event_type = 3057;
    {
	struct rec_3057 {
		struct b1	b1[3]STRARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_3057 v1 = {{1, 1, 1}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3057);
    }
	event_type = 3058;
    {
	struct rec_3058 {
		struct b2	b1[3]STRARRAY_FMT;
		int	bf1_i1:4;
	};
	struct rec_3058 v1 = {{{1,1}, {1,1}, {1,1}}, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3058);
    }
	event_type = 3059;
    {
	struct rec_3059 {
		longlong bf1_ll:40;
		int bf1_i:2;
	};
	struct rec_3059 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3059);
    }
	event_type = 3060;
    {
	struct rec_3060 {
		longlong bf1_ll:40;
		int bf1_i:8;
	};
	struct rec_3060 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3060);
    }
	event_type = 3061;
    {
	struct rec_3061 {
		longlong bf1_ll:40;
		short x:15;
	};
	struct rec_3061 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3061);
    }
	event_type = 3062;
    {
	struct rec_3062 {
		longlong bf1_ll:32;
		int :8;
		short bf1_s:2;
	};
	struct rec_3062 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3062);
    }
	event_type = 3063;
    {
	struct rec_3063 {
		longlong bf1_ll:32;
		int :8;
		short s;
	};
	struct rec_3063 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3063);
    }
	event_type = 3163;
    {
	struct rec_3163 {
		longlong bf1_ll:32;
		int :8;
		short s:16;
	};
	struct rec_3163 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3163);
    }
	event_type = 3064;
    {
	struct rec_3064 {
		longlong bf1_ll:32;
		int :8;
		int bf1_i:2;
	};
	struct rec_3064 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3064);
    }
	event_type = 3065;
    {
	struct rec_3065 {
		longlong bf1_ll:32;
		int :8;
		short x:15;
	};
	struct rec_3065 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3065);
    }
	event_type = 3066;
    {
	struct rec_3066 {
		longlong bf1_ll:40;
		int bf1_i:25;
	};
	struct rec_3066 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3066);
    }
	event_type = 3067;
    {
	struct rec_3067 {
		longlong	bf1_ll:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:2;
		int	i:32;
	};
	struct rec_3067 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3067);
    }
	event_type = 3068;
    {
	struct rec_3068 {
		longlong	bf1_ll:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:32;
	};
	struct rec_3068 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3068);
    }
	event_type = 3069;
    {
	struct rec_3069 {
		char	c:5;
		short	s:9;
		int	i:17;
	};
	struct rec_3069 v1 = {1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3069);
    }
	event_type = 3070;
    {
	struct rec_3070 {
		longlong	bf1_ll:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:7;
	};
	struct rec_3070 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3070);
    }
	event_type = 3071;
    {
	struct rec_3071 {
		int		i:2;
		longlong	ll:33;
	};
	struct rec_3071 v1 = {1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3071);
    }
	event_type = 3072;
    {
	struct rec_3072 {
		longlong	bf1_ll:2;
		char	c:6;
		short	s1:8;
		longlong	ll1:33;
		int	i:10;
	};
	struct rec_3072 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3072);
    }
	event_type = 3073;
    {
	struct rec_3073 {
		int	i1:2;
		char	c:8;
		short	s1:16;
		longlong	ll1:33;
		int	i:32;
	};
	struct rec_3073 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3073);
    }
	event_type = 3074;
    {
	struct rec_3074 {
		char	c1;
		char	c2;
		char	c3;
		longlong	ll1:33;
	};
	struct rec_3074 v1 = {1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3074);
    }
	event_type = 3075;
    {
	struct rec_3075 {
		char	c1;
		char	c2;
		char	c3;
		char	c4;
		longlong	ll1:33;
	};
	struct rec_3075 v1 = {1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3075);
    }
	event_type = 3076;
    {
	struct rec_3076 {
		char	c1;
		char	c2;
		char	c3;
		char	c4;
		char	c5;
		longlong	ll1:32;
	};
	struct rec_3076 v1 = {1, 1, 1, 1, 1, 1};
	status = posix_log_write(facility, event_type, severity,
		&v1, sizeof(v1), POSIX_LOG_BINARY, 0);
	checkStatus(status, 3076);
    }

	exit(exitStatus);
}
