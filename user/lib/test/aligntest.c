/*
 * IBM Event Logging for Linux
 * Copyright (c) International Business Machines Corp., 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "posix_evlog.h"
#include "posix_evlsup.h"

/*
 * This spits out the same data as evl_log_write_test2.c, except that each
 * record is a single C struct.
 * All the templates are defined in align.t.
 */

main(int argc, char **argv)
{
	int status;
	posix_log_facility_t facility;
	int type;
	posix_log_severity_t severity;
	long larray[5]={30L,31L,32L,33L,34L};
	int iarray[5] = {1,2,3,4,5};

	long long llarray[5] = {10LL,11LL,12LL,13LL,14LL};
	unsigned long long ullarray[5]={20LL,21LL,22LL,23LL,24L};
	short sarray[5]={1,2,3,4,5};
	char carray[4]={'l','o','v','e'};
	unsigned char ucarray[4]={0xff,0x10,0xfe,0x20};
	long double ldarray[2]={2.33333, 3.333333};
	long double ld1 = 1.7676764;
	long double ld2 = 5.57575757;

	void *addr1 = (void*) 0xfeedf00d;
	void *addr2 = (void*) 0xfacef00d;
	void *addrarray[5] = { NULL, (void*)1, (void*)2, addr1, addr2 };
	
	facility = 8;
	severity = 3;
	
	type = 2001;
    {
	struct rec2001 {
		unsigned int	uint10;
		int	int10;
		int	iarray[5];
		short	short10;
		short	short11;
		/* string	love; */
		char	love[5];
		int	int20;
		int	iarrayAgain[5];
		long	longf0f0f0f0;
		char	charc;
	} rec;
	rec.uint10 = 10;
	rec.int10 = 10;
	memcpy(rec.iarray, iarray, 5*sizeof(int));
	rec.short10 = 10;
	rec.short11 = 11;
	strcpy(rec.love, "love");
	rec.int20 = 20;
	memcpy(rec.iarrayAgain, iarray, 5*sizeof(int));
	rec.longf0f0f0f0 = 0xf0f0f0f0;
	rec.charc = 'c';

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }

	type = 2002;
    {
	struct rec2002 {
		short		short9;
		long long	longlong56;
		unsigned long long	ulonglong56;
		int		int10;
		long long	llarray[3];
		/* string	peace; */
		char		peace[10];
		int		int20;
		int		iarray[5];
		long		longf0f0f0f0;
		char		charc;
	} rec;
	rec.short9 = 9;
	rec.longlong56 = 56LL;
	rec.ulonglong56 = 56LL;
	rec.int10 = 10;
	memcpy(rec.llarray, llarray, 3*sizeof(long long));
	strcpy(rec.peace, "peace");
	rec.int20 = 20;
	memcpy(rec.iarray, iarray, 5*sizeof(int));
	rec.longf0f0f0f0 = 0xf0f0f0f0;
	rec.charc = 'c';

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }

	type = 2003;
    {
    	struct rec2003 {
		long long		llarray[5];
		unsigned long long	ullarray[5];
		/* string linux */
		char			xlinux[10];
	} rec;
	memcpy(rec.llarray, llarray, 5*sizeof(long long));
	memcpy(rec.ullarray, ullarray, 5*sizeof(unsigned long long));
	strcpy(rec.xlinux, "linux");

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }

	type = 2004;
    {
    	struct rec2004 {
		int	iarray[5];
	} rec;
	memcpy(rec.iarray, iarray, 5*sizeof(int));

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }
	
	type = 2005;
    {
	struct rec2005 {
		short			short1;
		unsigned short		ushort2;
		long			long1111;
		unsigned long		ulong2222;
		unsigned long long	ulonglong10;
		long long		longlong10;
		float			float0_123;
		double			double9_999;
		long double		ld1;
	} rec;
	rec.short1 = 1;
	rec.ushort2 = 2;
	rec.long1111 = 1111L;
	rec.ulong2222 = 2222L;
	rec.ulonglong10 = 10LL;
	rec.longlong10 = 10LL;
	rec.float0_123 = 0.123;
	rec.double9_999 = 9.999;
	rec.ld1 = ld1;

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }
	
	type = 2006;
    {
	struct rec2006 {
		char		charA;
		unsigned char	ucharB;
		long double	ld2;
	} rec;
	rec.charA = 'A';
	rec.ucharB = 'B';
	rec.ld2 = ld2;

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }

	type = 2007;
    {
	struct rec2007 {
		long long		llarray[5];
		unsigned long long	ullarray[5];
		int			int10;
	} rec;
	memcpy(rec.llarray, llarray, 5*sizeof(long long));
	memcpy(rec.ullarray, ullarray, 5*sizeof(unsigned long long));
	rec.int10 = 10;

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }

	type = 2008;
    {
    	struct rec2008 {
		int		iarray[5];
		unsigned int	uiarray[5];
		short		sarray[5];
		unsigned short	usarray[5];
		char		carray[4];
		unsigned char	ucarray[4];
		unsigned char	peace[6];
		long		larray[5];
		unsigned char	xlinux[12];
	} rec;
	memcpy(rec.iarray, iarray, 5*sizeof(int));
	memcpy(rec.uiarray, iarray, 5*sizeof(int));
	memcpy(rec.sarray, sarray, 5*sizeof(short));
	memcpy(rec.usarray, sarray, 5*sizeof(short));
	memcpy(rec.carray, carray, 4*sizeof(char));
	memcpy(rec.ucarray, ucarray, 4*sizeof(unsigned char));
	strcpy(rec.peace, "peace");
	memcpy(rec.larray, larray, 5*sizeof(long));
	strcpy(rec.xlinux, "linux");

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }

	type = 2009;
    {
	struct rec2009 {
		int		int10;
		int		int11;
		int		int12;
		int		int13;
		int		int14;
		int		int15;
		long long	longlong10;
		long long	longlong11;
		long double	ldarray[2];
		long double	ld1;
		long double	ld2;
	} rec;
	rec.int10 = 10;
	rec.int11 = 11;
	rec.int12 = 12;
	rec.int13 = 13;
	rec.int14 = 14;
	rec.int15 = 15;
	rec.longlong10 = 10LL;
	rec.longlong11 = 11LL;
	memcpy(rec.ldarray, ldarray, 2*sizeof(long double));
	rec.ld1 = ld1;
	rec.ld2 = ld2;

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }

	type = 2010;
    {
    	struct rec2010 {
		void	*addr1;
		void	*addr2;
		void	*addrarray[5];
		void	*addr_feedface;
	} rec;
	rec.addr1 = addr1;
	rec.addr2 = addr2;
	memcpy(rec.addrarray, addrarray, 5*sizeof(void*));
	rec.addr_feedface = (void*) 0xfeedface;

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }

	type = 2020;
    {
    	struct foreignWord {
		char	word[13];
		int	language;
	};

	struct rec2020 {
		struct foreignWord	peace;
		struct foreignWord	love;
		struct foreignWord	slinux;
		struct foreignWord	elinux;
		char			equals[5];
		struct foreignWord	paz;
		struct foreignWord	amor;
	} rec;
	strcpy(rec.peace.word, "peace");
	strcpy(rec.love.word, "love");
	strcpy(rec.slinux.word, "Linux");
	strcpy(rec.elinux.word, "Linux");
	strcpy(rec.paz.word, "paz");
	strcpy(rec.amor.word, "amor");
	rec.peace.language = 'E';
	rec.love.language = 'E';
	rec.slinux.language = 'S';
	rec.elinux.language = 'E';
	rec.paz.language = 'S';
	rec.amor.language = 'S';
	strcpy(rec.equals, "is");

	evl_log_write(facility, type, severity, 0,
				"char[]", sizeof(rec), &rec,
				"endofdata");
    }
	exit(0);
}
