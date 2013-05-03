/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2001
 *
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
 *
 */

#include <string.h>
#include <limits.h>
#include <ctype.h>

/*
 * Auxiliary functions that can be used by scanners to tokenize C-like input.
 */

/*
 * Collect the 1-to-3-digit octal number following a \.
 * If isCharConst is true, we're accumulating a char constant, so we flag
 * an error if there are excess digits.
 * Returns value > UCHAR_MAX if there's an error.
 */
int
lxGetOctalEscape(const char *digits, int isCharConst)
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

int
lxGetHexEscape(const char *digits)
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

int
lxGetCharEscape(char c)
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
collectOctalDigits(char *digits, char firstDigit, int(*input)(),
	void(*unput)(int))
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
collectHexDigits(char *digits, int maxDigits, int(*input)(), void(*unput)(int))
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
 *
 * The 'quoted' arg should be 1 if the string delimiter is a ", or 0 if the
 * string is terminated by EOF (as with the final section of a jimk-style
 * record template).
 */
char *
lxGetString(int quoted, int(*input)(), void(*unput)(int), int *lxLineNumber)
{
	int nc = 0;
	int c;
#define MAXSTRLEN (10*1024)
	char strbuf[MAXSTRLEN+1];
	int endOfString = (quoted ? '\"' : 0);

	for (;;) {
		c = input();
		if (c == endOfString) {
			break;
		} else if (c == 0) {
			/* EOF in middle of quoted string. */
			return 0;
		} else if (c == '\\') {
			/* Collect and decode the escape sequence. */
			c = input();
			if (c == 0) {
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
				collectOctalDigits(digits, c, input, unput);
				strbuf[nc++] = lxGetOctalEscape(digits, 0);
			} else if (c == 'x') {
				char digits[8+1];
				int nDigits = collectHexDigits(digits, 8,
					input, unput);
				if (nDigits == 0) {
					/* \x, no digits: undefined in ANSI C */
					strbuf[nc++] = 'x';
				} else {
					strbuf[nc++] = lxGetHexEscape(digits);
				}
			} else {
				int ce = lxGetCharEscape(c);
				/* Elide escaped newlines (ce == -1). */
				if (ce == -1) {
					(*lxLineNumber)++;
				} else {
					strbuf[nc++] = ce;
				}
			}
		} else {
			strbuf[nc++] = c;
		}

		if (nc > MAXSTRLEN) {
			return 0;
		}
	}

	strbuf[nc] = '\0';
	return strdup(strbuf);
}
