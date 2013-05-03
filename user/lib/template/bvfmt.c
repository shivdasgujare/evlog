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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "evl_template.h"

#define semanticError _evlTmplSemanticError

/*
 * s points at the first character past %b or %v.  Take that
 * as the delimiter, and parse the rest of the string into substrings.
 * fmtChar is either 'b' or 'v'.  If ends is not NULL, *ends points
 * to the first character past the end of the format spec upon return.
 * bvFmtAlone is 1 if %b and %v conversion specs cannot be part of a
 * larger format specification -- e.g.,
 * "(x[%I]=%b/.../\n)"
 *
 * NOTE: The substrings are not malloced; they are contained in the
 * original string pointed to by s (and s is perforated with null
 * characters upon return).
 */
static evl_list_t *
parseBorVformat(char *s, int bvFmtAlone, char fmtChar, char **ends)
{
	char delim = *s;
	char *d;
	evl_list_t *list = NULL;
	int nSubstrings;

	if (!ispunct(delim)) {
		semanticError("Illegal delimiter for %%%c format: '%c'",
			fmtChar, delim);
		return NULL;
	}
	s++;
	for (;;) {
		d = strchr(s, delim);
		if (!d) {
			/* No more delimiters. */
			if (ends) {
				*ends = s;
			}
			if (bvFmtAlone && *s != '\0') {
				semanticError("%%%c format does not end with '%c' delimiter",
					fmtChar, delim);
				_evlFreeList(list, 0);
				return NULL;
			}
			break;
		}
		/* Found the next delimiter. */
		*d = '\0';
		list = _evlAppendToList(list, s);
		s = d+1;
	}

	nSubstrings = _evlGetListSize(list);
	if (nSubstrings == 0 || nSubstrings % 2 != 0) {
		semanticError("%%%c format must have an even number of elements",
			fmtChar);
		_evlFreeList(list, 0);
		return NULL;
	}
	return list;
}

tmpl_bitmap_t *
_evlTmplCollectVformat(char *fmt, int vFmtOnly, char **endFmt)
{
	evl_list_t *list;
	int nPairs, i;
	tmpl_bitmap_t *bitMaps = NULL;
	evl_listnode_t *p;

	assert(_evlStartsWith(fmt, "%v"));
	list = parseBorVformat(fmt+2, vFmtOnly, 'v', endFmt);
	if (!list) {
		return NULL;
	}

	nPairs = _evlGetListSize(list) / 2;
	bitMaps = (tmpl_bitmap_t *) malloc((nPairs+1) * sizeof(tmpl_bitmap_t));
	assert(bitMaps != NULL);

	for (i = 0, p = list; i < nPairs; i++) {
		int j;
		char *ends;
		char *s = (char*) p->li_data;
		long val;

		if (s[0] == '\0') {
			semanticError("Missing value in %%v value/name pair");
			goto badFormat;
		}
		val = strtol(s, &ends, 0);
		if (*ends != '\0') {
			semanticError("Bad value in %%v value/name pair: %s", s);
			goto badFormat;
		}
		bitMaps[i].bm_1bits = val;
		bitMaps[i].bm_0bits = ~val;	/* Not really used for %v */
		p = p->li_next;
		bitMaps[i].bm_name = (char*) p->li_data;
		p = p->li_next;

		for (j = 0; j < i; j++) {
			if (bitMaps[j].bm_1bits == val) {
				semanticError("Duplicate value in %%v list: %d\n", val);
				goto badFormat;
			}
		}
	}
	bitMaps[nPairs].bm_name = NULL;
	_evlFreeList(list, 0);
	return bitMaps;

badFormat:
	_evlFreeList(list, 0);
	free(bitMaps);
	return NULL;
}

tmpl_bitmap_t *
_evlTmplCollectBformat(char *fmt, int bFmtOnly, char **endFmt)
{
	evl_list_t *list;
	evl_listnode_t *p;
	tmpl_bitmap_t *bitMaps;
	int nPairs, i, j;

	assert(_evlStartsWith(fmt, "%b"));
	list = parseBorVformat(fmt+2, bFmtOnly, 'b', endFmt);
	if (!list) {
		return NULL;
	}

	nPairs = _evlGetListSize(list) / 2;
	bitMaps = (tmpl_bitmap_t *) malloc((nPairs+1) * sizeof(tmpl_bitmap_t));
	assert(bitMaps != NULL);

	for (i = 0, p = list; i < nPairs; i++) {
		char *bitmap = (char *) p->li_data;
		char *b = bitmap;
		char *ends;
		unsigned long zeroBits, oneBits;

		if (*b == '\0') {
			semanticError("Missing bitmap in %%b format");
			goto badFormat;
		}

		if (*b == '0') {
			b++;
		}
		switch (*b) {
		case 'x':
		case 'X':
			b++;
			oneBits = strtoul(b, &ends, 16);
			if (*ends != '\0' || *b == '\0'
			    || strspn(b, "0123456789abcdefABCDEF") != strlen(b)) {
				semanticError("Bad hex value in %%b format: %s", bitmap);
				goto badFormat;
			}
			zeroBits = 0x0;
			break;
		case 'b':
		case 'B':
			b++;
			if (*b == '\0' || strspn(b, "01xX") != strlen(b)) {
				semanticError("Bad bitmap in %%b format: %s", bitmap);
				goto badFormat;
			}
			zeroBits = 0x0;
			oneBits = 0x0;
			while (*b) {
				zeroBits <<= 1;
				oneBits <<= 1;
				switch (*b) {
				case '0':
					zeroBits |= 0x1;
					break;
				case '1':
					oneBits |= 0x1;
					break;
				case 'x':
				case 'X':
					/* Don't-care bit */
					break;
				}
				b++;
			}
			break;
		default:
			semanticError("Bad bitmap in %%b format: %s", bitmap);
			goto badFormat;
		}
		bitMaps[i].bm_1bits = oneBits;
		bitMaps[i].bm_0bits = zeroBits;

		p = p->li_next;
		bitMaps[i].bm_name = (char*) p->li_data;
		p = p->li_next;

		if (i < nPairs-1 && (oneBits | zeroBits) == 0x0) {
			semanticError("Bitmap for %s matches all values but is not last",
				bitMaps[i].bm_name);
			goto badFormat;
		}

		for (j = 0; j < i; j++) {
			if (oneBits == bitMaps[j].bm_1bits
			    && zeroBits == bitMaps[j].bm_0bits) {
				semanticError("Bitmaps for \"%s\" and \"%s\" are the same",
					bitMaps[j].bm_name, bitMaps[i].bm_name);
				goto badFormat;
			}
		}
	}
	bitMaps[nPairs].bm_name = NULL;
	_evlFreeList(list, 0);
	return bitMaps;

badFormat:
	_evlFreeList(list, 0);
	free(bitMaps);
	return NULL;
}
