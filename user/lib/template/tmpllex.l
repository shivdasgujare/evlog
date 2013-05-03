%{
/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2001
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <wchar.h>
#include <assert.h>

#include "evl_template.h"
#include "tt.tab.h"

/*
 * Internationalization support: flex should work OK with input files encoded
 * in character encodings such as ASCII, Latin-1, and UTF-8.  For these
 * character sets, a null byte appears only at the end of a string.
 * For encodings such as UCS-2 and UCS-4, the text may be full of null bytes,
 * so we can't handle those.
 */

#define MYEOF '\0'

static int getOctalEscape(const char *digits, int isCharConst);
static wchar_t getHexEscape(const char *digits);
static int getCharEscape(char c);
static char *getString(int quoted);
static int skipComment();
static void processLineDirective();

static int inputFromFile = 1;
static tmpl_parser_context_t *pc;
%}
%option noyywrap

/* Define components of floating-point constants. */
digit	[0-9]
exp	[eE][-+]?{digit}+
%%
aligned		{ return TK_ALIGNED; }
attributes	{ return TK_ATTRIBUTES; }
const		{ return TK_CONST; }
default		{ return TK_DEFAULT; }
delimiter	{ return TK_DELIMITER; }
description	{ return TK_DESCRIPTION; }
event_type	{ return TK_EVENT_TYPE; }
facility	{ return TK_FACILITY; }
import		{ return TK_IMPORT; }
redirect	{ return TK_REDIRECT; }
struct		{ return TK_STRUCT; }
typedef		{ return TK_TYPEDEF; }

char		{ return TK_TYCHAR; }
double		{ return TK_TYDOUBLE; }
int		{ return TK_TYINT; }
long		{ return TK_TYLONG; }
short		{ return TK_TYSHORT; }
signed		{ return TK_TYSIGNED; }
unsigned	{ return TK_TYUNSIGNED; }

address		{ ttlval.ival = TY_ADDRESS;	return TK_TYPENAME; }
float		{ ttlval.ival = TY_FLOAT;	return TK_TYPENAME; }
ldouble		{ ttlval.ival = TY_LDOUBLE;	return TK_TYPENAME; }
longlong	{ ttlval.ival = TY_LONGLONG;	return TK_TYPENAME; }
schar		{ ttlval.ival = TY_CHAR;	return TK_TYPENAME; }
string		{ ttlval.ival = TY_STRING;	return TK_TYPENAME; }
uchar		{ ttlval.ival = TY_UCHAR;	return TK_TYPENAME; }
uint		{ ttlval.ival = TY_UINT;	return TK_TYPENAME; }
ulong		{ ttlval.ival = TY_ULONG;	return TK_TYPENAME; }
ulonglong	{ ttlval.ival = TY_ULONGLONG;	return TK_TYPENAME; }
ushort		{ ttlval.ival = TY_USHORT;	return TK_TYPENAME; }
wchar		{ ttlval.ival = TY_WCHAR;	return TK_TYPENAME; }
wstring		{ ttlval.ival = TY_WSTRING;	return TK_TYPENAME; }

[_A-Za-z][A-Za-z0-9_]* {
	char *thisName;
	if (! (thisName = strdup((const char*)yytext)) ) {
		perror("strdup");
		return(ERRTOK);
	}
	ttlval.sval = thisName;
	return TK_NAME;
	}

0[xX][0-9A-Fa-f]+ {
	/* hex */
	ttlval.ival = strtoul((char *)yytext, (char **)NULL, 16);
	return TK_INTEGER;
	}
0[0-7]*	{
	/* octal */
	ttlval.ival = strtoul((char *)yytext, (char **)NULL, 8);
	return TK_INTEGER;
	}
[1-9][0-9]*	{
	/* decimal */
	ttlval.ival = strtoul((char *)yytext, (char **)NULL, 10);
	return TK_INTEGER;
	}

'\\[0-7]+'	{
	int c = getOctalEscape((char *)&yytext[2], 1);
	if (c > UCHAR_MAX) {
		return ERRTOK;
	}
	ttlval.cval = c;
	return TK_CHAR;
	}

'\\x[0-9a-fA-F]+'	{
	ttlval.cval = (char) getHexEscape((char *)&yytext[3]);
	return TK_CHAR;
	}

'\\.'	{
	ttlval.cval = getCharEscape((char)yytext[2]);
	return TK_CHAR;
	}

'[^'\\]'	{
	ttlval.cval = (char) yytext[1];
	return TK_CHAR;
	}

L'\\[0-7]+'	{
	wchar_t c = getOctalEscape((char *)&yytext[3], 1);
	ttlval.wcval = c;
	return TK_WCHAR;
	}

L'\\x[0-9a-fA-F]+'	{
	ttlval.wcval = getHexEscape((char *)&yytext[4]);
	return TK_WCHAR;
	}

L'\\.'	{
	ttlval.wcval = getCharEscape((char)yytext[3]);
	return TK_WCHAR;
	}

L'[^'\\]+'	{
	wchar_t wc;
	int bytesToEat, bytesEaten;
	bytesToEat = strlen((char*)yytext) - 3;
	bytesEaten = mbtowc(&wc, yytext+2, bytesToEat);
	if (bytesEaten != bytesToEat) {
		return ERRTOK;
	}
	ttlval.wcval = wc;
	return TK_WCHAR;
	}

{digit}+"."{digit}*({exp})?	{
	/* 1. or 1.1 or 1.exp or 1.1exp */
	ttlval.dval = atof((char *)yytext);
	return TK_DOUBLE;
	}

"."{digit}+({exp})?	{
	/* .1 or .1exp */
	ttlval.dval = atof((char *)yytext);
	return TK_DOUBLE;
	}

{digit}+{exp}	{
	/* 1exp */
	ttlval.dval = atof((char *)yytext);
	return TK_DOUBLE;
	}

\"	{
	char *s = getString(1);
	if (!s) {
		return ERRTOK;
	}
	ttlval.sval = s;
	return TK_STRING;
	}

L\"	{
	size_t nwc, nwc2;
	wchar_t *ws;
	char *s = getString(1);
	if (!s) {
		return ERRTOK;
	}
	/*
	 * At this point, s points to a null-terminated string of bytes,
	 * which may include multibyte characters.  (In particular, each \x
	 * escape has been converted to the appropriate multibyte sequence
	 * in s.)  Convert s to an array of wide characters in ws.
	 */
	nwc = mbstowcs(NULL, s, 0);	/* Count multibyte chars */
	if (nwc == (size_t)(-1)) {
		free(s);
		return ERRTOK;
	}
	ws = (wchar_t*) malloc((nwc+1) * sizeof(wchar_t));
	assert(ws != NULL);
	nwc2 = mbstowcs(ws, s, nwc+1);
	assert(nwc2 == nwc);
	free(s);
	ttlval.wsval = ws;
	return TK_WSTRING;
	}

format[ \t\r]*\n	{
	/* Format specification at end of template */
	char *s;
	pc->pc_lineno++;	/* since we've eaten the \n after "format" */
	s = getString(0);
	if (!s) {
		return ERRTOK;
	}
	ttlval.sval = s;
	return TK_FORMAT;
	}

format[ \t]+string	{ return TK_FORMAT_STRING; }

format[ \t]+wstring	{ return TK_FORMAT_WSTRING; }

[ \t\r]	;	/* Skip white space. */

\0	;	/* Skip the terminating null, if it was not eaten with the TK_FORMAT string. */

\n	{ pc->pc_lineno++; }

^[ \t]*\#[ \t]+[0-9]+[ \t]+\"	{
	processLineDirective();
	}

"/*"	{
	if (skipComment() < 0) {
		return ERRTOK;
	}
	}

.	{
	/* Pass through any other characters. */
	return yytext[0];
	}
%%

static YY_BUFFER_STATE bufState;

void
_evlTmplReinitLex(const char *parseMe)
{
	/* Include a null character at the end. */
	bufState = yy_scan_bytes(parseMe, strlen(parseMe)+1);
	inputFromFile = 0;
	pc = _evlTmplGetParserContext();
}

void
_evlTmplEndLex()
{
	yy_delete_buffer(bufState);
}

/*
 * Collect the 1-to-3-digit octal number following a \.
 * If isCharConst is true, we're accumulating a char constant, so we flag
 * an error if there are excess digits.
 * Returns value > UCHAR_MAX if there's an error.
 */
static int
getOctalEscape(const char *digits, int isCharConst)
{
	int n = 0;
	int nDigits = 0;
	const char *c;
	for (c = digits; '0' <= *c && *c <= '7'; c++) {
		n *= 8;
		n += *c - '0';
		if (++nDigits == 3 && !isCharConst) {
			return n;
		} else if (nDigits > 3 && isCharConst) {
			return UCHAR_MAX+1;
		}
	}
	return n;
}

static wchar_t
getHexEscape(const char *digits)
{
	int n = 0;
	const char *c;
	for (c = digits; isxdigit(*c); c++) {
		n *= 16;
		if (isdigit(*c)) {
			n += *c - '0';
		} else {
			n += (10 + tolower(*c) - 'a');
		}
	}
	return n;
}

static int
getCharEscape(char c)
{
	switch (c) {
	case '\'':	return '\'';
	case '\"':	return '\"';
	case '\?':	return '\?';
	case '\\':	return '\\';
	case 'a':	return '\a';
	case 'b':	return '\b';
	case 'f':	return '\f';
	case 'n':	return '\n';
	case 'r':	return '\r';
	case 't':	return '\t';
	case 'v':	return '\v';
	case '\n':	return -1;
	default:	return c;
	}
}

static void
collectOctalDigits(char *digits, char firstDigit)
{
	int nDigits = 1;
	int c;
	digits[0] = firstDigit;

	while (nDigits <= 3) {
		c = input();
		if ('0' <= c && c <= '7') {
			digits[nDigits++] = c;
		} else {
			unput(c);
			break;
		}
	}
	digits[nDigits] = '\0';
}

static int
collectHexDigits(char *digits, int maxDigits)
{
	int nDigits = 0;
	int c;

	while (nDigits <= maxDigits) {
		c = input();
		if (isxdigit(c)) {
			digits[nDigits++] = c;
		} else {
			unput(c);
			break;
		}
	}
	digits[nDigits] = '\0';
	return nDigits;
}

/*
 * We have eaten the leading " in a quoted string.  Collect the characters
 * of the string into strbuf, and then return a strdup-ed copy of the string.
 * We end after eating the terminating ".
 */
static char *
getString(int quoted) {
	int nc = 0;
	int c;
#define MAXSTRLEN (10*1024)
#define STRSLOP 10	/* e.g., to handle a multibyte character at the end */
	char strbuf[MAXSTRLEN+STRSLOP];
	int endOfString = (quoted ? '\"' : MYEOF);

	for (;;) {
		c = input();
		if (c == endOfString) {
			break;
		} else if (c == MYEOF) {
			/* EOF in middle of quoted string. */
			return 0;
		} else if (c == '\\') {
			/* Collect and decode the escape sequence. */
			c = input();
			if (c == MYEOF) {
				/* End of input */
				if (quoted) {
					return 0;
				} else {
					/* Allow \ as the last character. */
					strbuf[nc++] = '\\';
					break;
				}
			} else if ('0' <= c && c <= '7') {
				char digits[3+1];
				collectOctalDigits(digits, c);
				strbuf[nc++] = getOctalEscape(digits, 0);
			} else if (c == 'x') {
				char digits[8+1];
				wchar_t wc;
				int nBytes;
				int nDigits = collectHexDigits(digits, 8);
				if (nDigits == 0) {
					/* \x, no digits: undefined in ANSI C */
					strbuf[nc++] = 'x';
				} else {
					wc = getHexEscape(digits);
					nBytes = wctomb(&strbuf[nc], wc);
					if (nBytes >= 1) {
						nc += nBytes;
					} else {
						/* Probably no locale set. */
						strbuf[nc++] = (char) wc;
					}
				}
			} else {
				int ce = getCharEscape(c);
				/* Elide escaped newlines (ce == -1). */
				if (ce == -1) {
					pc->pc_lineno++;
				} else {
					strbuf[nc++] = ce;
				}
			}
		} else {
			if (c == '\n') {
				pc->pc_lineno++;
			}
			strbuf[nc++] = c;
		}

		if (nc > MAXSTRLEN) {
			return 0;
		}
	}

	strbuf[nc] = '\0';
	return strdup(strbuf);
}

/*
 * We have already eaten the leading / and *.  Skip past the trailing * and /.
 */
static int
skipComment()
{
	int c;
	while ((c = input()) != MYEOF) {
		if (c == '\n') {
			pc->pc_lineno++;
		}

		/* Correctly handle multiple *s followed by /. */
		while (c == '*') {
			c = input();
			if (c == '/') {
				/* End of comment */
				return 0;
			} else if (c == MYEOF) {
				/* EOF after a '*'. */
				return -1;
			} else if (c == '\n') {
				pc->pc_lineno++;
			}
		}
	}

	/* End of file */
	return -1;
}

/*
 * This function processes the '# nnn "filename"' directive.  yytext contains
 * '# nnn "', where nnn is a sequence of digits.  Eat the rest
 * of the string and the newline that follows it, and set the line number and
 * pathname in the parser context accordingly.
 */
static void
processLineDirective()
{
	char *s = (char*) yytext;
	int lineno;
	char *path;
	int c;

	while (!isdigit(*s)) {
		s++;
	}
	lineno = (int) strtoul(s, NULL, 0);
	path = getString(1);
	if (!path) {
		return;
	}
	if (pc->pc_pathname == NULL || strcmp(path, pc->pc_pathname) != 0) {
		free((char*)pc->pc_pathname);
		pc->pc_pathname = path;
	}
	pc->pc_lineno = lineno;

	/*
	 * Eat the trailing newline and any extraneous characters preceding it.
	 * For example, in response to #include directives, cpp can generate
	 * directives like
	 *	# 28 "foo.h" 2 3
	 * We just ignore the " 2 3".
	 */
	do {
		c = input();
	} while (c != '\n' && c != MYEOF);
}
