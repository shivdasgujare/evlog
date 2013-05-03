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
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <errno.h>

#include "config.h"
#include "evlog.h"
#include "evl_util.h"
#include "evl_template.h"

#define dprintf _evlTmplDprintf

extern tmpl_arch_type_info_t _evlTmplArchTypeInfo[7][24];

static int formatRecordFromTemplate(const template_t *template, evl_fmt_buf_t *f);

enum myIZorder {S, SI, IS, Z, ZI, IZ};

static enum myIZorder
getIZorder(const tmpl_attribute_t *att)
{
	const char *izorder = att->ta_format.u.u_izorder;
	enum myIZorder izo;

	if (!strcmp(izorder,"SI")) {
		izo = SI;
	} else if (!strcmp(izorder,"IS")) {
		izo = IS;
	} else if (!strcmp(izorder,"Z")) {
		izo = Z;
	} else if (!strcmp(izorder,"ZI")) {
		izo = ZI;
	} else if (!strcmp(izorder,"IZ")) {
		izo = IZ;
	} else {
		izo = S;
	}
	return izo;
}

/*
 * Returns 1 if att is not a legal candidate for ad-hoc format changes of
 * the form %att:fmt% (e.g., %port:x%).
 */
static int
cantDoAdHocFormat(const tmpl_attribute_t *att)
{
	return (isArray(att) || isStruct(att));
}

/*
 * Create and return a list of format segments from the indicated format string.
 */
evl_list_t *
_evlTmplParseFormat(template_t *t, char *fmtSpec)
{
	char *errorMsg;
	evl_list_t *list;
	evl_listnode_t *head, *end, *p;
	int ok = 1;

	list = _evlParseFormat(fmtSpec, 1, &errorMsg);
	if (list == NULL) {
		_evlTmplSemanticError(errorMsg);
		free(errorMsg);
		return NULL;
	}

	/*
	 * _evlParseFormat() doesn't know from templates, so there's an
	 * EVL_FS_ATTNAME segment for each non-standard attribute mentioned.
	 * Convert each EVL_FS_ATTNAME segment into an EVL_FS_ATTR segment.
	 *
	 * We accept scoped names -- e.g., "%startPoint.x%" if they make
	 * sense, but we leave them as EVL_FS_ATTNAME because EVL_FS_ATTR
	 * segments can't store more than one attribute index.
	 *
	 * Also flag stuff like %arrayatt:d% -- "Format this array as a
	 * decimal integer."
	 *
	 * Also flag any ref to %data%, since that would create an infinite
	 * formatting loop.
	 */
	for (head=list, p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		evl_fmt_segment_t *fs = (evl_fmt_segment_t*) p->li_data;

		if (fs->fs_type == EVL_FS_MEMBER
		    && fs->u2.fs_member == POSIX_LOG_ENTRY_DATA) {
			_evlTmplSemanticError(
"Can't refer to %%data%% attribute in template");
			ok = 0;
		} else if (fs->fs_type == EVL_FS_ATTNAME) {
			tmpl_attribute_t *att = _evlTmplFindAttribute(t,
				fs->u.fs_attname);
			if (att) {
				if (fs->fs_userfmt) {
					if (cantDoAdHocFormat(att)) {
						_evlTmplSemanticError(
"%s: Can't do alternate formats for arrays or structs",
							att->ta_name);
						ok = 0;
					}
					/* TODO: Maybe check fmt vs att type */
				}

				if (! strchr(fs->u.fs_attname, '.')) {
					/* Simple attribute name */
					fs->fs_type = EVL_FS_ATTR;
					fs->u2.fs_attribute = att;
					free(fs->u.fs_attname);
					fs->u.fs_attname = NULL;
				}
			} else {
				_evlTmplSemanticError("Unknown attribute: %s",
					fs->u.fs_attname);
				ok = 0;
			}
		}
	}

	if (ok) {
		return list;
	} else {
		_evlFreeList(list, 0);
		return NULL;
	}
}

#define FORMAT_SCALAR(type, member) _evlBprintf(f, fmt, (type) att->ta_value.member);

static void
castAndFormatAtt(const tmpl_attribute_t *att, const char *fmt, evl_fmt_buf_t *f)
{
	switch (baseType(att)) {
	case TY_CHAR:	FORMAT_SCALAR(signed char, val_long);		break;
	case TY_UCHAR:	FORMAT_SCALAR(unsigned char, val_ulong);	break;
	case TY_SHORT:	FORMAT_SCALAR(short, val_long);			break;
	case TY_USHORT:	FORMAT_SCALAR(unsigned short, val_ulong);	break;
	case TY_INT:	FORMAT_SCALAR(int, val_long);			break;
	case TY_UINT:	FORMAT_SCALAR(unsigned int, val_ulong);		break;
	case TY_LONG:	FORMAT_SCALAR(long, val_long);			break;
	case TY_ULONG:	FORMAT_SCALAR(unsigned long, val_ulong);	break;
	case TY_LONGLONG:	FORMAT_SCALAR(long long, val_longlong);	break;
	case TY_ULONGLONG:	FORMAT_SCALAR(unsigned long long, val_ulonglong);	break;
	case TY_ADDRESS:	FORMAT_SCALAR(void*, val_address);	break;
	case TY_FLOAT:	FORMAT_SCALAR(float, val_double);		break;
	case TY_DOUBLE:	FORMAT_SCALAR(double, val_double);		break;
	case TY_LDOUBLE:	FORMAT_SCALAR(long double, val_longdouble);	break;
	case TY_STRING:	FORMAT_SCALAR(char*, val_string);		break;
	case TY_WCHAR:	FORMAT_SCALAR(wchar_t, val_long);		break;
	case TY_WSTRING:	FORMAT_SCALAR(wchar_t*, val_wstring);	break;
	default:
		/* Shouldn't get here */
		assert(0);
	}
}

/*
 * Return the delimiter (defaults to space) for the specified attribute,
 * which should be an array and populated.
 */
static const char *
getAttDelimiter(const tmpl_attribute_t *att)
{
	tmpl_delimiter_t *de = att->ta_delimiter;

	if (de) {
		if (de->de_delimiter) {
			return de->de_delimiter;
		} else {
			/* Record contains no value for delimiter variable. */
			return "";
		}
	} else {
		return " ";
	}
}

/*
 * Format one scalar in an array of scalars.  These macros make sense only
 * when called from formatArrayOfScalars().
 */
#define extractVal(type) *((type*)alval)
#define bpIS(type) _evlBprintf(f, fmt, i, extractVal(type));
#define bpSI(type) _evlBprintf(f, fmt, extractVal(type), i);
#define bpS(type)  _evlBprintf(f, fmt, extractVal(type))
#define FORMAT_PSCALAR(type) if(izo==IS) bpIS(type) else if(izo==SI) bpSI(type) else bpS(type)

static int
formatArrayOfScalars(const tmpl_attribute_t *att, const char *fmt, evl_fmt_buf_t *f)
{
	tmpl_base_type_t ty = baseType(att);
	size_t valsize = _evlTmplTypeInfo[ty].ti_size;
	char *val = (char*) att->ta_value.val_array;
	int nelements = att->ta_dimension->td_dimension2;
	int i;
	enum myIZorder izo = getIZorder(att);
	long long alval[4];	/* big, aligned buffer */
	const char *delimiter = getAttDelimiter(att);

	for (i = 0; i < nelements; i++) {
		if (ty != TY_STRING && ty != TY_WSTRING) {
			(void) memcpy(alval, val, valsize);
		}
                if (i > 0) {
			_evlBprintf(f, "%s", delimiter);
		}
		switch (ty) {
		case TY_CHAR:	FORMAT_PSCALAR(signed char);	break;
		case TY_WCHAR:	FORMAT_PSCALAR(wchar_t);	break;
		case TY_UCHAR:	FORMAT_PSCALAR(unsigned char);	break;
		case TY_SHORT:	FORMAT_PSCALAR(short);		break;
		case TY_USHORT:	FORMAT_PSCALAR(unsigned short);	break;
		case TY_INT:	FORMAT_PSCALAR(int);		break;
		case TY_UINT:	FORMAT_PSCALAR(unsigned int);	break;
		case TY_LONG:	FORMAT_PSCALAR(long);		break;
		case TY_ULONG:	FORMAT_PSCALAR(unsigned long);	break;
		case TY_LONGLONG:	FORMAT_PSCALAR(long long);	break;
		case TY_ULONGLONG:	FORMAT_PSCALAR(unsigned long long);	break;
		case TY_ADDRESS:	FORMAT_PSCALAR(void*);	break;
		case TY_FLOAT:	FORMAT_PSCALAR(float);	break;
		case TY_DOUBLE:	FORMAT_PSCALAR(double);	break;
		case TY_LDOUBLE:	FORMAT_PSCALAR(long double);	break;

		case TY_STRING:
		case TY_WSTRING:
			if (izo == IS) {
				_evlBprintf(f, fmt, i, val);
			} else if (izo == SI) {
				_evlBprintf(f, fmt, val, i);
			} else {
				_evlBprintf(f, fmt, val);
			}
			break;
		default:
			/* Shouldn't get here */
			assert(0);
		}
		if (ty == TY_STRING) {
			val += strlen(val) + 1;
		} else if (ty == TY_WSTRING) {
			val += (wcslen((wchar_t*)val) + 1) * sizeof(wchar_t);
		} else {
			val += valsize;
		}
	}
	return 0;
}

static void
dumpArray(int arch, const tmpl_attribute_t *att, evl_fmt_buf_t *f)
{
	tmpl_base_type_t bt = baseType(att);
	tmpl_arch_type_info_t *ti = &_evlTmplArchTypeInfo[arch][bt];
	int nel = att->ta_dimension->td_dimension2;
	size_t nBytes;
	const void *dumpAddr;

	if (att->ta_rawdata) {
		dumpAddr = att->ta_rawdata;
	} else {
		/* e.g., const array */
		dumpAddr = att->ta_value.val_array;
	}
	if (bt == TY_STRING) {
		/* Count all the chars in all the strings. */
		char *c, *array = (char*) att->ta_value.val_array;
		int ns;
		for (c = array, ns = 0; ns < nel; c++) {
			if (*c == '\0') {
				++ns;
			}
		}
		/* c points one past the last string's null. */
		nBytes = c - array;
	} else if (bt == TY_WSTRING) {
		/* Analogous to TY_STRING */
		wchar_t *c, *array = (wchar_t*) att->ta_value.val_array;
		int ns;
		for (c = array, ns = 0; ns < nel; c++) {
			if (*c == L'\0') {
				++ns;
			}
		}
		/* c points one element past the last string's null. */
		nBytes = (c - array) * sizeof(wchar_t);
	} else if (isStruct(att)) {
		/* Array of structs.  Parser ensures it's not const. */
		evl_listnode_t *head, *tail;
		template_t *firstTmpl, *lastTmpl;

		head = att->ta_value.val_list;
		assert(head != NULL);
		tail = head->li_prev;
		firstTmpl = (template_t*) head->li_data;
		lastTmpl = (template_t*) tail->li_data;
		nBytes = (lastTmpl->tm_data - firstTmpl->tm_data) +
			lastTmpl->tm_data_size;
		dumpAddr = firstTmpl->tm_data;
	} else {
		nBytes = nel * ti->ti_size;
	}
	_evlDumpBytesToFmtBuf(dumpAddr, nBytes, f);
}

/*
 * Format att -- which is an array of char, uchar, or wchar -- using the
 * %s or %S format provided.  If the array doesn't contain a terminating
 * null character, we make a temporary copy and append one.
 */
static void
bprintCharArrAsString(const tmpl_attribute_t *att, evl_fmt_buf_t *f)
{
	const char *fmt = att->ta_format.af_format;
	int nel = att->ta_dimension->td_dimension2;

	if (baseType(att) == TY_WCHAR) {
		wchar_t *tmp = NULL, *val;
		int i;
		val = (wchar_t*) att->ta_value.val_array;
		for (i = 0; i < nel; i++) {
			if (val[i] == L'\0') {
				break;
			}
		}
		if (i >= nel) {
			tmp = (wchar_t*) malloc((nel+1)*sizeof(wchar_t));
			assert(tmp != NULL);
			(void) memcpy(tmp, val, nel*sizeof(wchar_t));
			tmp[nel] = L'\0';
			_evlBprintf(f, fmt, tmp);
			free(tmp);
		} else {
			_evlBprintf(f, fmt, val);
		}
	} else {
		char *tmp = NULL, *val;
		int i;
		val = (char*) att->ta_value.val_array;
		for (i = 0; i < nel; i++) {
			if (val[i] == '\0') {
				break;
			}
		}
		if (i >= nel) {
			tmp = (char*) malloc(nel+1);
			assert(tmp != NULL);
			(void) memcpy(tmp, val, nel);
			tmp[nel] = '\0';
			_evlBprintf(f, fmt, tmp);
			free(tmp);
		} else {
			_evlBprintf(f, fmt, val);
		}
	}
}

/*
 * att is an array of structs.
 * fmt is att's format, modified as follows during template parsing:
 * - The %Z has been replaced by %s.
 * - The %I, if any, has been replaced by %d.
 * For each struct in the array:
 * 1. Format the struct (represented by %Z in the original format string) to a
 * temporary buffer.
 * 2. Using the result of #1, format the array element according to fmt.
 */
static int
formatArrayOfStructs(const tmpl_attribute_t *att, const char *fmt,
	evl_fmt_buf_t *f)
{
	/*
	 * If there's more than SCRATCH_PAD_SIZE left in f's buffer,
	 * then allocate a temp buffer with that much room.  Otherwise,
	 * just use the scratch pad.  We do the former in case one or
	 * more of the templates in the list result in a string greater
	 * than SCRATCH_PAD_SIZE.
	 */
	evl_listnode_t *head, *end, *p;
	enum myIZorder izo = getIZorder(att);
	int i;
	const char *delimiter = getAttDelimiter(att);
#define SCRATCH_PAD_SIZE (4*1024)
	char scratchPad[SCRATCH_PAD_SIZE];
	char *buf;
	size_t bufsz;
	size_t reqlen;
	evl_fmt_buf_t *stbuf;
	ptrdiff_t room = f->fb_end - f->fb_next;
	if (room > SCRATCH_PAD_SIZE) {
		bufsz = room;
		buf = (char*) malloc(room);
		assert(buf != NULL);
	} else {
		bufsz = SCRATCH_PAD_SIZE;
		buf = scratchPad;
	}

	head = att->ta_value.val_list;
	for (i=0, p=head, end=NULL; p!=end; i++, end=head, p=p->li_next) {
		int status;
		template_t *st = (template_t*) p->li_data;
		stbuf = _evlMakeFmtBuf(buf, bufsz);

		/* Format the current template into that buffer. */
		status = formatRecordFromTemplate(st, stbuf);
		if (status == EMSGSIZE) {
			/*
			 * Oh, bother.  This template formats into something
			 * too big to fit in our buffer.  But we have to
			 * build a string that long in order for us to know
			 * how long a buffer we really need for the whole
			 * universe of which we are a part.  Pad the end
			 * with Xs.
			 */
			reqlen = stbuf->fb_next - stbuf->fb_buf;
			if (buf == scratchPad) {
				buf = (char*) malloc(reqlen);
				(void) memcpy(buf, scratchPad, SCRATCH_PAD_SIZE);
			} else {
				buf = realloc(buf, reqlen);
			}
			(void) memset(buf+bufsz-1, 'X', reqlen-bufsz);
			buf[reqlen-1] = '\0';
			bufsz = reqlen;
		} else {
			assert(status == 0);
		}
		_evlFreeFmtBuf(stbuf);
		if (p != head) {
			_evlBprintf(f, "%s", delimiter);
		}
		/* buf contains the formatted output of the current template. */
		switch (izo) {
		case Z:
			_evlBprintf(f, fmt, buf);
			break;
		case IZ:
			_evlBprintf(f, fmt, i, buf);
			break;
		case ZI:
			_evlBprintf(f, fmt, buf, i);
			break;
		default:
			assert(0);
		}
	}
	if (buf != scratchPad) {
		free(buf);
	}
	return 0;
}

static int
formatAttribute(int arch, const tmpl_attribute_t *att, evl_fmt_buf_t *f)
{
	const char *fmt;
	if (!attExists(att)) {
		/* No data for this attribute. */
		return 0;
	}

	if (isArray(att)) {
		switch (att->ta_format.af_type) {
		case TMPL_AFS_DUMP:
			dumpArray(arch, att, f);
			return 0;
		case TMPL_AFS_CHARRSTR:
			bprintCharArrAsString(att, f);
			return 0;
                case TMPL_AFS_DEFAULT:
			assert(baseType(att) != TY_STRUCT);
			fmt = _evlTmplTypeInfo[baseType(att)].ti_format;
			return formatArrayOfScalars(att, fmt, f);
		case TMPL_AFS_ARRAY:
		case TMPL_AFS_IZARRAY:
			fmt = att->ta_format.af_format;
			if (baseType(att) == TY_STRUCT) {
				return formatArrayOfStructs(att, fmt, f);
			} else {
				return formatArrayOfScalars(att, fmt, f);
			}
		default:
			dprintf("Unsupported array format: %d\n",
				att->ta_format.af_type);
			assert(0);
		}
	} else if (isStruct(att)) {
		return formatRecordFromTemplate(att->ta_value.val_struct, f);
	} else {
		/* Not an array, not a struct.  ta_value holds the value. */
		/* Note: Populated bit-fields look like scalar integers. */
		switch (att->ta_format.af_type) {
		case TMPL_AFS_DEFAULT:
			fmt = _evlTmplTypeInfo[baseType(att)].ti_format;
			castAndFormatAtt(att, fmt, f);
			break;
		case TMPL_AFS_SCALAR:
			fmt = att->ta_format.af_format;
			castAndFormatAtt(att, fmt, f);
			break;
		case TMPL_AFS_VALNM:
		    {
			/* %v */
		    	long val = _evlTmplGetValueOfIntegerAttribute(att);
			tmpl_bitmap_t *bmap = att->ta_format.u.u_bitmaps;
			int found = 0;
			for ( ; bmap->bm_name; bmap++) {
				if (val == bmap->bm_1bits) {
					found = 1;
					_evlBprintf(f, "%s", bmap->bm_name);
					break;
				}
			}
			if (!found) {
				_evlBprintf(f, "%ld", val);
			}
			break;
		    }
		case TMPL_AFS_BITMAP:
		    {
			/* %b */
		    	long val = _evlTmplGetValueOfIntegerAttribute(att);
			long bar = 0x0;	/* Bits Already Reported */
			tmpl_bitmap_t *bmap = att->ta_format.u.u_bitmaps;

			_evlBprintf(f, "0x%lx", val);
			for ( ; bmap->bm_name; bmap++) {
				if ((bar|bmap->bm_1bits|bmap->bm_0bits)==bar) {
					/*
					 * We've already reported the name of
					 * a bitmap that covers all the bits
					 * that bmap covers.
					 */
					continue;
				}
				if (((val & bmap->bm_1bits) == bmap->bm_1bits)
				    && ((val & bmap->bm_0bits) == 0)) {
					/* Found a match */
					_evlBprintf(f, (bar ? "|" : "("));
					_evlBprintf(f, "%s", bmap->bm_name);
					bar |= (bmap->bm_1bits|bmap->bm_0bits);
				}
			}
			if (bar) {
				_evlBprintf(f, ")");
			}
			break;
		    }
		/* TODO: Handle dump of scalar attribute */
		case TMPL_AFS_DUMP:
		default:
			dprintf("Unsupported scalar format: %d\n",
				att->ta_format.af_type);
			assert(0);
		}
	}
	return 0;
}

/*
 * Format the record (or part thereof) defined by data, datasz into format
 * buffer f.  data points to a format string.  argsz and args follow.
 * printk=1 -> use prink formatting.
 */
static int
formatPrintfRecIntoFmtBuf(const char *data, size_t datasz, evl_fmt_buf_t *f,
	int printk)
{
	ptrdiff_t bufRemainder;
	size_t reqlen;
	int status;

	bufRemainder = f->fb_end - f->fb_next;
	if (bufRemainder <= 0) {
		return EMSGSIZE;
	}

	status = _evlFormatPrintfRec(data, datasz, f->fb_next,
		bufRemainder, &reqlen, printk);
	f->fb_next += reqlen - 1;	/* Back up over terminating null */
	f->fb_oflow = (f->fb_next >= f->fb_end);
	return status;
}

/*
 * If att is of type TY_STRING and its user-specified format is "printf"
 * or "printk", then format the string and subsequent attributes
 * accordingly.  Otherwise (the typical case) return -1.  Return
 * EMSGSIZE if _evlFormatPrintfRec() overflows buffer f, or 0 if not.
 */
static int
formatStringAsPrintf(const template_t *tmpl, const tmpl_attribute_t *att,
	const char *fmt, evl_fmt_buf_t *f)
{
	int printk = 0;
	const char *attAddr, *recAddr;
	size_t recRemainder;

	if (isArray(att)
	    || baseType(att) != TY_STRING
	    || !fmt) {
		return -1;
	}
	if (strcmp(fmt, "printk") == 0) {
		printk = 1;
	} else if (strcmp(fmt, "printf") != 0) {
		return -1;
	}

	attAddr = att->ta_value.val_string;
	recAddr = tmpl->tm_data;
	recRemainder = tmpl->tm_data_size - (attAddr - recAddr);

	return formatPrintfRecIntoFmtBuf(attAddr, recRemainder, f, printk);
}

/*
 * Format attribute att into format buf f.  If fmt != NULL, use that format.
 * Otherwise use the format specified for that attribute in the template.
 * If fmt is non-null, and the attribute is an array, struct, or string,
 * the result will be ?attname? to indicate that we can't use the supplied
 * format.
 */
int
_evlFormatTmplAttribute(const template_t *tmpl, const tmpl_attribute_t *att,
	const char *fmt, evl_fmt_buf_t *f)
{
	int status;
	
	if (!fmt) {
		return formatAttribute(tmpl->tm_arch, att, f);
	}

	if (cantDoAdHocFormat(att)) {
		_evlBprintf(f, "?%s?", att->ta_name);
		return EINVAL;
	}

	if (!attExists(att)) {
		return 0;
	}

	status = formatStringAsPrintf(tmpl, att, fmt, f);
	if (status != -1) {
		return status;
	}

	castAndFormatAtt(att, fmt, f);
	return 0;
}

/*
 * Returns a pointer to the simple (no dots) host name.  The name is computed
 * only once; subsequent calls to sethostname() have no effect.
 */
const char *
_evlGetHostName()
{
	static char namebuf[256];
	static char *name = NULL;
	if (!name) {
		int status = gethostname(namebuf, 256);
		if (status == 0) {
			char *dot = strchr(namebuf, '.');
			if (dot) {
				*dot = '\0';
			}
		} else {
			(void) strcpy(namebuf, "");
		}
		name = namebuf;
	}
	return name;
}

struct evlhost {
	char name[256];
	int id;
};
static struct evlhost *evlhosts = (struct evlhost *) 0;
static int numhosts = 0;

static int 
addEvlHost(char *name, int id) {
	evlhosts = (struct evlhost *) realloc(evlhosts, 
										  (numhosts + 1) * sizeof(struct evlhost));
	if (evlhosts == NULL) {
		return -1;
	}
	strcpy(evlhosts[numhosts].name, name);
	evlhosts[numhosts].id = id;
	++numhosts;
	return 0;
}

#define EVLHOSTS "/etc/evlog.d/evlhosts"
static int
populateEvlHosts()
{
	char *p, *name, *id;
	char line[256];
	FILE * f;
	struct evlhost *hp;
	int nodeID;
	int ret = 0;
	
	numhosts = 0;
	if((f = fopen(EVLHOSTS, "r")) == NULL) {
		fprintf(stderr, "can't open evlhosts file.\n");
		return -1;
	}	

	while (fgets(line, 256, f) != NULL) {
		int len;
		if (line[0] == '#' || line[0] == '\n' 
			|| strcspn(line, "") == 0) {
			continue;
		}
		len = strlen(line);
		/* replace newline with null char */
		if (line[len -1] == '\n') {
			line[len -1] = '\0';
		}

		/* host id */		
		id = (char *) strtok(line, " \t");
		if (!id) {
			ret = -1;
			goto error_exit;
		}
		_evlTrim(id);
		/*  host name */
		name = (char *) strtok(NULL, " \t");
		if (!name) {
			ret = -1;
			goto error_exit;
		}
		/* Remove leading space and trailing space */
		_evlTrim(name);

		if ((p=strchr(id, '.')) != NULL) {
			/* id in 255.255 format */
			char *endp = 0;
			int lowerbyte, upperbyte;
			upperbyte = (int) strtoul(id, &endp, 10);
			if (*endp != '.') {
				fprintf(stderr, "%s is an invalid node id.\n", id);
				continue;
			}
			lowerbyte = (int) strtoul(p + 1, &endp, 10);
			if (*endp != '\0') {
				fprintf(stderr, "%s is an invalid node id.\n", id);
				continue;
			}
			if (upperbyte > 255 || lowerbyte > 255) {
				fprintf(stderr, "%s is an invalid node id.\n", id);
				continue;
			}
			nodeID = (upperbyte << 8) + lowerbyte;
		} else {
			nodeID = (int) strtoul(id, (char **) NULL, 0);
		}
		addEvlHost(name, nodeID);
	}
		  
 error_exit:	
	fclose(f);
	return ret;
}
/* TODO: Make this function thread safe */
const char *
lookUpEvlHostName(int nodeId)
{
	int i;
	static char buf[8];

	if (!evlhosts) {
		populateEvlHosts();
	}
	if (evlhosts) {
		for (i=0; i < numhosts; ++i) {
			if(evlhosts[i].id == nodeId) {
				return evlhosts[i].name;
			}
		}
	}
	snprintf(buf, sizeof(buf), "0x%x", nodeId);
	return buf; 
}

int
_evlGetNodeId(const char * name)
{
	int i;

	if (!strcmp(name, _evlGetHostName())) {
		/* local host - return nodeId = 0 for local host */
		return 0;
	}
	if (!evlhosts) {
		populateEvlHosts();
	}
	if (evlhosts) {
		for (i=0; i < numhosts; ++i) {
			if (!strcmp(evlhosts[i].name, name)) {
				return evlhosts[i].id;
			}
		}
	}
	return -1;
}
const char *
_evlGetHostNameEx(int nodeId)
{
	nodeId &= 0x0000FFFF;
	if (nodeId == 0) {
		return _evlGetHostName();
	}
	return lookUpEvlHostName(nodeId);
}

/*
 * head heads a list of string and attribute segments that indicate how a record
 * should be formatted.  Format the indicated record accordingly into format
 * buffer s.
 *
 * If tmpl is non-null, it is a pointer to a template populated with this
 * event record.  It is used as necessary to format %data% in a binary
 * record.  (NOTE: For this reason, a template's format section cannot contain
 * %data%.  This would create an infinite loop.)  It is also used to format
 * segments of type EVL_FS_ATTNAME.
 *
 * Return 0 for success.  If there is not sufficient room in fmtBuf, fill
 * and null-terminate it and return EMSGSIZE.
 *
 * NOTE: This code used to reside in util/format.c.  When template support
 * was added, it was moved here to simplify include-file dependencies.  Keep
 * in mind that this function is the basis for evlview -S and -F as well as 
 * template-based formatting.
 */
static int
formatRecord(const struct posix_log_entry *entry, const void *evBuf,
	const template_t *tmpl, const evl_list_t *head, evl_fmt_buf_t *s)
{
	const evl_listnode_t *p = head;
	const evl_fmt_segment_t *fs;
	int status;

	do {
		fs = (const evl_fmt_segment_t *) p->li_data;
		switch (fs->fs_type) {
		case EVL_FS_STRING:
			_evlBprintf(s, "%s", fs->u.fs_string);
			break;
		case EVL_FS_MEMBER:
			if (fs->u2.fs_member == POSIX_LOG_ENTRY_DATA) {
				/* %data% */
				if (tmpl) {
					formatRecordFromTemplate(tmpl, s);
				} else if (entry->log_format == POSIX_LOG_STRING) {
					_evlBprintf(s, "%s", evBuf);
				} else if (entry->log_format == POSIX_LOG_BINARY) {
					_evlDumpBytesToFmtBuf(evBuf,
						entry->log_size, s);
				} else if (entry->log_format == POSIX_LOG_PRINTF) {
					formatPrintfRecIntoFmtBuf(evBuf,
						entry->log_size, s,
						(entry->log_flags & EVL_PRINTK) != 0);
				}
			} else if (fs->fs_userfmt == NULL) {
				/* Standard scalar attribute, standard format */
				char buf[POSIX_LOG_MEMSTR_MAXLEN];
				status = posix_log_memtostr(fs->u2.fs_member,
					entry, buf, POSIX_LOG_MEMSTR_MAXLEN);
				assert(status == 0);
				_evlBprintf(s, "%s", buf);
			} else {
				/* Use the user-specified format. */
				const char *fmt = fs->fs_userfmt;
				if (fs->fs_stringfmt) {
					/*
					 * User wants a variation of what we
					 * would have printed anyway.
					 */
					char buf[POSIX_LOG_MEMSTR_MAXLEN];
					status = posix_log_memtostr(
						fs->u2.fs_member, entry, buf,
						POSIX_LOG_MEMSTR_MAXLEN);
					assert(status == 0);
					_evlBprintf(s, fmt, buf);
				} else {
					_evlSprintfMember(s, fmt,
						fs->u2.fs_member, entry);
				}
			}
			break;
		case EVL_FS_ATTNAME:
		    {
			/*
			 * Referencing a non-standard attribute, which may
			 * or may not appear in the specified template.
			 * If it's not there, we encode nothing.
			 */
		    	tmpl_attribute_t *att;
			if (tmpl && (status = evltemplate_getatt(tmpl,
			    fs->u.fs_attname, &att)) == 0) {
				_evlFormatTmplAttribute(tmpl, att,
					fs->fs_userfmt, s);
			} else if (!strcmp(fs->u.fs_attname, "host")) {
				/* host pseudo-attribute */
				_evlBprintf(s, "%s", _evlGetHostNameEx(entry->log_processor >> 16));
			}
		    }
			break;
		case EVL_FS_ATTR:
			assert(fs->u2.fs_attribute != NULL);
			_evlFormatTmplAttribute(tmpl, fs->u2.fs_attribute,
				fs->fs_userfmt, s);
			break;
    default: /* keep gcc happy */;
		}
		p = p->li_next;
	} while (p != head);

	return (s->fb_oflow ? EMSGSIZE : 0);
}

/*
 * list heads a list of string and attribute segments that indicate how a record
 * should be formatted.  Format the indicated record (entry, evBuf) accordingly
 * into fmtBuf, whose length is assumed to be fmtBufLen.  Use tmpl as necessary
 * to format %data% or non-standard attributes.  Whether or not fmtBuf is big
 * enough to hold the formatted record, set *reqLen to the number of bytes we
 * would write if we had enough room.
 */
int
_evlSpecialFormatEvrec(const struct posix_log_entry *entry, const void *evBuf,
	const evltemplate_t *tmpl, const evl_list_t *list, char *fmtBuf,
	size_t fmtBufLen, size_t *reqLen)
{
	evl_fmt_buf_t *s;
	int status;

	s = _evlMakeFmtBuf(fmtBuf, fmtBufLen);
	status = formatRecord(entry, evBuf, tmpl, list, s);
	if (reqLen) {
		*reqLen = s->fb_next - s->fb_buf;
	} 
	_evlFreeFmtBuf(s);
	return status;
}

static int
formatRecordFromTemplate(const template_t *template, evl_fmt_buf_t *f)
{
	if (! template || ! f) {
		return EINVAL;
	}
	if (! isPopulatedTmpl(template)) {
		return EINVAL;
	}

	return formatRecord(template->tm_entry, template->tm_data, template,
		template->tm_parsed_format, f);
}

int
evlatt_getstring(const evlattribute_t *att, char *buf, size_t buflen)
{
	evl_fmt_buf_t *f;
	int status;

	if (!att || !buf) {
		return EINVAL;
	}

	if (buflen == 0) {
		return EMSGSIZE;
	}

	f = _evlMakeFmtBuf(buf, buflen);
	status = formatAttribute(0, att, f);	/* 0 is local architecture */
	if (status != 0) {
		dprintf("formatAttribute failed with status = %d\n", status);
	} else if (f->fb_oflow) {
		status = EMSGSIZE;
	}
	_evlFreeFmtBuf(f);
	return status;
}

int
evltemplate_formatrec(const evltemplate_t *t, char *buf, size_t buflen)
{
	evl_fmt_buf_t *f;
	int status;

	if (!t || !buf) {
		return EINVAL;
	}

	if (buflen == 0) {
		return EMSGSIZE;
	}

	f = _evlMakeFmtBuf(buf, buflen);
	status = formatRecordFromTemplate(t, f);
	if (status != 0) {
		dprintf("formatRecordFromTemplate failed with status = %d\n",
			status);
	} else if (f->fb_oflow) {
		status = EMSGSIZE;
	}
	_evlFreeFmtBuf(f);
	return status;
}

static void
bindent(evl_fmt_buf_t *f, int indent)
{
	int i;
	for (i = 0; i < indent; i++) {
		_evlBprintf(f, "  ");
	}
}

static int
isNewlineDelimitedArray(tmpl_attribute_t *att)
{
	tmpl_att_format_t *fmt;
	const char *delim;
	
	if (!isArray(att)) {
		return 0;
	}
	fmt = &att->ta_format;
	delim = getAttDelimiter(att);
	if (delim && strchr(delim, '\n')) {
		return 1;
	}
	if ((fmt->af_type == TMPL_AFS_ARRAY
	    || fmt->af_type == TMPL_AFS_IZARRAY)
	    && strchr(fmt->af_format, '\n')) {
	    	return 1;
	}
	return 0;
}

/* Encode the value for a "name=value" line. */
static void
neqvDumpAtt(int arch, tmpl_attribute_t *att, evl_fmt_buf_t *f)
{
	if (att->ta_format.af_type == TMPL_AFS_DUMP
	    || isNewlineDelimitedArray(att)) {
		_evlBprintf(f, "\n");
	} else {
		_evlBprintf(f, " ");
	}
	(void) formatAttribute(arch, att, f);
	_evlBprintf(f, "\n");
}

static void
neqvDumpTmpl(const evltemplate_t *t, evl_fmt_buf_t *f, int indent)
{
	evl_listnode_t *head, *end, *p;
	
	head = t->tm_attributes;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		tmpl_attribute_t *att = (tmpl_attribute_t*) p->li_data;
		if (!att->ta_name) {
			continue;
		}
		if (!attExists(att)) {
			continue;
		}
		bindent(f, indent);
		_evlBprintf(f, "%s\t=", att->ta_name);
		if (isStruct(att) && !isArray(att)) {
			_evlBprintf(f, "\n");
			neqvDumpTmpl(att->ta_value.val_struct, f, indent+1);
		} else {
			neqvDumpAtt(t->tm_arch, att, f);
		}
	}
}

/*
 * t is a populated template.  Format the record's attributes into buf,
 * using name=value format.
 */
int
evltemplate_neqvdump(const evltemplate_t *t, char *buf, size_t buflen)
{
	evl_fmt_buf_t *f;
	int status = 0;

	if (!t || !buf) {
		return EINVAL;
	}
	if (buflen == 0) {
		return EMSGSIZE;
	}
	if (! isPopulatedTmpl(t)) {
		return EINVAL;
	}

	f = _evlMakeFmtBuf(buf, buflen);
	neqvDumpTmpl(t, f, 0);
	if (f->fb_oflow) {
		status = EMSGSIZE;
	}
	_evlFreeFmtBuf(f);
	return status;
}
