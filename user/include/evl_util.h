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

#ifndef _EVLOG_UTIL_H_
#define _EVLOG_UTIL_H_

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The components of a printf-style conversion spec */
struct evl_parsed_format {
	char	fm_flags[10];	/* e.g., "#0" in "%#08x" */
	int	fm_width;	/* e.g., 2 in "%02d" */
	int	fm_precision;	/* e.g., 2 in "%6.2f" */
	char	fm_modifier[3];	/* e.g., "ll" in "lld" */
	char	fm_conversion;	/* e.g., 'f' in "%6.2f" */
	size_t	fm_length;	/* not counting % */
	int	fm_array;	/* flags include '[' */
};

typedef enum evl_fmt_segment_type {
	EVL_FS_STRING,	/* Ordinary text from the template */
	EVL_FS_MEMBER,	/* Member of event record header -- e.g., %uid% */
	EVL_FS_ATTNAME,	/* Name of an as-yet unknown attribute */
	EVL_FS_ATTR,	/* Pointer to non-standard attribute */
	EVL_FS_PRINTFCONV,	/* Pointer to an evl_parsed_format */
	EVL_FS_ERROR	/* Used to indicate a bad attribute spec */
} evl_fmt_segment_type_t;

/* One segment of a parsed format string */
typedef struct evl_fmt_segment {
	evl_fmt_segment_type_t	fs_type;
	char			*fs_userfmt;	/* format specified by user */
	int			fs_stringfmt;	/* 1 for %s */
	union {
		char	*fs_string;	/* EVL_FS_STRING or EVL_FS_ERROR */
		char	*fs_attname;	/* EVL_FS_ATTNAME */
		struct evl_parsed_format *fs_conversion; /* EVL_FS_PRINTFCONV */
	} u;
	union {
		int			fs_member;	/* EVL_FS_MEMBER */
		struct tmpl_attribute	*fs_attribute;	/* EVL_FS_ATTR */
	} u2;
} evl_fmt_segment_t;

/*
 * This object defines a buffer that holds the result of formatting a template
 * or an individual attribute.  fb_next starts at fb_buf, and advances through
 * the buffer as strings are written to it -- usually via bprintf().
 */
typedef struct evl_fmt_buf {
	char	*fb_buf;	/* start of buffer */
	char	*fb_next;	/* next value goes here */
	char	*fb_end;	/* fb_buf + bufsz */
	int	fb_oflow;	/* 1 if we would have written past the end */
	char	fb_dummy[1];	/* used when caller gives us a null buffer */
} evl_fmt_buf_t;

/* format.c */
extern int _evlParseFmtConvSpec(const char *fmt, struct evl_parsed_format *pf);
extern int _evlValidateAttrFmt(const char *fmt, struct evl_parsed_format *pf,
	int maxWidth);
extern evl_fmt_segment_t *_evlAllocFormatSegment();
extern void _evlFreeParsedFormat(evl_list_t *pf);
extern evl_list_t *_evlParseFormat(char *fmt, int allowNonStdAtts,
	char **errMsg);
extern char *_evlAllocateFmtBuf(const evl_list_t *head, size_t *bufsz);
extern void _evlSprintfMember(evl_fmt_buf_t *s, const char *fmt, int member,
	const struct posix_log_entry *entry);
extern evl_list_t *_evlParsePrintfFormat(char *format, int needStrings,
	int *status);

/* fmtbuf.c */
extern evl_fmt_buf_t *_evlMakeFmtBuf(char *buf, size_t bufsz);
extern void _evlFreeFmtBuf(evl_fmt_buf_t *f);
extern void _evlBprintf(evl_fmt_buf_t *f, const char *fmt, ...);
extern void _evlDumpBytesToFmtBuf(const void *buf, size_t nBytes,
	evl_fmt_buf_t *f);

/* facreg.c */
extern posix_log_facility_t _evlGetFacCodeByCIName(const char *name,
	_evlFacilityRegistry *facReg);
extern const char * _evlGetFacNameByCode(posix_log_facility_t facNum,
	_evlFacilityRegistry *facReg);
extern void _evlFreeFacReg(_evlFacilityRegistry *facReg);

#ifdef __cplusplus
}
#endif

#endif /* _EVLOG_UTIL_H_ */
