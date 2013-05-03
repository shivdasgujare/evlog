/*
 * IBM Event Logging for Linux
 * Copyright (c) International Business Machines Corp., 2003
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

#define EVL_FACILITY_NAME LOCAL6
#include "evlog.h"
#include "posix_evlog.h"
#include "posix_evlsup.h"

/*
 * Test of %[ support (for formatting arrays) in syslogat().  Derived from
 * printfArray.c
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
	
	/* syslog messages to LOCAL2, evlog to LOCAL6 */
	openlog("syslogatArray", 0, LOG_LOCAL2);
	
	severity = LOG_ERR;
	
	type = 1051;
	syslogat(severity,
"10|%u\n"
"10|%d\n"
"1 2 3 4 5|%[d\n"
"10|%d\n"
"11|%d\n"
"love|%s\n"
"20|%d\n"
"1 2 3 4 5 |%[d \n"
"0xf0f0f0f0|%#lx\n"
"c|%c\n",
10, 10, iarray,5," ", 10, 11, "love", 20, iarray,5," ", 0xf0f0f0f0L, 'c');

	type = 1052;
	syslogat(severity,
"9|%d\n"
"56|%lld\n"
"56|%llu\n"
"10|%d\n"
"10 11 12|%[lld\n"
"peace|%s\n"
"20|%d\n"
"1 2 3 4 5|%[d\n"
"0xf0f0f0f0|%#lx\n"
"c|%c\n",
9, 56LL, 56LL, 10, llarray,3," ", "peace", 20, iarray,5," ", 0xf0f0f0f0L, 'c');

	type = 1053;
	syslogat(severity,
"10 11 12 13 14|%[lld\n"
"20 21 22 23 24|%[llu\n"
"linux|%s\n",
llarray,5," ", ullarray,5," ", "linux");

	type = 1054;
	syslogat(severity,
"1 2 3 4 5 |%[d \n",
iarray,5," ");

	type = 1055;
	syslogat(severity,
"1|%d\n"
"2|%u\n"
"1111|%ld\n"
"2222|%lu\n"
"10|%llu\n"
"10|%lld\n"
"0.123|%.3f\n"
"9.999|%.3f\n"
"1.7676764|%.7Lf\n",
1, 2, 1111L, 2222L, 10LL, 10LL, 0.123, 9.999, ld1);
	
	type = 1056;
	syslogat(severity,
"A|%c\n"
"B|%c\n"
"5.57575757|%.8Lf\n",
'A', 'B', ld2);

	type = 1057;
	syslogat(severity,
"10 11 12 13 14|%[lld\n"
"20 21 22 23 24|%[llu\n"
"10|%d\n",
llarray,5," ", ullarray,5," ", 10);

	type = 1058;
	syslogat(severity,
"1 2 3 4 5|%[d\n"
"1 2 3 4 5|%[u\n"
"1 2 3 4 5|%[hd\n"
"1 2 3 4 5|%[hu\n"
"love|%[c\n"
"0xff 0x10 0xfe 0x20|%[#hhx\n"
"peace|%s\n"
"30 31 32 33 34|%[ld\n"
"linux|%s\n",
iarray,5," ", iarray,5," ", sarray,5," ", sarray,5," ", carray,4,"",
ucarray,4," ", "peace", larray,5," ", "linux");

	type = 1059;
	syslogat(severity,
"10|%d\n"
"11|%d\n"
"12|%d\n"
"13|%d\n"
"14|%d\n"
"15|%d\n"
"10|%lld\n"
"11|%lld\n"
"2.333330 3.333333|%[.6Lf\n"
"1.7676764|%.7Lf\n"
"5.57575757|%.8Lf\n",
10,11,12,13,14,15, 10LL, 11LL, ldarray,2," ", ld1,ld2);

	type = 1060;
	syslogat(severity,
"0xfeedf00d|%p\n"
"0xfacef00d|%p\n"
"(nil) 0x1 0x2 0xfeedf00d 0xfacef00d|%[p\n"
"0xfeedface|%p\n",
addr1, addr2, addrarray,5," ", (void*)0xfeedface);

	type = 1061;
	syslogat(severity,
"peace love and linux|%[s\n",
plal,4," ");

	type = 1062;
	copyStringToWideString("peace", wpeace);
	copyStringToWideString("love", wlove);
	copyStringToWideString("and", wand);
	copyStringToWideString("linux", wlinux);
	wc = L'W';
	syslogat(severity,
"W|%lc\n"
"peace|%ls\n"
"love|%ls\n"
"linux|%ls\n"
"peace love and linux|%[S\n",
wc, wpeace, wlove, wlinux, wplal,4," ");

	exit(0);
}
