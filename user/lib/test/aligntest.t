/*
 * Templates that correspond to the evl_log_write() calls in
 * evl_log_write_test2.c
 */
facility 8; event_type 2001;
aligned attributes {
	uint	uint10;
	int	int10;
	int	iarray[5]	"(%d )";
	short	short10;
	short	short11;
	char	love[5]		"%s";
	int	int20;
	int	iarrayAgain[5]	"(%d )";
	long	longf0f0f0f0	"%#lx";
	char	charc		"%c";
}
format
10=%uint10%
10=%int10%
1 2 3 4 5 =%iarray%
10=%short10%
11=%short11%
love=%love%
20=%int20%
1 2 3 4 5 =%iarrayAgain%
0xf0f0f0f0=%longf0f0f0f0%
c=%charc%
END


facility 8; event_type 2002;
aligned attributes {
	short		short9;
	longlong	longlong56;
	ulonglong	ulonglong56;
	int		int10;
	longlong	llarray[3]	"(%lld )";
	/* string	peace; */
	char		peace[10]	"%s";
	int		int20;
	int		iarray[5]	"(%d )";
	long		longf0f0f0f0	"%#lx";
	char		charc		"%c";
}
format
9=%short9%
56=%longlong56%
56=%ulonglong56%
10=%int10%
10 11 12 =%llarray%
peace=%peace%
20=%int20%
1 2 3 4 5 =%iarray%
0xf0f0f0f0=%longf0f0f0f0%
c=%charc%
END


facility 8; event_type 2003;
aligned attributes {
	longlong	llarray[5]	"(%lld )";
	ulonglong	ullarray[5]	"(%llu )";
	char		linux[10]	"%s";
}
format
10 11 12 13 14 =%llarray%
20 21 22 23 24 =%ullarray%
linux=%linux%
END


facility 8; event_type 2004;
aligned attributes {
	int	iarray[5]	"(%d )";
}
format
1 2 3 4 5 =%iarray%
END


facility 8; event_type 2005;
aligned attributes {
	short		short1;
	ushort		ushort2;
	long		long1111;
	ulong		ulong2222;
	ulonglong	ulonglong10;
	longlong	longlong10;
	float		float0_123	"%.3f";
	double		double9_999	"%.3f";
	ldouble		ld1	"%.7Lf";
}
format
1=%short1%
2=%ushort2%
1111=%long1111%
2222=%ulong2222%
10=%ulonglong10%
10=%longlong10%
0.123=%float0_123%
9.999=%double9_999%
1.7676764=%ld1%
END


facility 8; event_type 2006;
aligned attributes {
	char	charA	"%c";
	uchar	ucharB	"%c";
	ldouble	ld2	"%.8Lf";
}
format
A=%charA%
B=%ucharB%
5.57575757=%ld2%
END


facility 8; event_type 2007;
aligned attributes {
	longlong	llarray[5]	"(%lld )";
	ulonglong	ullarray[5]	"(%llu )";
	int		int10;
}
format
10 11 12 13 14 =%llarray%
20 21 22 23 24 =%ullarray%
10=%int10%
END


facility 8; event_type 2008;
aligned attributes {
	int	iarray[5]	"(%d )";
	uint	uiarray[5]	"(%u )";
	short	sarray[5]	"(%d )";
	ushort	usarray[5]	"(%u )";
	char	carray[4]	"(%c)";
	uchar	ucarray[4]	"(%#x )";
	uchar	peace[6]	"%s";
	long	larray[5]	"(%ld )";
	uchar	linux[12]	"%s";
}
format
1 2 3 4 5 =%iarray%
1 2 3 4 5 =%uiarray%
1 2 3 4 5 =%sarray%
1 2 3 4 5 =%usarray%
love=%carray%
0xff 0x10 0xfe 0x20 =%ucarray%
peace=%peace%
30 31 32 33 34 =%larray%
linux=%linux%
END


facility 8; event_type 2009;
aligned attributes {
	int		int10;
	int		int11;
	int		int12;
	int		int13;
	int		int14;
	int		int15;
	longlong	longlong10;
	longlong	longlong11;
	ldouble		ldarray[2]	"(%.6Lf )";
	ldouble		ld1		"%.7Lf";
	ldouble		ld2		"%.8Lf";
}
format
10=%int10%
11=%int11%
12=%int12%
13=%int13%
14=%int14%
15=%int15%
10=%longlong10%
11=%longlong11%
2.333330 3.333333 =%ldarray%
1.7676764=%ld1%
5.57575757=%ld2%
END


facility 8; event_type 2010;
aligned attributes {
	address	addr1;
	address	addr2;
	address	addrarray[5]	"(%p )";
	address	addr_feedface;
}
format
0xfeedf00d=%addr1%
0xfacef00d=%addr2%
(nil) 0x1 0x2 0xfeedf00d 0xfacef00d =%addrarray%
0xfeedface=%addr_feedface%
END


struct foreignWord;
aligned attributes {
	char	word[13]	"%s";
	int	language	"%c";
}
format
(%language%: %word%)
END


facility 8; event_type 2020;
aligned attributes {
	struct foreignWord	peace;
	struct foreignWord	love;
	struct foreignWord	slinux;
	struct foreignWord	elinux;
	char			equals[5]	"%s";
	struct foreignWord	paz;
	struct foreignWord	amor;
}
format
(E: peace) is (S: paz)=%peace% %equals% %paz%
(E: love) is (S: amor)=%love% %equals% %amor%
(E: Linux) is (S: Linux)=%elinux% %equals% %slinux%
