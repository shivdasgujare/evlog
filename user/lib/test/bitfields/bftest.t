#define ARRAY_FMT "(%d,)"
#define STRARRAY_FMT "(%Z,)"
typedef int xint;
typedef uint uxint;

struct b1;
aligned attributes {
	char c;
}
format string "{%c%}"
END

struct b2;
aligned attributes {
	char c1:4;
	char c2:4;
}
format string "{%c1%,%c2%}"
END
facility 8; event_type 13001;
aligned attributes {
	int	bf1_i:2;
	short	bf1_s:2;
	char	bf1_c:2;
}
format string "natts = 3 \t%bf1_i% %bf1_s% %bf1_c% "
END
facility 8; event_type 13002;
aligned attributes {
	char	bf1_c:2;
	short	bf1_s:2;
	int	bf1_i:2;
}
format string "natts = 3 \t%bf1_c% %bf1_s% %bf1_i% "
END
facility 8; event_type 13003;
aligned attributes {
	char	bf1_c:2;
	char	bf1_c2:2;
}
format string "natts = 2 \t%bf1_c% %bf1_c2% "
END
facility 8; event_type 13004;
aligned attributes {
	char	bf1_c:2;
	char	bf1_c2:2;
	char	bf1_c3:2;
	char	bf1_c4:2;
	char	bf1_c5:2;
}
format string "natts = 5 \t%bf1_c% %bf1_c2% %bf1_c3% %bf1_c4% %bf1_c5% "
END
facility 8; event_type 13005;
aligned attributes {
	int	bf1_i:2;
	int	bf1_i2:2;
	int	bf1_i3:2;
	int	bf1_i4:2;
	int	bf1_i5:2;
}
format string "natts = 5 \t%bf1_i% %bf1_i2% %bf1_i3% %bf1_i4% %bf1_i5% "
END
facility 8; event_type 13006;
aligned attributes {
	short	bf1_s:2;
	char	bf1_c2:2;
	char	bf1_c3:2;
	char	bf1_c4:2;
}
format string "natts = 4 \t%bf1_s% %bf1_c2% %bf1_c3% %bf1_c4% "
END
facility 8; event_type 13007;
aligned attributes {
	char	bf1_c2:2;
	char	bf1_c3:2;
	char	bf1_c4:2;
	short	bf1_s:2;
}
format string "natts = 4 \t%bf1_c2% %bf1_c3% %bf1_c4% %bf1_s% "
END
facility 8; event_type 13008;
aligned attributes {
	char	bf1_c:7;
	short	bf1_s:8;
	int	bf1_i:16;
}
format string "natts = 3 \t%bf1_c% %bf1_s% %bf1_i% "
END
facility 8; event_type 13010;
aligned attributes {
	char	bf1_c1:3;
	char	bf1_c2:3;
	char	bf1_c3:3;
	char	bf1_c4:3;
	char	bf1_c5:3;
}
format string "natts = 5 \t%bf1_c1% %bf1_c2% %bf1_c3% %bf1_c4% %bf1_c5% "
END
facility 8; event_type 13011;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 13012;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	char	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 13013;
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
facility 8; event_type 13014;
aligned attributes {
	short	bf1_s1:3;
	char	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_s1% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 13015;
aligned attributes {
	char	bf1_c1:3;
	short	:0;
	char	bf1_c2:3;
	char	bf1_c3:3;
}
format string "natts = 3 \t%bf1_c1% %bf1_c2% %bf1_c3% "
END
facility 8; event_type 13016;
aligned attributes {
	short	bf1_s1:3;
	char	:0;
	char	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_s1% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 13017;
aligned attributes {
	short	bf1_s1:3;
	short	:0;
	char	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_s1% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 13018;
aligned attributes {
	short	bf1_s1:3;
	char	:0;
	short	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_s1% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 13019;
aligned attributes {
	int	bf1_i:3;
	char	:0;
	int	bf1_i2:3;
	short	:0;
	int	bf1_i3:3;
}
format string "natts = 3 \t%bf1_i% %bf1_i2% %bf1_i3% "
END
facility 8; event_type 13020;
aligned attributes {
	char	bf1_c:8;
	char	:0;
	short	bf1_s2:3;
	short	bf1_s3:3;
}
format string "natts = 3 \t%bf1_c% %bf1_s2% %bf1_s3% "
END
facility 8; event_type 13021;
aligned attributes {
	short bf1_s1:4;
	char bf1_c1:4;
	char bf1_c2:4;
	char bf1_c3:4;
	char bf1_c4:4;
	char c1;
}
format string "natts = 6 \t%bf1_s1% %bf1_c1% %bf1_c2% %bf1_c3% %bf1_c4% %c1% "
END
facility 8; event_type 13121;
aligned attributes {
	short bf1_s1:4;
	char bf1_c1:4;
	char bf1_c2:4;
	char bf1_c3:4;
	char bf1_c4:4;
	char c1:8;
}
format string "natts = 6 \t%bf1_s1% %bf1_c1% %bf1_c2% %bf1_c3% %bf1_c4% %c1% "
END
facility 8; event_type 13022;
aligned attributes {
	int	bf1_i:16;
	short	bf1_s:7;
	short	bf1_c:9;
}
format string "natts = 3 \t%bf1_i% %bf1_s% %bf1_c% "
END
facility 8; event_type 13023;
aligned attributes {
	char	bf1_c:7;
	short	bf1_s:10;
	int	bf1_i:23;
}
format string "natts = 3 \t%bf1_c% %bf1_s% %bf1_i% "
END
facility 8; event_type 13024;
aligned attributes {
	char	bf1_c:7;
	short	bf1_s:10;
	int	bf1_i:23;
	char	c;
	char	bf1_c2:7;
	short	bf1_s2:10;
	int	bf1_i2:23;
}
format string "natts = 7 \t%bf1_c% %bf1_s% %bf1_i% %c% %bf1_c2% %bf1_s2% %bf1_i2% "
END
facility 8; event_type 13124;
aligned attributes {
	char	bf1_c:7;
	short	bf1_s:10;
	int	bf1_i:23;
	char	c:8;
	char	bf1_c2:7;
	short	bf1_s2:10;
	int	bf1_i2:23;
}
format string "natts = 7 \t%bf1_c% %bf1_s% %bf1_i% %c% %bf1_c2% %bf1_s2% %bf1_i2% "
END
facility 8; event_type 13025;
aligned attributes {
	int	bf1_i:15;
	char	c1;
	char	c2;
}
format string "natts = 3 \t%bf1_i% %c1% %c2% "
END
facility 8; event_type 13125;
aligned attributes {
	int	bf1_i:15;
	char	c1:8;
	char	c2:8;
}
format string "natts = 3 \t%bf1_i% %c1% %c2% "
END
facility 8; event_type 13026;
aligned attributes {
	int	bf1_i:2;
	short	bf1_s:2;
	char	bf1_c:2;
	char	c;
}
format string "natts = 4 \t%bf1_i% %bf1_s% %bf1_c% %c% "
END
facility 8; event_type 13126;
aligned attributes {
	int	bf1_i:2;
	short	bf1_s:2;
	char	bf1_c:2;
	char	c:8;
}
format string "natts = 4 \t%bf1_i% %bf1_s% %bf1_c% %c% "
END
facility 8; event_type 13027;
aligned attributes {
	longlong	bf1_ll:2;
	short	bf1_s:2;
	char	bf1_c:2;
	char	c;
}
format string "natts = 4 \t%bf1_ll% %bf1_s% %bf1_c% %c% "
END
facility 8; event_type 13127;
aligned attributes {
	longlong	bf1_ll:2;
	short	bf1_s:2;
	char	bf1_c:2;
	char	c:8;
}
format string "natts = 4 \t%bf1_ll% %bf1_s% %bf1_c% %c% "
END
facility 8; event_type 13028;
aligned attributes {
	longlong	bf1_ll:2;
	short	bf1_s:2;
	char	bf1_c:2;
	char	c;
	short	s1;
}
format string "natts = 5 \t%bf1_ll% %bf1_s% %bf1_c% %c% %s1% "
END
facility 8; event_type 13128;
aligned attributes {
	longlong	bf1_ll:2;
	short	bf1_s:2;
	char	bf1_c:2;
	char	c:8;
	short	s1:16;
}
format string "natts = 5 \t%bf1_ll% %bf1_s% %bf1_c% %c% %s1% "
END
facility 8; event_type 13029;
aligned attributes {
	ulonglong	bf1_ll:2;
	short	bf1_s:2;
	char	bf1_c:2;
	char	c;
	short	s1;
}
format string "natts = 5 \t%bf1_ll% %bf1_s% %bf1_c% %c% %s1% "
END
facility 8; event_type 13129;
aligned attributes {
	ulonglong	bf1_ll:2;
	short	bf1_s:2;
	char	bf1_c:2;
	char	c:8;
	short	s1:16;
}
format string "natts = 5 \t%bf1_ll% %bf1_s% %bf1_c% %c% %s1% "
END
facility 8; event_type 13030;
aligned attributes {
	longlong	bf1_ll:2;
	char	c;
	short	s1;
	longlong	ll1;
	int	i;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 13130;
aligned attributes {
	longlong	bf1_ll:2;
	char	c:8;
	short	s1:16;
	longlong	ll1:64;
	int	i:32;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 13031;
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
facility 8; event_type 13032;
aligned attributes {
	xint	bf1_i:2;
}
format string "natts = 1 \t%bf1_i% "
END
facility 8; event_type 13033;
aligned attributes {
	int :2;
}
format string "natts = 0 \t"
END
facility 8; event_type 13035;
aligned attributes {
	char bf1_c1:2;
	int :2;
	char bf1_c2:2;
}
format string "natts = 2 \t%bf1_c1% %bf1_c2% "
END
facility 8; event_type 13036;
aligned attributes {
	longlong bf1_ll:40;
	short bf1_s:2;
}
format string "natts = 2 \t%bf1_ll% %bf1_s% "
END
facility 8; event_type 13037;
aligned attributes {
	longlong bf1_ll:40;
	short s;
}
format string "natts = 2 \t%bf1_ll% %s% "
END
facility 8; event_type 13137;
aligned attributes {
	longlong bf1_ll:40;
	short s:16;
}
format string "natts = 2 \t%bf1_ll% %s% "
END
facility 8; event_type 13038;
aligned attributes {
	char c;
	char bf1_c1:3;
	short bf1_s1:3;
	int bf1_i1:3;
	short s;
}
format string "natts = 5 \t%c% %bf1_c1% %bf1_s1% %bf1_i1% %s% "
END
facility 8; event_type 13138;
aligned attributes {
	char c:8;
	char bf1_c1:3;
	short bf1_s1:3;
	int bf1_i1:3;
	short s:16;
}
format string "natts = 5 \t%c% %bf1_c1% %bf1_s1% %bf1_i1% %s% "
END
facility 8; event_type 13039;
aligned attributes {
	char c;
	int bf1_i1:3;
	short bf1_s1:3;
	char bf1_c1:3;
	short s;
}
format string "natts = 5 \t%c% %bf1_i1% %bf1_s1% %bf1_c1% %s% "
END
facility 8; event_type 13139;
aligned attributes {
	char c:8;
	int bf1_i1:3;
	short bf1_s1:3;
	char bf1_c1:3;
	short s:16;
}
format string "natts = 5 \t%c% %bf1_i1% %bf1_s1% %bf1_c1% %s% "
END
facility 8; event_type 13040;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	char	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
	char	c;
}
format string "natts = 6 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% %c% "
END
facility 8; event_type 13140;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	char	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:3;
	char	c:8;
}
format string "natts = 6 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% %c% "
END
facility 8; event_type 13041;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	int	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:2;
	char	c;
}
format string "natts = 6 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% %c% "
END
facility 8; event_type 13141;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	int	:0;
	short	bf1_s3:3;
	short	bf1_s4:3;
	short	bf1_s5:2;
	char	c:8;
}
format string "natts = 6 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% %c% "
END
facility 8; event_type 13042;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	char	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 13043;
aligned attributes {
	int	bf1_s1:3;
	int	bf1_s2:3;
	char	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 13044;
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
facility 8; event_type 13045;
aligned attributes {
	char	c;
	int	bf1_s1:3;
	int	bf1_s2:3;
	longlong	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 6 \t%c% %bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 13145;
aligned attributes {
	char	c:8;
	int	bf1_s1:3;
	int	bf1_s2:3;
	longlong	:0;
	int	bf1_s3:3;
	int	bf1_s4:3;
	int	bf1_s5:3;
}
format string "natts = 6 \t%c% %bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 13046;
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
facility 8; event_type 13146;
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
facility 8; event_type 13047;
aligned attributes {
	char	bf1_c:7;
	short	bf1_s:9;
	int	bf1_i:23;
}
format string "natts = 3 \t%bf1_c% %bf1_s% %bf1_i% "
END
facility 8; event_type 13052;
aligned attributes {
	short	bf1_s1:3;
	short	bf1_s2:3;
	short	bf1_s3:3;
	char	:0;
	short	bf1_s4:3;
	short	bf1_s5:3;
}
format string "natts = 5 \t%bf1_s1% %bf1_s2% %bf1_s3% %bf1_s4% %bf1_s5% "
END
facility 8; event_type 13053;
aligned attributes {
	char	c;
	char	bf1_c1:4;
	short	:0;
	int	bf1_i1:4;
}
format string "natts = 3 \t%c% %bf1_c1% %bf1_i1% "
END
facility 8; event_type 13153;
aligned attributes {
	char	c:8;
	char	bf1_c1:4;
	short	:0;
	int	bf1_i1:4;
}
format string "natts = 3 \t%c% %bf1_c1% %bf1_i1% "
END
facility 8; event_type 13054;
aligned attributes {
	char	c[3]ARRAY_FMT;
	int	bf1_i1:4;
}
format string "natts = 2 \t%c% %bf1_i1% "
END
facility 8; event_type 13055;
aligned attributes {
	char	c[3]ARRAY_FMT;
	int	bf1_i1:10;
}
format string "natts = 2 \t%c% %bf1_i1% "
END
facility 8; event_type 13056;
aligned attributes {
	struct b1	b1;
	int	bf1_i1:10;
}
format string "natts = 2 \t%b1% %bf1_i1% "
END
facility 8; event_type 13057;
aligned attributes {
	struct b1	b1[3]STRARRAY_FMT;
	int	bf1_i1:4;
}
format string "natts = 2 \t%b1% %bf1_i1% "
END
facility 8; event_type 13058;
aligned attributes {
	struct b2	b1[3]STRARRAY_FMT;
	int	bf1_i1:4;
}
format string "natts = 2 \t%b1% %bf1_i1% "
END
facility 8; event_type 13059;
aligned attributes {
	longlong bf1_ll:40;
	int bf1_i:2;
}
format string "natts = 2 \t%bf1_ll% %bf1_i% "
END
facility 8; event_type 13060;
aligned attributes {
	longlong bf1_ll:40;
	int bf1_i:8;
}
format string "natts = 2 \t%bf1_ll% %bf1_i% "
END
facility 8; event_type 13061;
aligned attributes {
	longlong bf1_ll:40;
	short x:15;
}
format string "natts = 2 \t%bf1_ll% %x% "
END
facility 8; event_type 13062;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	short bf1_s:2;
}
format string "natts = 2 \t%bf1_ll% %bf1_s% "
END
facility 8; event_type 13063;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	short s;
}
format string "natts = 2 \t%bf1_ll% %s% "
END
facility 8; event_type 13163;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	short s:16;
}
format string "natts = 2 \t%bf1_ll% %s% "
END
facility 8; event_type 13064;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	int bf1_i:2;
}
format string "natts = 2 \t%bf1_ll% %bf1_i% "
END
facility 8; event_type 13065;
aligned attributes {
	longlong bf1_ll:32;
	int :8;
	short x:15;
}
format string "natts = 2 \t%bf1_ll% %x% "
END
facility 8; event_type 13066;
aligned attributes {
	longlong bf1_ll:40;
	int bf1_i:25;
}
format string "natts = 2 \t%bf1_ll% %bf1_i% "
END
facility 8; event_type 13067;
aligned attributes {
	longlong	bf1_ll:2;
	char	c:8;
	short	s1:16;
	longlong	ll1:2;
	int	i:32;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 13068;
aligned attributes {
	longlong	bf1_ll:2;
	char	c:8;
	short	s1:16;
	longlong	ll1:33;
	int	i:32;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 13069;
aligned attributes {
	char	c:5;
	short	s:9;
	int	i:17;
}
format string "natts = 3 \t%c% %s% %i% "
END
facility 8; event_type 13070;
aligned attributes {
	longlong	bf1_ll:2;
	char	c:8;
	short	s1:16;
	longlong	ll1:33;
	int	i:7;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 13071;
aligned attributes {
	int		i:2;
	longlong	ll:33;
}
format string "natts = 2 \t%i% %ll% "
END
facility 8; event_type 13072;
aligned attributes {
	longlong	bf1_ll:2;
	char	c:6;
	short	s1:8;
	longlong	ll1:33;
	int	i:10;
}
format string "natts = 5 \t%bf1_ll% %c% %s1% %ll1% %i% "
END
facility 8; event_type 13073;
aligned attributes {
	int	i1:2;
	char	c:8;
	short	s1:16;
	longlong	ll1:33;
	int	i:32;
}
format string "natts = 5 \t%i1% %c% %s1% %ll1% %i% "
END
facility 8; event_type 13074;
aligned attributes {
	char	c1;
	char	c2;
	char	c3;
	longlong	ll1:33;
}
format string "natts = 4 \t%c1% %c2% %c3% %ll1% "
END
facility 8; event_type 13075;
aligned attributes {
	char	c1;
	char	c2;
	char	c3;
	char	c4;
	longlong	ll1:33;
}
format string "natts = 5 \t%c1% %c2% %c3% %c4% %ll1% "
END
facility 8; event_type 13076;
aligned attributes {
	char	c1;
	char	c2;
	char	c3;
	char	c4;
	char	c5;
	longlong	ll1:32;
}
format string "natts = 6 \t%c1% %c2% %c3% %c4% %c5% %ll1% "
END
