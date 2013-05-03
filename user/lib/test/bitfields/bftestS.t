#define ARRAY_FMT "(%d,)"
#define STRARRAY_FMT "(%Z,)"
typedef int xint;
typedef uint uxint;

struct b1;
aligned attributes {
	schar c;
}
format string "{%c%}"
END

struct b2;
aligned attributes {
	schar c1:4;
	schar c2:4;
}
format string "{%c1%,%c2%}"
END
facility 8; event_type 53001;
aligned attributes {
	int	bf1_i:2;
	short	bf1_s:2;
	schar	bf1_c:2;
}
format string "natts = 3 \t%bf1_i% %bf1_s% %bf1_c% "
END
facility 8; event_type 53002;
aligned attributes {
	schar	bf1_c:2;
	short	bf1_s:2;
	int	bf1_i:2;
}
format string "natts = 3 \t%bf1_c% %bf1_s% %bf1_i% "
END
facility 8; event_type 53003;
aligned attributes {
	schar	bf1_c:2;
	schar	bf1_c2:2;
}
format string "natts = 2 \t%bf1_c% %bf1_c2% "
END
facility 8; event_type 53004;
aligned attributes {
	schar	bf1_c:2;
	schar	bf1_c2:2;
	schar	bf1_c3:2;
	schar	bf1_c4:2;
	schar	bf1_c5:2;
}
format string "natts = 5 \t%bf1_c% %bf1_c2% %bf1_c3% %bf1_c4% %bf1_c5% "
END
facility 8; event_type 53005;
aligned attributes {
	int	bf1_i:2;
	int	bf1_i2:2;
	int	bf1_i3:2;
	int	bf1_i4:2;
	int	bf1_i5:2;
}
format string "natts = 5 \t%bf1_i% %bf1_i2% %bf1_i3% %bf1_i4% %bf1_i5% "
END
facility 8; event_type 53006;
aligned attributes {
	short	bf1_s:2;
	schar	bf1_c2:2;
	schar	bf1_c3:2;
	schar	bf1_c4:2;
}
format string "natts = 4 \t%bf1_s% %bf1_c2% %bf1_c3% %bf1_c4% "
END
facility 8; event_type 53007;
aligned attributes {
	schar	bf1_c2:2;
	schar	bf1_c3:2;
	schar	bf1_c4:2;
	short	bf1_s:2;
}
format string "natts = 4 \t%bf1_c2% %bf1_c3% %bf1_c4% %bf1_s% "
END
facility 8; event_type 53008;
aligned attributes {
	schar	bf1_c:7;
	short	bf1_s:8;
	int	bf1_i:16;
}
format string "natts = 3 \t%bf1_c% %bf1_s% %bf1_i% "
END
facility 8; event_type 53010;
aligned attributes {
	schar	bf1_c1:3;
	schar	bf1_c2:3;
	schar	bf1_c3:3;
	schar	bf1_c4:3;
	schar	bf1_c5:3;
}
format string "natts = 5 \t%bf1_c1% %bf1_c2% %bf1_c3% %bf1_c4% %bf1_c5% "
END
facility 8; event_type 53011;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53012;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	schar	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53013;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	int	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53014;
aligned attributes {
	short	bf1_s1:3;
	schar	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_s1% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 53015;
aligned attributes {
	schar	bf1_c1:3;
	short	:0;
	schar	bf1_c2:3;
	schar	bf1_c3:3;
}
format string "natts = 3 \t%bf1_c1% %bf1_c2% %bf1_c3% "
END
facility 8; event_type 53016;
aligned attributes {
	short	bf1_s1:3;
	schar	:0;
	schar	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_s1% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 53017;
aligned attributes {
	short	bf1_s1:3;
	short	:0;
	schar	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_s1% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 53018;
aligned attributes {
	short	bf1_s1:3;
	schar	:0;
	short	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_s1% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 53019;
aligned attributes {
	int	bf1_i:3;
	schar	:0;
	int	bf1_i2:3;
	short	:0;
	int	bf1_i3:3;
}
format string "natts = 3 \t%bf1_i% %bf1_i2% %bf1_i3% "
END
facility 8; event_type 53020;
aligned attributes {
	schar	bf1_c:8;
	schar	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_c% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 53021;
aligned attributes {
	short bf1_s1:4;
	schar bf1_c1:4;
	schar bf1_c2:4;
	schar bf1_c3:4;
	schar bf1_c4:4;
	schar c1;
}
format string "natts = 6 \t%bf1_s1% %bf1_c1% %bf1_c2% %bf1_c3% %bf1_c4% %c1% "
END
facility 8; event_type 53121;
aligned attributes {
	short bf1_s1:4;
	schar bf1_c1:4;
	schar bf1_c2:4;
	schar bf1_c3:4;
	schar bf1_c4:4;
	schar c1:8;
}
format string "natts = 6 \t%bf1_s1% %bf1_c1% %bf1_c2% %bf1_c3% %bf1_c4% %c1% "
END
facility 8; event_type 53022;
aligned attributes {
	int	bf1_i:16;
	short	bf1_s:7;
	short	bf1_c:9;
}
format string "natts = 3 \t%bf1_i% %bf1_s% %bf1_c% "
END
facility 8; event_type 53023;
aligned attributes {
	schar	bf1_c:7;
	short	bf1_s:10;
	int	bf1_i:23;
}
format string "natts = 3 \t%bf1_c% %bf1_s% %bf1_i% "
END
facility 8; event_type 53024;
aligned attributes {
	schar	bf1_c:7;
	short	bf1_s:10;
	int	bf1_i:23;
	schar	c;
	schar	bf1_c2:7;
	short	bf1_s2:10;
	int	bf1_i2:23;
}
format string "natts = 7 \t%bf1_c% %bf1_s% %bf1_i% %c% %bf1_c2% %bf1_s2% %bf1_i2% "
END
facility 8; event_type 53124;
aligned attributes {
	schar	bf1_c:7;
	short	bf1_s:10;
	int	bf1_i:23;
	schar	c:8;
	schar	bf1_c2:7;
	short	bf1_s2:10;
	int	bf1_i2:23;
}
format string "natts = 7 \t%bf1_c% %bf1_s% %bf1_i% %c% %bf1_c2% %bf1_s2% %bf1_i2% "
END
facility 8; event_type 53025;
aligned attributes {
	int	bf1_i:15;
	schar	c1;
	schar	c2;
}
format string "natts = 3 \t%bf1_i% %c1% %c2% "
END
facility 8; event_type 53125;
aligned attributes {
	int	bf1_i:15;
	schar	c1:8;
	schar	c2:8;
}
format string "natts = 3 \t%bf1_i% %c1% %c2% "
END
facility 8; event_type 53026;
aligned attributes {
	int	bf1_i:2;
	short	bf1_s:2;
	schar	bf1_c:2;
	schar	c;
}
format string "natts = 4 \t%bf1_i% %bf1_s% %bf1_c% %c% "
END
facility 8; event_type 53126;
aligned attributes {
	int	bf1_i:2;
	short	bf1_s:2;
	schar	bf1_c:2;
	schar	c:8;
}
format string "natts = 4 \t%bf1_i% %bf1_s% %bf1_c% %c% "
END
facility 8; event_type 53027;
aligned attributes {
	longlong	bf1_ll:2;
	short	bf1_s:2;
	schar	bf1_c:2;
	schar	c;
}
format string "natts = 4 \t%bf1_ll% %bf1_s% %bf1_c% %c% "
END
facility 8; event_type 53127;
aligned attributes {
	longlong	bf1_ll:2;
	short	bf1_s:2;
	schar	bf1_c:2;
	schar	c:8;
}
format string "natts = 4 \t%bf1_ll% %bf1_s% %bf1_c% %c% "
END
facility 8; event_type 53028;
aligned attributes {
	longlong	bf1_ll:2;
	short	bf1_s:2;
	schar	bf1_c:2;
	schar	c;
	short	s1;
}
format string "natts = 5 \t%bf1_ll% %bf1_s% %bf1_c% %c% %s1% "
END
facility 8; event_type 53128;
aligned attributes {
	longlong	bf1_ll:2;
	short	bf1_s:2;
	schar	bf1_c:2;
	schar	c:8;
	short	s1:16;
}
format string "natts = 5 \t%bf1_ll% %bf1_s% %bf1_c% %c% %s1% "
END
facility 8; event_type 53029;
aligned attributes {
	ulonglong	bf1_ll:2;
	short	bf1_s:2;
	schar	bf1_c:2;
	schar	c;
	short	s1;
}
format string "natts = 5 \t%bf1_ll% %bf1_s% %bf1_c% %c% %s1% "
END
facility 8; event_type 53129;
aligned attributes {
	ulonglong	bf1_ll:2;
	short	bf1_s:2;
	schar	bf1_c:2;
	schar	c:8;
	short	s1:16;
}
format string "natts = 5 \t%bf1_ll% %bf1_s% %bf1_c% %c% %s1% "
END
facility 8; event_type 53030;
aligned attributes {
	longlong	bf1_ll:2;
	schar	c;
	short	s1;
	longlong	ll1;
	int	i;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 53130;
aligned attributes {
	longlong	bf1_ll:2;
	schar	c:8;
	short	s1:16;
	longlong	ll1:64;
	int	i:32;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 53031;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	longlong	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53032;
aligned attributes {
	xint	bf1_i:2;
}
format string "natts = 1 \t%bf1_i% "
END
facility 8; event_type 53033;
aligned attributes {
	int :2;
}
format string "natts = 0 \t"
END
facility 8; event_type 53035;
aligned attributes {
	schar bf1_c1:2;
	int :2;
	schar bf1_c2:2;
}
format string "natts = 2 \t%bf1_c1% %bf1_c2% "
END
facility 8; event_type 53036;
aligned attributes {
	longlong bf1_ll:40;
	short bf1_s:2;
}
format string "natts = 2 \t%bf1_ll% %bf1_s% "
END
facility 8; event_type 53037;
aligned attributes {
	longlong bf1_ll:40;
	short s;
}
format string "natts = 2 \t%bf1_ll% %s% "
END
facility 8; event_type 53137;
aligned attributes {
	longlong bf1_ll:40;
	short s:16;
}
format string "natts = 2 \t%bf1_ll% %s% "
END
facility 8; event_type 53038;
aligned attributes {
	schar c;
	schar bf1_c1:3;
	short bf1_s1:3;
	int bf1_i1:3;
	short s;
}
format string "natts = 5 \t%c% %bf1_c1% %bf1_s1% %bf1_i1% %s% "
END
facility 8; event_type 53138;
aligned attributes {
	schar c:8;
	schar bf1_c1:3;
	short bf1_s1:3;
	int bf1_i1:3;
	short s:16;
}
format string "natts = 5 \t%c% %bf1_c1% %bf1_s1% %bf1_i1% %s% "
END
facility 8; event_type 53039;
aligned attributes {
	schar c;
	int bf1_i1:3;
	short bf1_s1:3;
	schar bf1_c1:3;
	short s;
}
format string "natts = 5 \t%c% %bf1_i1% %bf1_s1% %bf1_c1% %s% "
END
facility 8; event_type 53139;
aligned attributes {
	schar c:8;
	int bf1_i1:3;
	short bf1_s1:3;
	schar bf1_c1:3;
	short s:16;
}
format string "natts = 5 \t%c% %bf1_i1% %bf1_s1% %bf1_c1% %s% "
END
facility 8; event_type 53040;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	schar	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
	schar	c;
}
format string "natts = 6 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% %c% "
END
facility 8; event_type 53140;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	schar	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
	schar	c:8;
}
format string "natts = 6 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% %c% "
END
facility 8; event_type 53041;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	int	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:2;
	schar	c;
}
format string "natts = 6 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% %c% "
END
facility 8; event_type 53141;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	int	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:2;
	schar	c:8;
}
format string "natts = 6 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% %c% "
END
facility 8; event_type 53042;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	schar	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53043;
aligned attributes {
	int	bf1_s1:3;
	int	bf1_s2:3;
	schar	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53044;
aligned attributes {
	int	bf1_s1:3;
	int	bf1_s2:3;
	longlong	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53045;
aligned attributes {
	schar	c;
	int	bf1_s1:3;
	int	bf1_s2:3;
	longlong	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 6 \t%c% %bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53145;
aligned attributes {
	schar	c:8;
	int	bf1_s1:3;
	int	bf1_s2:3;
	longlong	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 6 \t%c% %bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53046;
aligned attributes {
	int	i;
	int	bf1_s1:3;
	int	bf1_s2:3;
	longlong	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 6 \t%i% %bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53146;
aligned attributes {
	int	i:32;
	int	bf1_s1:3;
	int	bf1_s2:3;
	longlong	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 6 \t%i% %bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53047;
aligned attributes {
	schar	bf1_c:7;
	short	bf1_s:9;
	int	bf1_i:23;
}
format string "natts = 3 \t%bf1_c% %bf1_s% %bf1_i% "
END
facility 8; event_type 53052;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	short	bf1_s3:3;
	schar	:0;
	short	bf1_s4:3;
	short	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 53053;
aligned attributes {
	schar	c;
	schar	bf1_c1:4;
	short	:0;
	int	bf1_i1:4;
}
format string "natts = 3 \t%c% %bf1_c1% %bf1_i1% "
END
facility 8; event_type 53153;
aligned attributes {
	schar	c:8;
	schar	bf1_c1:4;
	short	:0;
	int	bf1_i1:4;
}
format string "natts = 3 \t%c% %bf1_c1% %bf1_i1% "
END
facility 8; event_type 53054;
aligned attributes {
	schar	c[3]ARRAY_FMT;
	int	bf1_i1:4;
}
format string "natts = 2 \t%c% %bf1_i1% "
END
facility 8; event_type 53055;
aligned attributes {
	schar	c[3]ARRAY_FMT;
	int	bf1_i1:10;
}
format string "natts = 2 \t%c% %bf1_i1% "
END
facility 8; event_type 53056;
aligned attributes {
	struct b1	b1;
	int	bf1_i1:10;
}
format string "natts = 2 \t%b1% %bf1_i1% "
END
facility 8; event_type 53057;
aligned attributes {
	struct b1	b1[3]STRARRAY_FMT;
	int	bf1_i1:4;
}
format string "natts = 2 \t%b1% %bf1_i1% "
END
facility 8; event_type 53058;
aligned attributes {
	struct b2	b1[3]STRARRAY_FMT;
	int	bf1_i1:4;
}
format string "natts = 2 \t%b1% %bf1_i1% "
END
facility 8; event_type 53059;
aligned attributes {
	longlong bf1_ll:40;
	int bf1_i:2;
}
format string "natts = 2 \t%bf1_ll% %bf1_i% "
END
facility 8; event_type 53060;
aligned attributes {
	longlong bf1_ll:40;
	int bf1_i:8;
}
format string "natts = 2 \t%bf1_ll% %bf1_i% "
END
facility 8; event_type 53061;
aligned attributes {
	longlong bf1_ll:40;
	short x:15;
}
format string "natts = 2 \t%bf1_ll% %x% "
END
facility 8; event_type 53062;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	short bf1_s:2;
}
format string "natts = 2 \t%bf1_ll% %bf1_s% "
END
facility 8; event_type 53063;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	short s;
}
format string "natts = 2 \t%bf1_ll% %s% "
END
facility 8; event_type 53163;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	short s:16;
}
format string "natts = 2 \t%bf1_ll% %s% "
END
facility 8; event_type 53064;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	int bf1_i:2;
}
format string "natts = 2 \t%bf1_ll% %bf1_i% "
END
facility 8; event_type 53065;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	short x:15;
}
format string "natts = 2 \t%bf1_ll% %x% "
END
facility 8; event_type 53066;
aligned attributes {
	longlong bf1_ll:40;
	int bf1_i:25;
}
format string "natts = 2 \t%bf1_ll% %bf1_i% "
END
facility 8; event_type 53067;
aligned attributes {
	longlong	bf1_ll:2;
	schar	c:8;
	short	s1:16;
	longlong	ll1:2;
	int	i:32;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 53068;
aligned attributes {
	longlong	bf1_ll:2;
	schar	c:8;
	short	s1:16;
	longlong	ll1:33;
	int	i:32;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 53069;
aligned attributes {
	schar	c:5;
	short	s:9;
	int	i:17;
}
format string "natts = 3 \t%c% %s% %i% "
END
facility 8; event_type 53070;
aligned attributes {
	longlong	bf1_ll:2;
	schar	c:8;
	short	s1:16;
	longlong	ll1:33;
	int	i:7;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 53071;
aligned attributes {
	int		i:2;
	longlong	ll:33;
}
format string "natts = 2 \t%i% %ll% "
END
facility 8; event_type 53072;
aligned attributes {
	longlong	bf1_ll:2;
	schar	c:6;
	short	s1:8;
	longlong	ll1:33;
	int	i:10;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 53073;
aligned attributes {
	int	i1:2;
	schar	c:8;
	short	s1:16;
	longlong	ll1:33;
	int	i:32;
}
format string "natts = 5 \t%i1% %c% %s1% %ll1% %i% "
END
facility 8; event_type 53074;
aligned attributes {
	schar	c1;
	schar	c2;
	schar	c3;
	longlong	ll1:33;
}
format string "natts = 4 \t%c1% %c2% %c3% %ll1% "
END
facility 8; event_type 53075;
aligned attributes {
	schar	c1;
	schar	c2;
	schar	c3;
	schar	c4;
	longlong	ll1:33;
}
format string "natts = 5 \t%c1% %c2% %c3% %c4% %ll1% "
END
facility 8; event_type 53076;
aligned attributes {
	schar	c1;
	schar	c2;
	schar	c3;
	schar	c4;
	schar	c5;
	longlong	ll1:32;
}
format string "natts = 6 \t%c1% %c2% %c3% %c4% %c5% %ll1% "
END
