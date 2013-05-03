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
#include <errno.h>
#include <wchar.h>
#include "posix_evlog.h"
#include "posix_evlsup.h"

/*
 * This is just like evl_log_write_test.c, except that each record is assigned
 * a different event number so we can do a different template for each one.
 * All the templates are defined in evl_log_write_test2.t.
 */

wchar_t *
copyStringToWideString(const char *s, wchar_t *ws)
{
	while (*ws++ = *s++)
		;
	return ws;
}

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

	char *plal[4] = { "peace", "love", "and", "linux" };

	wchar_t wc, wpeace[10], wlove[10], wand[10], wlinux[10];
	wchar_t *wplal[4] = { wpeace, wlove, wand, wlinux };
	
	facility = 8;
	severity = 3;
	
	type = 1001;
	evl_log_write(facility, type, severity, 0,
				"uint", 10,
				"int", 10,
				"int[]",5,&iarray,
				"2*short", 10, 11, 
				"string", "love",
				"int", 20, 
				"int[]", 5, &iarray, 
				"long", 0xf0f0f0f0L, 
				"char", 'c',
				"endofdata");

	type = 1002;
	evl_log_write(facility, type, severity, 0,
				"short", 9,
				"longlong", 56LL,				// must postfix value with LL 
				"ulonglong", 56LL,
				"int", 10,
				"longlong[]",3,&llarray, 
				"string", "peace",
				"int", 20, 
				"int[]", 5, &iarray, 
				"long", 0xf0f0f0f0L,
				"char", 'c',
				"endofdata");
	
	type = 1003;
	evl_log_write(facility, type, severity, 0,
				"longlong[]", 5, &llarray,
				"ulonglong[]", 5, &ullarray,
				"string", "linux",
				"endofdata");
	
	type = 1004;
	evl_log_write(facility, type, severity, 0,
				"int[]", 5, &iarray,
				"endofdata");

	type = 1005;
	evl_log_write(facility, type, severity, 0,
				"short", 1,
				"ushort", 2, 
				"long", 1111L, 
				"ulong", 2222L, 
				"ulonglong", 10LL,
				"longlong", 10LL,
				"float", 0.123, 
				"double", 9.999,  
				"ldouble", ld1,					// must use long double variable
				"endofdata"); 

	
	type = 1006;
	evl_log_write(facility, type, severity, 0,
				"char", 'A', 
				"uchar", 'B', 
				"ldouble", ld2,					// must use long double variable		
				"endofdata"); 

	type = 1007;
	evl_log_write(facility, type, severity, 0,
				"longlong[]", 5, &llarray,
				"ulonglong[]", 5, &ullarray,
				"int", 10,
				"endofdata");

	type = 1008;
	evl_log_write(facility, type, severity, 0,
				"int[]", 5, &iarray,
				"uint[]", 5, &iarray,
				"short[]", 5, &sarray,
				"ushort[]", 5, &sarray,
				"char[]", 4, &carray,
				"uchar[]", 4, &ucarray,
				"string", "peace",
				"long[]",5, &larray,
				"string","linux",
				"endofdata");

	type = 1009;
	evl_log_write(facility, type, severity, 0,
				"6*int", 10,11,12,13,14,15,
				"2*longlong", 10LL, 11LL, 
				"ldouble[]", 2, &ldarray,
				"2*ldouble", ld1,ld2,
				"endofdata");

	type = 1010;
	evl_log_write(facility, type, severity, 0,
				"2*address", addr1, addr2,
				"address[]", 5, addrarray,
				"address", (void*)0xfeedface,
				"endofdata");

	type = 1011;
	evl_log_write(facility, type, severity, 0,
				"string[]", 4, plal,
				"endofdata");

	type = 1012;
	copyStringToWideString("peace", wpeace);
	copyStringToWideString("love", wlove);
	copyStringToWideString("and", wand);
	copyStringToWideString("linux", wlinux);
	wc = L'W';
	evl_log_write(facility, type, severity, 0,
				"wchar", wc,
				"wstring", wpeace,
				"2*wstring", wlove, wlinux,
				"wstring[]", 4, wplal,
				"endofdata");
	exit(0);
}
