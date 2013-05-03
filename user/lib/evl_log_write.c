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
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include <ctype.h>
#include <sys/klog.h>

#include "evlog.h"
#include "evl_util.h"
#include "evl_template.h"

typedef tmpl_base_type_t base_type_t;
extern tmpl_base_type_t _evlGetTypeFromConversion(struct evl_parsed_format *pf,
	int promote, int signedOnly);

struct att_type_info {
        base_type_t     at_type;    /* TY_INT for "int" or "int[]" or "2*int" */
        int             at_nelements;   /* 5 for "5*int */
        int             at_array;       /* 1 (true) for "int[]" */
};

static base_type_t
get_type_by_name(const char *name)
{
        base_type_t i;
	tmpl_type_info_t *tinfo = _evlTmplTypeInfo;
        for (i = 0; tinfo->ti_name; i++, tinfo++) {
		if (!tinfo->ti_isScalar && i != TY_STRING && i != TY_WSTRING) {
			/* Skip over template pseudo-types such as TY_LIST. */
			continue;
		}
                if (!strcmp(name, tinfo->ti_name)) {
                        return i;
                }
        }
        return TY_NONE;
}

/*
 * att_type should be a type spec such as "int", "int[]", or "5*int".  Parse it
 * and fill in *ti accordingly.  Returns 0 on success, -1 on failure.
 */
static int
parse_att_type(const char *att_type, struct att_type_info *ti)
{
        const char *s, *left_bracket;
        const char *type_name;
#define MAX_TYPE_NAME_LEN 20
        char name_buf[MAX_TYPE_NAME_LEN+1];

        if (isdigit(att_type[0])) {
                /* "5*int" */
                ti->at_nelements = (int) strtoul(att_type, (char**) &s, 10);
                if (*s != '*') {
                        return -1;
                }
                type_name = s+1;
                ti->at_array = 0;
        } else if ((left_bracket = strchr(att_type, '[')) != NULL) {
                /* int[] */
                size_t name_len;
                ti->at_array = 1;
                ti->at_nelements = 0;
                if (0 != strcmp(left_bracket, "[]")) {
                        return -1;
                }
                /* Copy the name to name_buf and point type_name at it. */
                type_name = name_buf;
                name_len = left_bracket - att_type;
                if (name_len == 0 || name_len > MAX_TYPE_NAME_LEN) {
                        return -1;
                }
                (void) memcpy(name_buf, att_type, name_len);
                name_buf[name_len] = '\0';
        } else {
                /* "int" */
                type_name = att_type;
                ti->at_array = 0;
                ti->at_nelements = 1;
        }
        ti->at_type = get_type_by_name(type_name);
        return (ti->at_type == TY_NONE ? -1 : 0);
}

static size_t
memcpyex(void *dest, const void *src, size_t n, size_t *buflen)
{
        size_t nb = 0;
	size_t rlen = *buflen + n;
	if (rlen > POSIX_LOG_ENTRY_MAXLEN) {
		if (*buflen <= POSIX_LOG_ENTRY_MAXLEN) {
            		nb = POSIX_LOG_ENTRY_MAXLEN - *buflen;
	    	} else {
			return 0;
		}
        } else {
		nb = n;
	}
	/*
	 * Upon return, *buflen can be greater than the MAX_RECORD_SIZE
	 * even though we don't write past the end. Hence, posix_log_write
	 * can determine the whether the record is truncated or not.
	 */
        *buflen = rlen;
	memcpy(dest, src, nb);

        return nb;
}

/*
 * Pack a null-terminated string into the data buffer.  If there's room
 * for at least one byte, but not for the whole string, we copy in what
 * we can and null-terminate it.
 * param d	destination: The string will be copied to here.
 * param s	source: The string to be copied.
 * param reclen	points to the record length that is being accumulated by
 *		the caller.  It is updated to contain what the new
 *		record length would be if the buffer were sufficiently big.
 * return	the destination pointer, updated to point past the null
 *		at the end of the string we just added
 */
static char *
packString(char *d, const char *s, size_t *reclen)
{
	size_t slen = strlen(s) + 1;
	int bytesLeft = POSIX_LOG_ENTRY_MAXLEN - *reclen;
	*reclen += slen;
	if (slen <= bytesLeft) {
		memcpy(d, s, slen);
		d += slen;
	} else if (bytesLeft > 0) {
		/* Copy the partial string */
		memcpy(d, s, bytesLeft-1);
		d[bytesLeft-1] = '\0';
		d += bytesLeft;
	}
	return d;
}

/*
 * Like packString, except we pack a wide string and don't try to
 * null-terminate it if it overflows.
 */
static char *
packWstring(char *d, wchar_t *s, size_t *reclen)
{
	size_t slen = (wcslen(s) + 1) * sizeof(wchar_t);
	d += memcpyex(d, (void *)s, slen, reclen);
	return d;
}

/*
 * COPYARGS copies n args of type lt (little type) from the stack using
 * memcpyex.  bt (big type) is the type of the arg as it appears on the stack.
 */
#define COPYARGS(lt,bt) \
{ \
	while(n-- > 0) { \
		lt v=(lt)va_arg(args,bt); \
		d+=memcpyex(d,&v,sizeof(lt), &reclen); \
	} \
}

/*
 * Packing attributes data onto a data buffer
 *
 * param        args - the variable arguments list.
 * param        databuf - the event record buffer 
 * param        bufsz - Returns the size of the buffer copied.
 *
 * return       0 for Success
 *		-1 for Fail 
 */

int
copy_attr_data(va_list args, char *databuf, size_t *bufsz)
{
        char *att_type;
		size_t reclen = 0;
        char *d = databuf;                      /* next data goes here */

        while ((att_type = va_arg(args, char*))
            && 0 != strcmp(att_type, "endofdata")) {
                struct att_type_info ti;
                if (parse_att_type(att_type, &ti) == -1) {
                        return -1;
                }
                if (ti.at_array) {
                        char *array, *a;
                        size_t size = _evlTmplTypeInfo[ti.at_type].ti_size;
                        int n;

                        /* Next arg is the array size. */
                        n = va_arg(args, int);
                        /* Next arg is the array address. */
                        array = (char*) va_arg(args, void*);

                        switch (ti.at_type) {
			case TY_STRING:
			    {
				/* array points to an array of char* */
				char **sarray = (char**)array;
				int i;
				for (i = 0; i < n; i++) {
					d = packString(d, sarray[i], &reclen);
				}
				break;
			    }
			case TY_WSTRING:
			    {
				/* array points to an array of wchar_t* */
				wchar_t **warray = (wchar_t**)array;
				int i;
				for (i = 0; i < n; i++) {
					d = packWstring(d, warray[i], &reclen);
				}
			    	break;
			    }
			default:
                        	for (a = array; n > 0; a += size, n--) {
                                	d += memcpyex(d, a, size, &reclen);
                        	}
				break;
                        }
                } else {
                        /*
                         * One or more args of the same type.
                         */
                        int n = ti.at_nelements;

                        switch (ti.at_type) {
                        case TY_CHAR:
                        case TY_UCHAR:
                                COPYARGS(char, int)
                                break;
                        case TY_SHORT:
                        case TY_USHORT:
                                COPYARGS(short, int)
                                break;
                        case TY_INT:
                        case TY_UINT:
                                COPYARGS(int, int)
                                break;
                        case TY_LONG:
                        case TY_ULONG:
                                COPYARGS(long, long)
                                break;
                        case TY_LONGLONG:
                        case TY_ULONGLONG:
                                COPYARGS(long long, long long)
                                break;
                        case TY_ADDRESS:
                                COPYARGS(void*, void*)
                                break;
                        case TY_FLOAT:
                                COPYARGS(float, double)
                                break;
                        case TY_DOUBLE:
                                COPYARGS(double, double)
                                break;
                        case TY_LDOUBLE:
                                COPYARGS(long double, long double)
                                break;
                        case TY_STRING:
                            {
                                char *s;
                                while (n-- > 0) {
                                        s = (char *) va_arg(args, char*);
					d = packString(d, s, &reclen);
                                }
                                break;
                            }
                        case TY_WCHAR:
                                COPYARGS(wchar_t, wchar_t)
                                break;
                        case TY_WSTRING:
                            {
                                wchar_t *s;
                                while (n-- > 0) {
                                        s = (wchar_t *) va_arg(args, wchar_t*);
					d = packWstring(d, s, &reclen);
                                }
                            } /* end of TY_WSTRING case */
                        default: /* keep gcc happy */ ;
                        } /* end of switch */
                } /* not array */
        } /* next att_type */

	*bufsz = reclen;
        return 0;
}

int
evl_log_vwrite(posix_log_facility_t facility, int event_type,
	posix_log_severity_t severity, unsigned int flags, va_list args)
{
	char evl_buf[POSIX_LOG_ENTRY_MAXLEN];
	size_t log_data_len;
	
	if (copy_attr_data(args, (char *)evl_buf, &log_data_len) == -1) {
		return EINVAL;
	}

	return(posix_log_write(facility, event_type, severity, evl_buf, 
		log_data_len, POSIX_LOG_BINARY, flags)); 
}

int
evl_log_write(posix_log_facility_t facility, int event_type,
	posix_log_severity_t severity, unsigned int flags, ...)
{
	va_list args;
	int ret;
	va_start(args, flags);
	ret = evl_log_vwrite(facility, event_type, severity, flags, args);
	va_end(args);
	return ret;
}

/* Utility functions for _evlPackCmdArgs() */

/* Decode unsigned int */
static int
decodeUint(const char *val, int size, unsigned long long *u)
{
	int error = 0;
	char *c = NULL;
	unsigned long long uval;
	
	errno = 0;
	uval = strtoull(val, &c, 0);
	if (errno != 0 || *c != '\0') {
		/* junk beyond end of number */
		return EINVAL;
	}

	if (size == sizeof(unsigned char)) {
		error = (uval > UCHAR_MAX);
	} else if (size == sizeof(unsigned short)) {
		error = (uval > USHRT_MAX);
	} else if (size == sizeof(unsigned int)) {
		error = (uval > UINT_MAX);
	} else if (size == sizeof(unsigned long)) {
		error = (uval > ULONG_MAX);
	}
	if (error) {
		return ERANGE;
	}
	*u = uval;
	return 0;
}

/* Decode signed int */
static int
decodeSint(const char *val, int size, long long *v)
{
	int error = 0;
	char *c = NULL;
	long long sval;
	
	errno = 0;
	sval = strtoll(val, &c, 0);
	if (errno != 0 || *c != '\0') {
		/* junk beyond end of number */
		return EINVAL;
	}

	if (size == sizeof(char)) {
		/*
		 * Even on architectures with char = unsigned, allow negative
		 * values, since they can't explicitly specify signed char.
		 */
		error = (sval < SCHAR_MIN || sval > CHAR_MAX);
	} else if (size == sizeof(short)) {
		error = (sval < SHRT_MIN || sval > SHRT_MAX);
	} else if (size == sizeof(int)) {
		error = (sval < INT_MIN || sval > INT_MAX);
	} else if (size == sizeof(long)) {
		error = (sval < LONG_MIN || sval > LONG_MAX);
	}
	if (error) {
		return ERANGE;
	}
	*v = sval;
	return 0;
}

static int
decodeReal(const char *val, base_type_t type, long double *v)
{
	char garbage;
	char *c = NULL;
	long double ld;

	errno = 0;
	if (type == TY_FLOAT) {
		float f;
		if (sscanf(val, "%f%c", &f, &garbage) != 1) {
			return EINVAL;
		}
		ld = f;
	} else if (type == TY_DOUBLE) {
		double d = strtod(val, &c);
		if (*c != '\0') {
			return EINVAL;
		}
		ld = d;
	} else {
		extern long double strtold(const char *s, char **c);
		assert(type == TY_LDOUBLE);
		ld = strtold(val, &c);
		if (*c != '\0') {
			return EINVAL;
		}
	}
	if (errno != 0) {
		return ERANGE;
	}
	*v = ld;
	return 0;
}

static void
reportBadVal(const char *val, int status, char *errbuf, size_t ebufsz)
{
	switch (status) {
	case EINVAL:
		snprintf(errbuf, ebufsz, "Unrecognized value: %s", val);
		break;
	case ERANGE:
		snprintf(errbuf, ebufsz, "Value out of range: %s", val);
		break;
	}
}

struct decodeContext {
	char	*d;
	size_t	reclen;
	int	nelements;
	char	*errbuf;
	size_t	ebufsz;
};

static void
initDecodeContext(struct decodeContext *dc, char *d, size_t reclen,
	char *errbuf, size_t ebufsz)
{
	dc->d = d;
	dc->reclen = reclen;
	dc->errbuf = errbuf;
	dc->ebufsz = ebufsz;
	dc->nelements = 0;
}

static inline void
dcMemcpy(struct decodeContext *dc, const void *src, size_t nbytes)
{
	dc->d += memcpyex(dc->d, src, nbytes, &dc->reclen);
}

static inline void
dcPackString(struct decodeContext *dc, const char *s)
{
	dc->d = packString(dc->d, s, &dc->reclen);
}

static inline void
dcPackWstring(struct decodeContext *dc, wchar_t *wstring)
{
	dc->d = packWstring(dc->d, wstring, &dc->reclen);
}

#define CHECKVAL if (status!=0) { reportBadVal(val, status, dc->errbuf, dc->ebufsz); return status; }

#define COPYVAL(ty,lv) {ty v=(ty)lv; dcMemcpy(dc,&v,sizeof(ty));}

int
decodeArg(const char *val, tmpl_base_type_t type, struct decodeContext *dc)
{
	long long sval;
	unsigned long long uval;
	long double fval;
	int status = EINVAL;

	if (!val) {
		if (dc->nelements == 1) {
			snprintf(dc->errbuf, dc->ebufsz, "Missing value");
		} else {
			snprintf(dc->errbuf, dc->ebufsz, 
				"Missing value(s) in %d-element list",
				dc->nelements);
		}
		return EINVAL;
	}

	switch (type) {
	case TY_CHAR:
		status = decodeSint(val, sizeof(char), &sval);
		if (status == EINVAL && strlen(val) == 1) {
			sval = val[0];
			status = 0;
		}
		CHECKVAL
		COPYVAL(char, sval)
		break;
	case TY_UCHAR:
		status = decodeUint(val, sizeof(char), &uval);
		if (status == EINVAL && strlen(val) == 1) {
			uval = val[0];
			status = 0;
		}
		CHECKVAL
		COPYVAL(unsigned char, uval)
		break;
	case TY_SHORT:
		status = decodeSint(val, sizeof(short), &sval);
		CHECKVAL
		COPYVAL(short, sval)
		break;
	case TY_USHORT:
		status = decodeUint(val, sizeof(short), &uval);
		CHECKVAL
		COPYVAL(unsigned short, uval)
		break;
	case TY_INT:
		status = decodeSint(val, sizeof(int), &sval);
		CHECKVAL
		COPYVAL(int, sval)
		break;
	case TY_UINT:
		status = decodeUint(val, sizeof(int), &uval);
		CHECKVAL
		COPYVAL(unsigned int, uval)
		break;
	case TY_LONG:
		status = decodeSint(val, sizeof(long), &sval);
		CHECKVAL
		COPYVAL(long, sval)
		break;
	case TY_ULONG:
		status = decodeUint(val, sizeof(long), &uval);
		CHECKVAL
		COPYVAL(unsigned long, uval)
		break;
	case TY_LONGLONG:
		status = decodeSint(val, sizeof(long long), &sval);
		CHECKVAL
		COPYVAL(long long, sval)
		break;
	case TY_ULONGLONG:
		status = decodeUint(val, sizeof(long long), &uval);
		CHECKVAL
		COPYVAL(unsigned long long, uval)
		break;
	case TY_ADDRESS:
	    {
		/*
		 * Can't use COPYVAL here because cast of
		 * unsigned long long to void* yields a
		 * warning on some architectures.
		 */
		void *v;
		unsigned long ul;
		status = decodeUint(val, sizeof(void*), &uval);
		CHECKVAL
		ul = (unsigned long) uval;
		v = (void*) ul;
		dcMemcpy(dc, &v, sizeof(void*));
		break;
	    }
	case TY_FLOAT:
		status = decodeReal(val, TY_FLOAT, &fval);
		CHECKVAL
		COPYVAL(float, fval)
		break;
	case TY_DOUBLE:
		status = decodeReal(val, TY_DOUBLE, &fval);
		CHECKVAL
		COPYVAL(double, fval)
		break;
	case TY_LDOUBLE:
		status = decodeReal(val, TY_LDOUBLE, &fval);
		CHECKVAL
		COPYVAL(long double, fval)
		break;
	case TY_STRING:
		dcPackString(dc, val);
		status = 0;
		break;
	case TY_WCHAR:
		status = decodeSint(val, sizeof(wchar_t), &sval);
		if (status == EINVAL && strlen(val) == 1) {
			sval = val[0];
			status = 0;
		}
		CHECKVAL
		COPYVAL(wchar_t, sval)
		break;
	case TY_WSTRING:
	    {
		/* Convert val to a wide-char string. */
		wchar_t *wstring;
		size_t nc = strlen(val) + 1;
		wstring = (wchar_t*) malloc(nc*sizeof(wchar_t));
		assert(wstring != NULL);
		(void) mbstowcs(wstring, val, nc);
		dcPackWstring(dc, wstring);
		free(wstring);
		status = 0;
		break;
	    }
        default: /* keep gcc happy */;
	}

	return status;
}

/*
 * Analogous to copy_attr_data(), except that the contents of the value
 * list are an array of command-line arguments, argv.  argc is the total
 * length of the arg list, as passed to main().  *iargc points to the
 * first type-name in the list.
 *
 * databuf is assumed to be a buffer of size POSIX_LOG_ENTRY_MAXLEN.
 * Upon return, *datasz is set to the total number of bytes packed into
 * databuf, and *iargc points to the next arg after the end of the list.
 * If we would have overflowed the buffer, *datasz will exceed
 * POSIX_LOG_ENTRY_MAXLEN, and we return EMSGSIZE.
 *
 * The end of the list is indicated by one of the following:
 * - the end of the argv array (argv[i] == NULL)
 * - the string "endofdata" where a type name is expected
 * - a string that starts with "-" where a type name is expected.  This is
 * taken to be a new command-line option.
 *
 * Returns EMSGSIZE (see above) or EINVAL on error, or 0 on success.
 * If there is an error, an error message is written to errbuf, whose size
 * is assumed to be ebufsz.
 */
int
_evlPackCmdArgs(int argc, const char **argv, int *iarg, char *databuf,
	size_t *datasz, char *errbuf, size_t ebufsz)
{
	int i, j;
	unsigned long long uval;
	char discarded_err_msg[1];
	struct decodeContext dc;

	if (!errbuf) {
		errbuf = discarded_err_msg;
		ebufsz = 1;
	}

	initDecodeContext(&dc, databuf, 0, errbuf, ebufsz);

	for (i = *iarg; i < argc; ) {
                struct att_type_info ti;

		if (argv[i][0] == '-') {
			break;
		}
		if (!strcmp(argv[i], "endofdata")) {
			i++;
			break;
		}
                if (parse_att_type(argv[i], &ti) == -1) {
                        snprintf(errbuf, ebufsz, "Unrecognized type: %s",
				argv[i]);
			return EINVAL;
                }

		i++;
                if (ti.at_array) {
			/* Here "int[] 3" is same as "3*int" */
			if (decodeUint(argv[i], sizeof(int), &uval) != 0) {
				snprintf(errbuf, ebufsz,
					"Bad array dimension: %s", argv[i]);
				return EINVAL;
			}
			i++;
			dc.nelements = (int) uval;
                } else {
                        dc.nelements = ti.at_nelements;
		}
		for (j = 1; j <= dc.nelements; j++, i++) {
			if (decodeArg(argv[i], ti.at_type, &dc) != 0) {
				return EINVAL;
			}
                }
	} /* end of this type + val(s) */

	*iarg = i;
	*datasz = dc.reclen;
	if (dc.reclen > POSIX_LOG_ENTRY_MAXLEN) {
		snprintf(errbuf, ebufsz, "Record would exceed %d bytes.",
			POSIX_LOG_ENTRY_MAXLEN);
		return EMSGSIZE;
	}
	return 0;
}

/*
 * Pack the specified array into the event record at the point pointed
 * to by d.  *reclen is the current length of the record's variable data,
 * and is updated accordingly.  Returns the new value of d.
 */
char *
packArray(char *d, size_t *reclen, char *array, int dim, char *delim,
	tmpl_base_type_t type)
{
	size_t size;
	int i;

	d += memcpyex(d, &dim, sizeof(dim), reclen);
	d = packString(d, delim, reclen);

	switch (type) {
	case TY_STRING:
	    {
		char **s = (char**) array;
		for (i = 0; i < dim; i++) {
			d = packString(d, s[i], reclen);
		}
		break;
	    }
	case TY_WSTRING:
	    {
		wchar_t **s = (wchar_t**) array;
		for (i = 0; i < dim; i++) {
			d = packWstring(d, s[i], reclen);
		}
		break;
	    }
	default:
		size = _evlTmplTypeInfo[type].ti_size;
		assert(size != 0);
		d += memcpyex(d, array, dim*size, reclen);
		break;
	}
	return d;
}

/*
 * Analogous to copy_attr_data(), except that the contents of the value
 * list (args) are not self-describing, but rather specified by the
 * printf-style format string, format.  Also, this function can handle
 * a buffer that's already partly filled.
 *
 * databuf is assumed to be a buffer of size POSIX_LOG_ENTRY_MAXLEN.
 * *datasz is the number of bytes already in the buffer.  Append the format
 * string to databuf, and then pack the values in after it.  Upon return,
 * *datasz is set to the total number of bytes packed into databuf.
 *
 * Returns EINVAL if there's a printf conversion spec that we don't
 * know how to handle (e.g., %Lc).  If we would overflow the buffer,
 * *datasz is set to a value greater than POSIX_LOG_ENTRY_MAXLEN, we pack
 * as much into the buffer as we can, and we return EMSGSIZE.  (This is not
 * exactly consistent with copy_attr_data(), which returns 0 in this
 * situation.)
 */
int
evl_pack_format_and_args(const char *format, va_list args, char *databuf,
	size_t *datasz)
{
	/*
	 * Note: Since we use the COPYARGS macro, don't rename args, d,
	 * or reclen.
	 */
	char *d;		/* Next arg stored here. */
	size_t reclen = *datasz;
	char *formatDup;	/* Copy of format to scribble on */
	evl_list_t *parsedFmt;
	evl_listnode_t *head, *end, *p;
	int status = 0;
	int argsz = 0;
	size_t argszGoesHere, argsStartHere;

	if (!format || !databuf) {
		return EINVAL;
	}

	formatDup = strdup(format);
	assert(formatDup != NULL);
	parsedFmt = _evlParsePrintfFormat(formatDup, 0, &status);
	if (status != 0) {
		free(formatDup);
		return EINVAL;
	}

	/*
	 * OK so far.  Pour in the attributes:
	 * - first the format string
	 * - next, a placeholder for the size of the "args" attributes.
	 *	We will go back and plug this value in after we've packed
	 *	in the args.
	 * - finally, the args
	 */
	d = databuf + reclen;
	d = packString(d, format, &reclen);

	argszGoesHere = reclen;
	d += memcpyex(d, &argsz, sizeof(argsz), &reclen);
	argsStartHere = reclen;

	head = parsedFmt;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		int n = 1;	/* COPYARGS gets 1 arg at a time. */
		evl_fmt_segment_t *seg = (evl_fmt_segment_t*) p->li_data;

		assert(seg->fs_type == EVL_FS_PRINTFCONV);
		if (seg->u.fs_conversion->fm_array) {
			char *array, *delim;
			int dim;
			tmpl_base_type_t type;

			array = (char*) va_arg(args, char*);
			dim = (int) va_arg(args, int);
			delim = (char*) va_arg(args, char*);
			type = _evlGetTypeFromConversion(seg->u.fs_conversion,
				0, 1);
			d = packArray(d, &reclen, array, dim, delim, type);
			continue;
		}
		switch (_evlGetTypeFromConversion(seg->u.fs_conversion, 1, 1)) {
		case TY_INT:
			COPYARGS(int, int)
			break;
		case TY_LONG:
			COPYARGS(long, long)
			break;
		case TY_LONGLONG:
			COPYARGS(long long, long long)
			break;
		case TY_ADDRESS:
			COPYARGS(void*, void*)
			break;
		case TY_DOUBLE:
			COPYARGS(double, double)
			break;
		case TY_LDOUBLE:
			COPYARGS(long double, long double)
			break;
		case TY_STRING:
		    {
			char *s = (char *) va_arg(args, char*);
			d = packString(d, s, &reclen);
			break;
		    }
		case TY_WCHAR:
			COPYARGS(wchar_t, wchar_t)
			break;
		case TY_WSTRING:
		    {
			wchar_t *s = (wchar_t *) va_arg(args, wchar_t*);
			d = packWstring(d, s, &reclen);
			break;
		    }
		case TY_NONE:
			status = EINVAL;
			goto done;
		default:
			assert(0);
		}
	}

	if (argszGoesHere <= (POSIX_LOG_ENTRY_MAXLEN - sizeof(argsz))) {
		argsz = reclen - argsStartHere;
		memcpy(databuf + argszGoesHere, &argsz, sizeof(argsz));
	}

	*datasz = reclen;
	if (reclen > POSIX_LOG_ENTRY_MAXLEN) {
		status = EMSGSIZE;
	}

done:
	free(formatDup);
	_evlFreeParsedFormat(parsedFmt);
	return status;
}

/*
 * Sort of a cross between _evlPackCmdArgs() and evl_pack_format_and_args().
 * The attribute types are dictated by the specified format string, as in
 * evl_pack_format_and_args(); but the attribute values are decoded from
 * an array of command-line arguments, as in _evlPackCmdArgs().
 *
 * Except format, all function args are as with _evlPackCmdArgs().
 * *iarg starts at the first arg beyond the format string.
 */
extern int
_evlPackCmdArgsPerFormat(const char *format, int argc, const char **argv,
	int *iarg, char *databuf, size_t *datasz, char *errbuf, size_t ebufsz)
{
	int i = *iarg, j;
	char discarded_err_msg[1];
	struct decodeContext dc;
	char *formatDup;	/* Copy of format to scribble on */
	evl_list_t *parsedFmt;
	evl_listnode_t *head, *end, *p;
	int argsz = 0;
	size_t argszGoesHere, argsStartHere;
	int status;

	if (!errbuf) {
		errbuf = discarded_err_msg;
		ebufsz = 1;
	}

	initDecodeContext(&dc, databuf, 0, errbuf, ebufsz);

	formatDup = strdup(format);
	assert(formatDup != NULL);
	parsedFmt = _evlParsePrintfFormat(formatDup, 0, &status);
	if (status != 0) {
		free(formatDup);
		snprintf(errbuf, ebufsz, "Invalid format string.");
		return EINVAL;
	}

	dcPackString(&dc, format);

	argszGoesHere = dc.reclen;
	dcMemcpy(&dc, &argsz, sizeof(argsz));
	argsStartHere = dc.reclen;

	head = parsedFmt;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		int promote;	/* Promote short to int, etc? */
		tmpl_base_type_t type;
		evl_fmt_segment_t *seg = (evl_fmt_segment_t*) p->li_data;

		assert(seg->fs_type == EVL_FS_PRINTFCONV);
		if (seg->u.fs_conversion->fm_array) {
			/* Expect dimension n, delimiter, and n values. */
			long long sval;
			promote = 0;
			if (!argv[i]) {
				snprintf(errbuf, ebufsz,
					"Missing array dimension");
				return EINVAL;
			}
			if (decodeSint(argv[i], sizeof(int), &sval) != 0
			    || sval < 0) {
				snprintf(errbuf, ebufsz,
					"Bad array dimension: %s", argv[i]);
				return EINVAL;
			}
			dc.nelements = (int) sval;
			dcMemcpy(&dc, &dc.nelements, sizeof(int));
			i++;

			if (!argv[i]) {
				snprintf(errbuf, ebufsz,
					"Missing array delimiter");
				return EINVAL;
			}
			dcPackString(&dc, argv[i]);
			i++;
		} else {
			promote = 1;
			dc.nelements = 1;
		}
		type = _evlGetTypeFromConversion(seg->u.fs_conversion, promote,
			0);
		for (j = 1; j <= dc.nelements; j++, i++) {
			if (decodeArg(argv[i], type, &dc) != 0) {
				return EINVAL;
			}
                }
	}

	if (argszGoesHere <= (POSIX_LOG_ENTRY_MAXLEN - sizeof(argsz))) {
		argsz = dc.reclen - argsStartHere;
		memcpy(databuf + argszGoesHere, &argsz, sizeof(argsz));
	}

	*iarg = i;
	*datasz = dc.reclen;
	if (dc.reclen > POSIX_LOG_ENTRY_MAXLEN) {
		snprintf(errbuf, ebufsz, "Record would exceed %d bytes.",
			POSIX_LOG_ENTRY_MAXLEN);
		return EMSGSIZE;
	}
	return 0;
}
