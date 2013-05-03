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
 *
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <wchar.h>
// #include <endian.h>

#include "config.h"
#include "evl_common.h"
#include "evl_util.h"
#include "evl_template.h"

#define EXTRA_DATA_ATT_NAME "_EXTRA_DATA_"

/*
 * typedefs is the list of typedefs (stored as tmpl_attribute_ts) seen so
 * far in this source file.  Currently, this list is never freed.
 * TODO: Improve performance by hashing typedefs.
 */
evl_list_t *typedefs = NULL;

extern void tterror(char *s);	/* AKA yyerror */
extern tmpl_base_type_t _evlGetTypeFromConversion(struct evl_parsed_format *pf,
	int promote, int signedOnly);

static int validateAttInitializer(template_t *template, tmpl_attribute_t *att,
	tmpl_type_and_value_t *val);
static void freeArrayOfStructs(tmpl_attribute_t *att);
static void zapExtraDataAtt(template_t *template);

#define SZALGN(type) sizeof(type), __alignof__(type)
#ifdef __i386__
/* __alignof__ doesn't seem to work for the big types */
#define LLALIGN 4
#define ULLALIGN 4
#define DALIGN 4
#define LDALIGN 4
#else
#define LLALIGN __alignof__(long long)
#define ULLALIGN __alignof__(unsigned long long)
#define DALIGN __alignof__(double)
#define LDALIGN __alignof__(long double)
#endif

tmpl_type_info_t _evlTmplTypeInfo[] = {
	/* size, align, isScalar, isInteger, name, default format */
	{0, 0,			0, 0,	"none",		"%#x"},
	{SZALGN(char),		1, 1,	"char",		"%d"},
	{SZALGN(unsigned char),	1, 1,	"uchar",	"%u"},
	{SZALGN(short),		1, 1,	"short",	"%d"},
	{SZALGN(unsigned short),1, 1,   "ushort",	"%u"},
	{SZALGN(int),		1, 1,	"int",		"%d"},
	{SZALGN(unsigned int),	1, 1,	"uint",		"%u"},
	{SZALGN(long),		1, 1,	"long",		"%ld"},
	{SZALGN(unsigned long),	1, 1,	"ulong",	"%lu"},
	{sizeof(long long), LLALIGN,
				1, 1,	"longlong",	"%Ld"},
	{sizeof(unsigned long long), ULLALIGN,
				1, 1,	"ulonglong",	"%Lu"},
	{SZALGN(float),		1, 0,	"float",	"%f"},
	{sizeof(double), DALIGN,
				1, 0,	"double",	"%f"},
	{sizeof(long double), LDALIGN,
				1, 0,	"ldouble",	"%Lf"},
	{sizeof(char*), 0,	0, 0,	"string",	"%s"},
	{SZALGN(wchar_t),	1, 1,	"wchar",	"%lc"},
	{sizeof(wchar_t*), 0,	0, 0,	"wstring",	"%ls"},
	{SZALGN(void*),		1, 1,	"address",	"%p"},
	{0, 0,			0, 0,	"struct",	"%#x"},
	{0, 0,			0, 0,	"prefix3",	"%#x"},
	{0, 0,			0, 0,	"list",		"%#x"},
	{0, 0,			0, 0,	"struct",	"%#x"},/*TY_STRUCTNAME*/
	{0, 0,			0, 0,	"typedef",	"%#x"},	/*TY_TYPEDEF*/
	{0, 0,			0, 0,	NULL,		NULL}	/* the end */
};

/*
 * Todo : Make sure to find out exactly LLALIGN ULLALIGN DALIGN LDALIGN
 * for each platform - and replace them in the _evlTmplArchTypeInfo array
 */ 
tmpl_arch_type_info_t _evlTmplArchTypeInfo[10][24] = {
/* the first subcript is for architecture */
/* LOGREC_NO_ARCH */
	{
	/* size, align */
	{0, 0},
	{SZALGN(char)},
	{SZALGN(unsigned char)},
	{SZALGN(short)},
	{SZALGN(unsigned short)},
	{SZALGN(int)},
	{SZALGN(unsigned int)},
	{SZALGN(long)},
	{SZALGN(unsigned long)},
	{sizeof(long long), 	LLALIGN},
	{sizeof(unsigned long long), ULLALIGN},
	{SZALGN(float)},
	{sizeof(double), 	DALIGN},
	{sizeof(long double), 	LDALIGN},
	{sizeof(char*), 0},
	{SZALGN(wchar_t)},
	{sizeof(wchar_t*), 0},
	{SZALGN(void*)},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/* LOGREC_ARCH_I386 */
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{4, 4},
	{4, 4},
	{8,	4},
	{8, 4},
	{4, 4},
	{8,	4},
	{12, 4},
	{4, 0},
	{4, 4},
	{4, 0},
	{4, 4},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/*LOGREC_ARCH_IA64*/
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{8, 8},
	{8, 8},
	{8,	8},
	{8, 8},
	{4, 4},
	{8,	8},
	{16, 16},
	{8, 0},
	{4, 4},
	{8, 0}, 
	{8, 8},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/* LOGREC_ARCH_S390 */
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{4, 4},
	{4, 4},
	{8,	8},
	{8, 8},
	{4, 4},
	{8,	8},
	{8, 8},
	{4, 0},
	{4, 4},
	{4, 0},
	{4, 4},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/* LOGREC_ARCH_S390X */
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{8, 8},
	{8, 8},
	{8,	8},
	{8, 8},
	{4, 4},
	{8,	8},
	{8, 8},
	{8, 0},
	{4, 4},
	{8, 0},
	{8, 8},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/* LOGREC_ARCH_PPC */
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{4, 4},
	{4, 4},
	{8,	8},
	{8, 8},
	{4, 4},
	{8,	8},
	{8, 8},
	{4, 0},
	{4, 4},
	{4, 0},
	{4, 4},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/* LOGREC_ARCH_PPC64 */
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{8, 8},
	{8, 8},
	{8,	8},
	{8, 8},
	{4, 4},
	{8,	8},
	{8, 8},
	{8, 0},
	{4, 4},
	{8, 0}, 
	{8, 8},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/*LOGREC_ARCH_X86_64*/
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{8, 8},
	{8, 8},
	{8,	8},
	{8, 8},
	{4, 4},
	{8,	8},
	{16, 16},
	{8, 0},
	{4, 4},
	{8, 0},
	{8, 8},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/* LOGREC_ARCH_ARM_BE */
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{8, 8},
	{8, 8},
	{8,	8},
	{8, 8},
	{4, 4},
	{8,	8},
	{8, 8},
	{8, 0},
	{4, 4},
	{8, 0}, 
	{8, 8},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	},
/* LOGREC_ARCH_ARM_LE */
	{
	{0, 0},
	{1,	1},
	{1, 1},
	{2, 2},
	{2, 2},
	{4, 4},
	{4, 4},
	{8, 8},
	{8, 8},
	{8,	8},
	{8, 8},
	{4, 4},
	{8,	8},
	{8, 8},
	{8, 0},
	{4, 4},
	{8, 0}, 
	{8, 8},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},/*TY_STRUCTNAME*/
	{0, 0},	/*TY_TYPEDEF*/
	{0, 0}	/* the end */
	}
};



typedef struct tmpl_popl_notes {
	char	*pn_next;	/* start of remaining data */
	char	*pn_end;	/* 1 past end of data */
	char	pn_out_of_data;	/* 1 if no more data to populate */
/* Note: We can be out of data even if pn_next < pn_end.  In that case,
 * pn_next points to the "remainder data"
 */
} tmpl_popl_notes_t;

static void populateTmpl(template_t *template, tmpl_popl_notes_t *notes);

/***** Functions to export *****/
void
_evlTmplDprintf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

#define semanticError _evlTmplSemanticError
void
semanticError(const char *fmt, ...)
{
	char errbuf[200];
	template_t *t;
	tmpl_parser_context_t *pc = _evlTmplGetParserContext();

	va_list args;
	va_start(args, fmt);
	vsnprintf(errbuf, 200, fmt, args);
	tterror(errbuf);
	va_end(args);

	t = pc->pc_template;
	if (t) {
		t->tm_flags |= TMPL_TF_ERROR;
	} else {
		pc->pc_errors++;
	}
}

/* Return 1 if s starts with prefix, 0 if not. */
int
_evlStartsWith(const char *s, const char *prefix)
{
	size_t plen;

	if (!s || !prefix || (plen = strlen(prefix)) == 0) {
		return 0;
	}

	return ! strncmp(s, prefix, plen);
}

/* Return 1 if s ends with suffix, 0 if not. */
int
_evlEndsWith(const char *s, const char *suffix)
{
	size_t slen, sfxlen;

	if (!s || !suffix || (sfxlen = strlen(suffix)) == 0
	    || (slen = strlen(s)) < sfxlen) {
		return 0;
	}

	return ! strncmp(s+(slen-sfxlen), suffix, sfxlen);
}

/***** Auxiliary functions *****/

tmpl_attribute_t *
_evlTmplGetNthAttribute(template_t *t, int n)
{
	assert(t != NULL);
	return (tmpl_attribute_t*) _evlGetNthValue(t->tm_attributes, n);
}

void
_evlTmplIncRef(template_t *t)
{
	_evlLockMutex(&t->tm_mutex);
	t->tm_ref_count++;
	_evlUnlockMutex(&t->tm_mutex);
}

void
_evlTmplDecRef(template_t *t)
{
	_evlLockMutex(&t->tm_mutex);
	t->tm_ref_count--;
	_evlUnlockMutex(&t->tm_mutex);
}

template_t *
_evlAllocTemplate()
{
	template_t *t = (template_t *) malloc(sizeof(template_t));
	assert(t != NULL);
	memset(t, 0, sizeof(template_t));
	t->tm_recid = (posix_log_recid_t) -1;
#ifdef _POSIX_THREADS
	(void) pthread_mutex_init(&t->tm_mutex, NULL);
#endif
	return t;
}

template_t *
_evlMakeEvrecTemplate(posix_log_facility_t facility, int eventType,
	const char *description)
{
	char name[100];
	template_t *t = _evlAllocTemplate();
	t->tm_header.th_type = TMPL_TH_EVLOG;
	t->tm_header.th_description = description;
	t->tm_header.u.u_evl.evl_facility = facility;
	t->tm_header.u.u_evl.evl_event_type = eventType;
	_evlTmplMakeEvlogName(facility, eventType, name, sizeof(name));
	t->tm_name = strdup(name);
	return t;
}

template_t *
_evlMakeStructTemplate(const char *structName, const char *description)
{
	template_t *t = _evlAllocTemplate();
	t->tm_header.th_type = TMPL_TH_STRUCT;
	t->tm_header.th_description = description;
	t->tm_header.u.u_struct.st_name = structName;
	t->tm_name = structName;
	return t;
}

template_t *_evlCloneTemplate(template_t *t1);

tmpl_data_type_t *
_evlTmplAllocDataType()
{
	tmpl_data_type_t *t = (tmpl_data_type_t *) malloc(sizeof(tmpl_data_type_t));
	assert(t != NULL);
	memset(t, 0, sizeof(tmpl_data_type_t));
	return t;
}

tmpl_dimension_t *
_evlTmplAllocDimension()
{
	tmpl_dimension_t *t = (tmpl_dimension_t *) malloc(sizeof(tmpl_dimension_t));
	assert(t != NULL);
	memset(t, 0, sizeof(tmpl_dimension_t));
	return t;
}

tmpl_type_and_value_t *
_evlTmplAllocTypeAndValue()
{
	tmpl_type_and_value_t *t = (tmpl_type_and_value_t *) malloc(sizeof(tmpl_type_and_value_t));
	assert(t != NULL);
	memset(t, 0, sizeof(tmpl_type_and_value_t));
	return t;
}

tmpl_delimiter_t *
_evlTmplAllocDelimiter()
{
	tmpl_delimiter_t *d = (tmpl_delimiter_t *) malloc(sizeof(tmpl_delimiter_t));
	assert(d != NULL);
	memset(d, 0, sizeof(tmpl_delimiter_t));
	return d;
}

tmpl_attribute_t *
_evlTmplAllocAttribute()
{
	tmpl_attribute_t *t = (tmpl_attribute_t *) malloc(sizeof(tmpl_attribute_t));
	assert(t != NULL);
	memset(t, 0, sizeof(tmpl_attribute_t));
	return t;
}

static void
dumpAttribute(template_t *template, int constAttr, tmpl_data_type_t *type,
	const char *name, tmpl_dimension_t *dim,
	tmpl_type_and_value_t *val, const char *format)
{
	_evlTmplDprintf("Adding %s attribute %s:\n",
		(constAttr ? "const" : "non-const"),
		(name ? name : "<unnamed>"));
	_evlTmplDprintf("Base type = %s\n", _evlTmplTypeInfo[type->tt_base_type].ti_name);
	if (type->tt_base_type == TY_STRUCTNAME) {
		_evlTmplDprintf("\t(struct %s)\n", type->u.st_name);
	}
	if (dim) {
		_evlTmplDprintf("Dimension type = %d\n", dim->td_type);
		_evlTmplDprintf("Dimension = ");
		switch (dim->td_type) {
		case TMPL_DIM_CONST:
			_evlTmplDprintf("%d\n", dim->td_dimension);
			break;
		case TMPL_DIM_IMPLIED:
			_evlTmplDprintf("[]\n");
			break;
		case TMPL_DIM_BITFIELD:
			_evlTmplDprintf("%d bits\n", dim->td_dimension);
			break;
		case TMPL_DIM_ATTNAME:
			_evlTmplDprintf("%s\n", dim->u.u_attname);
			break;
		default:
			_evlTmplDprintf("<unexpected>\n");
			break;
		}
	} else {
		_evlTmplDprintf("No dimension\n");
	}
	if (val) {
		_evlTmplDprintf("Initializer type = %s\n",
			_evlTmplTypeInfo[val->tv_type].ti_name);
	} else {
		_evlTmplDprintf("No initializer\n");
	}
	_evlTmplDprintf("Format = %s\n", (format ? format : "<default>"));
}

static tmpl_attribute_t *
findAttBySimpleName(const template_t *template, const char *name) 
{
	evl_listnode_t *attNode =
		_evlFindNamedItemInList(template->tm_attributes, name);
	if (attNode) {
		return (tmpl_attribute_t*) attNode->li_data;
	} else {
		return NULL;
	}
}

/*
 * path is a simple attribute name, or a series of attribute names delimited
 * by periods (e.g., "startPoint.x").  path is non-const, so we can replace
 * periods with nulls as needed.  Return a pointer to the indicated attribute,
 * or NULL if there is none.
 *
 * Unless path is a simple name, the first name in the series must specify
 * an attribute of type TY_STRUCT in the indicated template.  If template
 * is a master template, we follow that attribute's ta_type->u.st_template
 * pointer to get the master template for that struct.  Otherwise...
 * 1) If that attribute is not populated, we return NULL.
 * 2) If it is populated, we follow that attribute's att->ta_value.val_struct
 * pointer to get the populated clone template that contains that struct's
 * values.
 */
static tmpl_attribute_t *
findAttByScopedName(const template_t *template, char *path) 
{
	char *dot = strchr(path, '.');
	if (dot) {
		char *structAttName = path;
		tmpl_attribute_t *structAtt;
		*dot = '\0';
		path = dot + 1;

		structAtt = findAttBySimpleName(template, structAttName);
		if (!structAtt || !isStruct(structAtt) || isArray(structAtt)) {
			return NULL;
		}
		if (attExists(structAtt)) {
			return findAttByScopedName(
				structAtt->ta_value.val_struct, path);
		} else if (isMasterTmpl(template)) {
			return findAttByScopedName(
				structAtt->ta_type->u.st_template, path);
		} else {
			return NULL;
		}
	} else {
		return findAttBySimpleName(template, path);
	}
}

tmpl_attribute_t *
_evlTmplFindAttribute(const template_t *template, const char *name) 
{
	if (strchr(name, '.')) {
		tmpl_attribute_t *att;
		char *path = strdup(name);
		assert(path != NULL);
		att = findAttByScopedName(template, path);
		free(path);
		return att;
	} else {
		return findAttBySimpleName(template, name);
	}
}

tmpl_attribute_t *
findTypedef(const char *name)
{
	evl_listnode_t *td = _evlFindNamedItemInList(typedefs, name);
	return (td ? (tmpl_attribute_t*) td->li_data : NULL);
}

static int
validateAttName(template_t *template, tmpl_attribute_t *att)
{
	const char *name = att->ta_name;

	/*
	 * Missing names are allowed only for bit-fields.
	 * Parser should ensure that.
	 */
	if (!name) {
		assert(isBitField(att));
		return 0;
	}

	/* Verify that this isn't the name of a standard attribute. */
	if (_evlGetValueByName(_evlAttributes, name, -1) != -1) {
		semanticError("Reserved name: %s", name);
		return -1;
	}

	/* Verify that this isn't a name that is otherwise reserved. */
	if (!strcmp(name, "_R_") || !strcmp(name, "_PREFIX3_")) {
		semanticError("Reserved name: %s", name);
		return -1;
	}

	if (isTypedef(att)) {
		if (findTypedef(name) != NULL) {
			semanticError("typedef name already in use: %s", name);
			return -1;
		}
	} else {
		/*
		 * Verify that this template has not already defined an
		 * attribute by this name.
		 */
		if (_evlTmplFindAttribute(template, name)) {
			semanticError("Attribute name already in use: %s", name);
			return -1;
		}
	}

	return 0;
}

static int
validateAttType(template_t *template, tmpl_attribute_t *att)
{
	template_t *structTemplate;
	tmpl_base_type_t bt = baseType(att);

	if (isBitField(att)) {
		if (!isIntegerAtt(att)) {
			semanticError("Bit-field %s must be integer.",
				(att->ta_name ? att->ta_name : ""));
			return -1;
		}
		/* Bit-fields will be handled later. */
	}

	if (bt == TY_STRUCTNAME) {
		const char *structName = att->ta_type->u.st_name;

		structTemplate = _evlFindStructTemplate(structName);
		if (!structTemplate) {
			semanticError("Cannot find template for struct %s",
				structName);
			return -1;
		}
		free((char*)structName);
		att->ta_type->u.st_template = structTemplate;
		_evlTmplIncRef(structTemplate);
		att->ta_type->tt_base_type = TY_STRUCT;
	}

	/*
	 * Note that this check is relevant even if we didn't execute the
	 * TY_STRUCTNAME block above.  We could have got our TY_STRUCT from a
	 * typedef.
	 */
	if (template && isStruct(att)) {
		structTemplate = att->ta_type->u.st_template;
	    	if (structTemplate != NULL
		    && (structTemplate->tm_flags & TMPL_TF_EATSALL) != 0) {
			/*
			 * This attribute will eat all the rest of
			 * the data, and so must be last.
			 */
			template->tm_flags |= TMPL_TF_EATSALL;
		}
	    	if (structTemplate != NULL
		    && (structTemplate->tm_flags & TMPL_TF_IMPLDIM) != 0) {
			if (template->tm_header.th_type == TMPL_TH_STRUCT) {
				/* IMPLDIM is contagious, like EATSALL. */
				template->tm_flags |= TMPL_TF_IMPLDIM;
			} else if (!isConstAtt(att)) {
				semanticError("struct attribute %s is not const but contains array(s) with implied dimension(s).", 
					att->ta_name);
				return -1;
			}
		}
	}
	return 0;
}

long
_evlTmplGetValueOfIntegerAttribute(const tmpl_attribute_t *att)
{
	switch (baseType(att)) {
	case TY_CHAR:
	case TY_SHORT:
	case TY_INT:
	case TY_LONG:
		return evl_getLongAttVal(att);
	case TY_UCHAR:
	case TY_USHORT:
	case TY_UINT:
	case TY_ULONG:
		return (long) evl_getUlongAttVal(att);
	case TY_LONGLONG:
		return (long) evl_getLonglongAttVal(att);
	case TY_ULONGLONG:
		return (long) evl_getUlonglongAttVal(att);
  default: /* keep gcc happy */;
	}
	assert(0);
	return 0;
}

static int
getDimensionFromAttribute(tmpl_attribute_t *dimAtt)
{
	if (!attExists(dimAtt)) {
		/* Dimension attribute doesn't exist in this record. */
		return 0;
	}
	return (int) _evlTmplGetValueOfIntegerAttribute(dimAtt);
}

static int
validateAttDimension(template_t *template, tmpl_attribute_t *att)
{
	tmpl_dimension_t *dim = att->ta_dimension;
	char *dimAttName;
	int variableDimension = 0;
	int simpleAttName;

	if (!dim) {
		return 0;
	}

	if (isBitField(att)) {
		/* Bit-field fussing will be done by validateAttBitField(). */
		return 0;
	}

	if (dim->td_type == TMPL_DIM_CONST) {
		if (dim->td_dimension <= 0) {
			semanticError("Dimension must be positive: %s",
				att->ta_name);
			return -1;
		}
		if (isConstAtt(att)) {
			dim->td_dimension2 = dim->td_dimension;
		}
		return 0;
	}

	/* Note that we allow 'typedef int intarray[] "(%d )";' */
	if (dim->td_type == TMPL_DIM_IMPLIED) {
		if (!isTypedef(att) && !isConstAtt(att)) {
			/*
			 * We used to always report an error here, but it
			 * turns out to be useful to have implied-dimension
			 * arrays in structs that are initialized later.
			 */
			assert(template != NULL);
			if (template->tm_header.th_type != TMPL_TH_STRUCT) {
				semanticError("Cannot infer dimension for non-const attribute %s",
					att->ta_name);
				return -1;
			}
			template->tm_flags |= TMPL_TF_IMPLDIM;
			att->ta_flags |= EVL_ATTR_IMPLDIM;
		}
		return 0;
	}

	if (dim->td_type == TMPL_DIM_ATTNAME) {
		dimAttName = dim->u.u_attname;
		simpleAttName = (strchr(dimAttName, '.') == NULL);
	} else {
		dimAttName = NULL;
	}
	if (dim->td_type == TMPL_DIM_REST) {
		variableDimension = 1;
		if (template) {
			template->tm_flags |= TMPL_TF_EATSALL;
		}
	} else if (dim->td_type != TMPL_DIM_ATTNAME) {
		assert(0);
	} else if (!strcmp(dimAttName, "_R_")) {
		dim->td_type = TMPL_DIM_REST;
		dim->td_dimension = -1;
		variableDimension = 1;
		if (template) {
			template->tm_flags |= TMPL_TF_EATSALL;
		}
	} else if (!strcmp(dimAttName, "_PREFIX3_")) {
		dim->td_type = TMPL_DIM_PREFIX3;
		dim->td_dimension = -1;
		variableDimension = 1;
	} else {
		tmpl_attribute_t *dimAtt;

		if (isTypedef(att)) {
			semanticError("typedef's dimension cannot be an attribute name: %s[%s]",
				att->ta_name, dimAttName);
			goto badDimAtt;
		}
		
		dimAtt = _evlTmplFindAttribute(template, dimAttName);
		if (dimAtt) {
			if (!isIntegerAtt(dimAtt)) {
				semanticError("Dimension attribute must be integer: %s",
					dimAttName);
				goto badDimAtt;
			}

			/* Plug in the value of a const attribute right now. */
			if (isConstAtt(dimAtt)) {
				dim->td_dimension = getDimensionFromAttribute(
					dimAtt);
				if (dim->td_dimension <= 0) {
					semanticError("Dimension must be positive: %s[%s]",
						att->ta_name, dimAttName);
					goto badDimAtt;
				}
				dim->td_type = TMPL_DIM_CONST;
			} else {
				/*
				 * For a simple attribute ref, we discard the
				 * name and remember the attribute pointer.
				 * Otherwise we keep the name and call
				 * _evlTmplFindAttribute() each time to find
				 * the attribute.
				 */
				variableDimension = 1;
				if (simpleAttName) {
					dim->td_type = TMPL_DIM_ATTR;
					dim->u.u_attribute = dimAtt;
					dim->td_dimension = -1;
				}
			}
		} else {
			semanticError("Unknown dimension attribute: %s",
				dimAttName);
			goto badDimAtt;
		}
	}
	if (variableDimension && isConstAtt(att)) {
		semanticError("Const attribute %s cannot have variable dimension",
			att->ta_name);
		return -1;
	}
	if (isConstAtt(att)) {
		dim->td_dimension2 = dim->td_dimension;
	}
	
	if (dimAttName && dim->td_type != TMPL_DIM_ATTNAME) {
		free(dimAttName);
	}
	return 0;

badDimAtt:
	/* Bad dimension attribute */
	assert(dim->td_type == TMPL_DIM_ATTNAME);
	// free(dimAttName);
	return -1;
}

static int
validateAttBitField(template_t *template, tmpl_attribute_t *att)
{
	int nBits, maxBits;
	assert(isBitField(att));

	maxBits = CHAR_BIT * _evlTmplTypeInfo[baseType(att)].ti_size;
	nBits = att->ta_dimension->td_dimension;
	if (nBits > maxBits) {
		semanticError("Bit-field %s cannot exceed %d bits",
			(att->ta_name ? att->ta_name : ""), maxBits);
		return -1;
	}
	if (nBits == 0 && att->ta_name) {
		semanticError("Zero-length bit-field cannot be named");
		return -1;
	}
	return 0;
}

static void
cantAssign(tmpl_base_type_t valType, tmpl_base_type_t varType)
{
	semanticError("Cannot assign %s to %s",
		_evlTmplTypeInfo[valType].ti_name, _evlTmplTypeInfo[varType].ti_name);
}

static int initScalarValue(tmpl_base_type_t attType, void *loc,
	tmpl_type_and_value_t *val);

/*
 * values points to a list of nElements values.  Allocate an array of objects
 * of type 'type', and copy the values from the list into the array.
 */
static void *
initConstScalarArray(tmpl_base_type_t type, int nElements, evl_list_t *values)
{
	/*
	 * Caller has verified that nElements is the number of values
	 * in the list.
	 */
	evl_listnode_t *p;
	int elemSize = _evlTmplTypeInfo[type].ti_size;
	size_t arraySizeBytes = nElements * elemSize;
	char *array = (char*) malloc(arraySizeBytes);
	char *a;
	int i;

	assert(array != NULL);
	for (p=values, i=0, a=array;
	    i<nElements;
	    p=p->li_next, i++, a+=elemSize) {
		if (-1 == initScalarValue(type, a,
		    (tmpl_type_and_value_t*) p->li_data)) {
			/* Bad initializer */
			free(array);
			return (void*) NULL;
		}
	}

	return (void*) array;
}

/*
 * stringList points to a list of values that had better be strings.
 * Allocate a buffer and pack the strings into the buffer just as they'd
 * appear in an event record.  Return a pointer to the buffer, or NULL on
 * failure.
 */
static char *
initConstStringArray(evl_list_t *stringList)
{
	evl_listnode_t *head = stringList;
	evl_listnode_t *p;
	char *sl, *sa, *array;
	size_t nBytes = 0;
	tmpl_type_and_value_t *tv;

	assert(head != NULL);

	/* Go through the list once to compute how big a buffer we need. */
	p = head;
	do {
		tv = (tmpl_type_and_value_t*) p->li_data;
		if (tv->tv_type != TY_STRING) {
			cantAssign(tv->tv_type, TY_STRING);
			return NULL;
		}
		sl = tv->tv_value.val_string;
		nBytes += strlen(sl) + 1;
		p = p->li_next;
	} while (p != head);

	array = (char*) malloc(nBytes);
	assert(array != NULL);

	/* Go through the list again, copying the strings into the buffer. */
	sa = array;
	p = head;
	do {
		tv = (tmpl_type_and_value_t*) p->li_data;
		sl = tv->tv_value.val_string;
		strcpy(sa, sl);
		sa += strlen(sa) + 1;
		p = p->li_next;
	} while (p != head);

	return array;
}

/* A shameless cut-and-paste of initConstStringArray(), adapted for wstrings */
static wchar_t *
initConstWstringArray(evl_list_t *stringList)
{
	evl_listnode_t *head = stringList;
	evl_listnode_t *p;
	wchar_t *sl, *sa, *array;
	size_t nBytes = 0;
	size_t nwc;
	tmpl_type_and_value_t *tv;

	assert(head != NULL);

	/* Go through the list once to compute how big a buffer we need. */
	p = head;
	do {
		tv = (tmpl_type_and_value_t*) p->li_data;
		if (tv->tv_type != TY_WSTRING) {
			cantAssign(tv->tv_type, TY_WSTRING);
			return NULL;
		}
		sl = tv->tv_value.val_wstring;
		nBytes += (wcslen(sl) + 1) * sizeof(wchar_t);
		p = p->li_next;
	} while (p != head);

	array = (wchar_t*) malloc(nBytes);
	assert(array != NULL);

	/* Go through the list again, copying the strings into the buffer. */
	sa = array;
	p = head;
	do {
		tv = (tmpl_type_and_value_t*) p->li_data;
		sl = tv->tv_value.val_wstring;
		nwc = wcslen(sl) + 1;
		memcpy(sa, sl, nwc*sizeof(wchar_t));
		sa += nwc;
		p = p->li_next;
	} while (p != head);

	return array;
}

static int initConstScalar(tmpl_attribute_t *att, tmpl_type_and_value_t *val);

/*
 * Attribute statt is a struct (or array of structs).  val is a list of values.
 * Clone a struct template of the appropriate type, and populate it with the
 * values in the list.  Return a pointer to the clone.
 */
static template_t *
initStructAtt(tmpl_attribute_t *statt, tmpl_type_and_value_t *val)
{
	evl_listnode_t *head, *end, *p;
	evl_listnode_t *v;
	tmpl_attribute_t *member;
	template_t *master, *clone;
	int nAtt, nVal;

	if (val->tv_type != TY_LIST) {
		semanticError("Struct %s requires initializer list",
			statt->ta_name);
		return NULL;
	}

	/* Look at the master struct and count the non-const attributes. */
	master = statt->ta_type->u.st_template;
	head = master->tm_attributes;
	nAtt = 0;
	for (end=NULL, p=head; p != end; end=head, p=p->li_next) {
		member = (tmpl_attribute_t*) p->li_data;
		if (! isConstAtt(member)) {
			nAtt++;
		}
	}

	nVal = _evlGetListSize(val->tv_value.val_list);
	if (nAtt != nVal) {
		semanticError("Struct %s requires %d initializers, but %d provided",
			statt->ta_name, nAtt, nVal);
		return NULL;
	}

	/* Make a clone of the master and populate it. */
	clone = _evlCloneTemplate(master);
	head = clone->tm_attributes;
	v = val->tv_value.val_list;
	for (end=NULL, p=head; p != end; end=head, p=p->li_next) {
		member = (tmpl_attribute_t*) p->li_data;
		if (! isConstAtt(member)) {
			/*
			 * A non-const member that wants a value.  Change its
			 * designation to const so that validateAttInitializer
			 * accepts it and it doesn't get re-populated later.
			 */
			member->ta_flags |= (EVL_ATTR_CONST|EVL_ATTR_EXISTS);
			if (validateAttInitializer(clone, member,
			    (tmpl_type_and_value_t*) v->li_data) == -1) {
				_evlFreeTemplate(clone);
				return NULL;
			}
			v = v->li_next;
		}
	}

	clone->tm_flags |= TMPL_TF_POPULATED;
	return clone;
}

/*
 * initializers is a list of one or more aggregate initializers.  statt
 * is an array of structs.  For each initializer, clone and populate a
 * struct with the initializer's values, and add the struct to tlist.
 * Return tlist, the list of initialized structs.  On failure, free up
 * tlist and return NULL.
 */
static evl_list_t *
initConstStructArray(tmpl_attribute_t *statt, evl_list_t *initializers)
{
	evl_list_t *tlist = NULL;
	template_t *st;
	evl_listnode_t *head = initializers, *end, *p;
	tmpl_type_and_value_t *val;

	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		val = (tmpl_type_and_value_t*) p->li_data;
		st = initStructAtt(statt, val);
		if (st) {
			tlist = _evlAppendToList(tlist, st);
		} else {
			goto badList;
		}
	}
	return tlist;

badList:
	/*
	 * Free structs already in the list.  This appears to be the most
	 * straightforward way.
	 */
	statt->ta_value.val_list = tlist;
	freeArrayOfStructs(statt);
	return NULL;
}

/*
 * If att is not a const struct, return -2.  Otherwise do appropriate
 * semantic checks and point att->ta_value.val_struct at the master.
 * (This breaks the old rule that val_struct never points to a master,
 * but there's really no reason to make a clone here.)  Returns 0 on
 * success, -1 on semantic error.
 */
static int
handleConstStructAtt(template_t *template, tmpl_attribute_t *att,
	tmpl_type_and_value_t *val)
{
	template_t *master;

	if (!isStruct(att)) {
		return -2;
	}
	master = att->ta_type->u.st_template;
	if (!isConstStructTmpl(master)) {
		return -2;
	}
	if (!isConstAtt(att)) {
		semanticError("Non-const attribute %s cannot be a const struct\n",
			att->ta_name);
		return -1;
	}
	if (val) {
		semanticError("Attribute %s: cannot re-initialize const struct %s\n",
			att->ta_name, master->tm_name);
		return -1;
	}
	if (isArray(att)) {
		semanticError("Attribute %s: arrays of const structs not supported\n",
			att->ta_name);
		return -1;
	}

	_evlTmplIncRef(master);
	att->ta_value.val_struct = master;
	return 0;
}

/*
 * If att is a const attribute and val specifies one or more values,
 * initialize att with those values.
 * NOTE: By convention, after being initialized, att doesn't share any
 * memory with val (e.g., strings are copied rather than shared).  This allows
 * val to be freed with a minimum of fuss.
 */
static int
validateAttInitializer(template_t *template, tmpl_attribute_t *att,
	tmpl_type_and_value_t *val)
{
	tmpl_dimension_t *dim;
	int status;

	status = handleConstStructAtt(template, att, val);
	if (status == 0 || status == -1) {
		return status;
	}

	if (isConstAtt(att)) {
		if (!val) {
			semanticError("Const attribute %s must be initialized",
				att->ta_name);
			return -1;
		}
	} else if (isTypedef(att)) {
		if (val) {
			semanticError("typedef  %s cannot be initialized",
				att->ta_name);
			return -1;
		}
		return 0;
	} else {
		if (val) {
			semanticError("Non-const attribute %s cannot be initialized",
				att->ta_name);
			return -1;
		}
		return 0;
	}

	dim = att->ta_dimension;
	if (isStruct(att) && !isArray(att)) {
		template_t *clone = initStructAtt(att, val);
		if (clone) {
			att->ta_value.val_struct = clone;
		} else {
			return -1;
		}
	} else if (isArray(att)) {
		/* const array with initializers */
		int nElements, nVals;

		if (val->tv_type != TY_LIST) {
			/*
			 * TODO: Consider allowing an array of [u]char to
			 * be initialized with a string.
			 */
			semanticError("Array %s requires initializer list",
				att->ta_name);
			return -1;
		}
		nVals = _evlGetListSize(val->tv_value.val_list);
		if (dim->td_type == TMPL_DIM_IMPLIED) {
			/* e.g., int x[] = {1,2,3}; */
			dim->td_type = TMPL_DIM_CONST;
			dim->td_dimension = nVals;
			dim->td_dimension2 = nVals;
		} else {
			assert(dim->td_type == TMPL_DIM_CONST);
		}
		nElements = dim->td_dimension;
		if (nElements != nVals) {
			semanticError("Wrong number of initializers for array %s",
				att->ta_name);
			return -1;
		}

		if (baseType(att) == TY_STRING) {
			att->ta_value.val_array = initConstStringArray(
				val->tv_value.val_list);
		} else if (baseType(att) == TY_WSTRING) {
			att->ta_value.val_array = initConstWstringArray(
				val->tv_value.val_list);
		} else if (isStruct(att)) {
			att->ta_value.val_list = initConstStructArray(att,
				val->tv_value.val_list);
		} else {
			att->ta_value.val_array = initConstScalarArray(
				baseType(att), nElements,
				val->tv_value.val_list);
		}
		if (att->ta_value.val_array == NULL) {
			return -1;
		}
	} else {
		/* Single initialized attribute */
		/* Note that this includes bit-fields. */
		if (val->tv_type == TY_LIST) {
			if (_evlGetListSize(val->tv_value.val_list) == 1) {
				/*
				 * Allow a 1-element initializer list.
				 * C allows this, too.
				 */
				val = (tmpl_type_and_value_t*)
					val->tv_value.val_list->li_data;
			} else {
				cantAssign(TY_LIST, baseType(att));
				return -1;
			}
		}

		if (initConstScalar(att, val) < 0) {
			return -1;
		}
	}
	return 0;
}

static int
stripParens(char *s) {
	size_t newlen;

	if (!_evlStartsWith(s, "(") || !_evlEndsWith(s, ")")) {
		return -1;
	}

	newlen = strlen(s) - 2;
	if (newlen != 0) {
		(void) memmove(s, s+1, newlen);
	}
	s[newlen] = '\0';
	return 0;
}

/*
 * s points to the character following the '%' in a format conversion spec.
 * Validate the conversion spec given the specified data type, and return the
 * number of characters in the conversion spec, not counting the '%'.
 * Returns -1 if there's an error in the conversion spec, or -2 if it's
 * otherwise OK but inappropriate given the attribute's type.
 */
static int
validateScalarFormat(const char *s, tmpl_base_type_t attType)
{
	struct evl_parsed_format pf;
	tmpl_base_type_t fmtType;
	int attSize, fmtSize, intSize;

	if (_evlParseFmtConvSpec(s, &pf) != 0) {
		/* Doesn't look like a conversion spec. */
		return -1;
	}
	if (pf.fm_array) {
		/* '[' in flags */
		return -1;
	}
	fmtType = _evlGetTypeFromConversion(&pf, 1, 0);
	if (fmtType == TY_NONE) {
		/* Unknown modifier and/or conversion */
		return -1;
	}

	if ((fmtType == TY_STRING && attType != TY_STRING)
	    || (fmtType == TY_WSTRING && attType != TY_WSTRING)) {
		return -2;
	}

	attSize = _evlTmplTypeInfo[attType].ti_size;
	fmtSize = _evlTmplTypeInfo[fmtType].ti_size;
	intSize = _evlTmplTypeInfo[TY_INT].ti_size;

	if (attSize == fmtSize
	    || (attType == TY_FLOAT && fmtType == TY_DOUBLE)
	    || (attSize < intSize && fmtSize == intSize)) {
		return pf.fm_length;
	}

	/* Type sizes don't match, and no promotions apply. */
	return -2;
}

/*
 * Set the attribute's format type to TMPL_AFS_CHARRSTR, and return 0, for
 * the following cases:
 *	array of char with %s format
 *	array of uchar with %s format
 *	array of wchar with %S or %ls format
 * Note that we accept variants such as "%-10s".
 * Returns -1 for a bad format specification, or -2 if it's not an array of
 * characters.
 */
static int
formatCharArrAsString(tmpl_attribute_t *att)
{
	tmpl_base_type_t type = baseType(att);
	char *fstr = att->ta_format.af_format;
	struct evl_parsed_format pf;
	int ok = 0;

	assert(isArray(att));
	if (type != TY_CHAR && type != TY_UCHAR && type != TY_WCHAR) {
		return -2;
	}
	if (fstr[0] != '%') {
		return -1;
	}
	if (_evlParseFmtConvSpec(fstr+1, &pf) != 0) {
		/* fstr doesn't contain a valid conversion spec. */
		return -1;
	}
	if (pf.fm_length != strlen(fstr+1)) {
		/* fstr contains something beyond the conversion spec. */
		return -1;
	}
	switch (type) {
	case TY_CHAR:
	case TY_UCHAR:
		if (pf.fm_conversion == 's' && !strcmp(pf.fm_modifier, "")) {
			ok = 1;
		}
		break;
	case TY_WCHAR:
		if (pf.fm_conversion == 'S' && !strcmp(pf.fm_modifier, "")) {
			ok = 1;
		}
		if (pf.fm_conversion == 's' && !strcmp(pf.fm_modifier, "l")) {
			ok = 1;
		}
		break;
  default: /* keep gcc happy */;
	}
	if (ok) {
		att->ta_format.af_type = TMPL_AFS_CHARRSTR;
		return 0;
	}
	return -1;
}

static int
validateAttDelimiter(template_t *template, tmpl_attribute_t *att)
{
	tmpl_delimiter_t *de = att->ta_delimiter;
	char *delimAttName;
	tmpl_attribute_t *delimAtt;

	if (!de) {
		return 0;
	}
	if (! isArray(att)) {
		semanticError("Cannot specify a delimiter for scalar %s", att->ta_name);
		return -1;
	}
	if (de->de_type == TMPL_DELIM_CONST) {
		return 0;
	}

	assert (de->de_type == TMPL_DELIM_ATTNAME);
	delimAttName = de->u.u_attname;

	if (isTypedef(att)) {
		semanticError("typedef's delimiter cannot be an attribute name: %s, %s",
			att->ta_name, delimAttName);
		return -1;
	}
	
	delimAtt = _evlTmplFindAttribute(template, delimAttName);
	if (delimAtt) {
		if (baseType(delimAtt) != TY_STRING || isArray(delimAtt)) {
			semanticError("Delimiter attribute must be a string: %s",
				delimAttName);
			return -1;
		}

		/* Plug in the value of a const attribute right now. */
		if (isConstAtt(delimAtt)) {
			de->de_delimiter = strdup(evl_getStringAttVal(delimAtt));
			de->de_type = TMPL_DELIM_CONST;
			free(delimAttName);
			de->u.u_attname = NULL;
		} else {
			/*
			 * For a simple attribute ref, we discard the
			 * name and remember the attribute pointer.
			 * Otherwise we keep the name and call
			 * _evlTmplFindAttribute() each time to find
			 * the attribute.
			 */
			if (!strchr(delimAttName, '.')) {
				free(delimAttName);
				de->de_type = TMPL_DELIM_ATTR;
				de->u.u_attribute = delimAtt;
			}
		}
	} else {
		semanticError("Unknown delimiter attribute: %s", delimAttName);
		return -1;
	}

	/* Note: delimAttName (AKA de->u.u_attname) may be freed by now. */
	if (de->de_type != TMPL_DELIM_CONST && isConstAtt(att)) {
		semanticError("Const attribute %s cannot have variable delimiter",
			att->ta_name);
		return -1;
	}
	return 0;
}

static int
validateAttFormat(template_t *template, tmpl_attribute_t *att)
{
	char *fstr;
	
	if (att->ta_format.af_type == TMPL_AFS_DEFAULT) {
		/*
		 * No format specified.  The default format for arrays of
		 * structs is %Z.  Otherwise, we're done.
		 */
		if (isArray(att) && isStruct(att)) {
			att->ta_format.af_format = strdup("%Z");
			att->ta_format.af_type = TMPL_AFS_TBD;
		} else {
			return 0;
		}
	}

	fstr = att->ta_format.af_format;
	if (att->ta_format.af_type != TMPL_AFS_TBD) {
		/*
		 * applyTypedefIfAny() must have given us the typedef's
		 * format.
		 */
		assert(att->ta_format.af_shared);
		return 0;
	}

	/* Grammar should guarantee this: */
	assert(fstr != NULL);
	assert(att->ta_name != NULL);

	if (!strcmp(fstr, "%t")) {
		free(fstr);
		att->ta_format.af_format = NULL;
		att->ta_format.af_type = TMPL_AFS_DUMP;
		if (isArray(att) && isStruct(att) && isConstAtt(att)) {
			semanticError("Cannot specify dump format for const array of structs %s",
				att->ta_name);
			return -1;
		}
		if (att->ta_delimiter) {
			semanticError("Cannot specify delimiter with dump format: %s",
				att->ta_name);
			return -1;
		}
		return 0;
	}

	if (isArray(att)) {
		/* TODO: Consider allowing %v and %b in array fmt specs. */

		char *izorder = att->ta_format.u.u_izorder;
		char *c;
		int foundI = 0, foundZ = 0, foundS = 0;
		int flen;

		/* Check to see if af_format was specified */

		/* Strip ( ), if any, from original string. */
		
		if (stripParens(fstr) < 0) {
			/* Format not enclosed in parens. */
			if (formatCharArrAsString(att) == 0) {
				if (att->ta_delimiter != NULL) {
					semanticError("Cannot specify a delimiter for a char array %s with string-type format", att->ta_name);
					return -1;
				}
				return 0;
			}
		} else {
			/*
			 * Format enclosed in parens.  Delimiter defaults to
			 * empty string, not space.
			 */
			if (att->ta_delimiter == NULL) {
				att->ta_delimiter = _evlTmplAllocDelimiter();
				att->ta_delimiter->de_type = TMPL_DELIM_CONST;
				att->ta_delimiter->de_delimiter = strdup("");
			}
		}
		
		/* Hunt for scalar conversion spec and/or %I and/or %Z. */
		izorder[0] = '\0';
		for (c = fstr; *c; c++) {
			if (*c != '%') {
				continue;
			}

			/* Found a '%'. */
			c++;
			switch (*c) {
			case '%':
				/* Skip over %%. */
				break;
			case 'I':
				/* Replace %I with %d. */
				if (foundI) {
					semanticError(
"Cannot specify %%I more than once in format for array %s", att->ta_name);
					return -1;
				}
				foundI = 1;
				*c = 'd';
				(void) strcat(izorder, "I");
				break;
			case 'Z':
				/* Replace %Z with %s. */
				if (foundZ) {
					semanticError(
"Cannot specify %%Z more than once in format for array %s", att->ta_name);
					return -1;
				}
				if (!isStruct(att)) {
					semanticError(
"Attribute %s is not an array of structs, so %%Z not permitted in format", att->ta_name);
					return -1;
				}
				foundZ = 1;
				*c = 's';
				(void) strcat(izorder, "Z");
				break;
			case '\0':
				semanticError(
"Format string for %s ends with %%", att->ta_name);
				return -1;
			default:
				/*
				 * Must be the conversion spec for
				 * the array element itself.
				 */
				if (foundS) {
					semanticError(
"Too many conversion specifiers in format for %s", att->ta_name);
					return -1;
				}
				if (isStruct(att)) {
					semanticError(
"Attribute %s is an array of structs, so %%Z and %%I are the only permitted conversion specifiers", att->ta_name);
					return -1;
				}
				foundS = 1;
				(void) strcat(izorder, "S");
				flen = validateScalarFormat(c, baseType(att));
				if (flen < 0) {
					/* %2% or some such */
					semanticError(
"Malformed or inappropriate format string for %s", att->ta_name);
					return -1;
				} else {
					c += flen-1;
				}
				break;
			}
		}
		/* Finished parsing string */
		if (!foundZ && !foundS) {
			semanticError(
"Incomplete format string for %s", att->ta_name);
			return -1;
		}

		if (foundZ || foundI) {
			att->ta_format.af_type = TMPL_AFS_IZARRAY;
		} else {
			att->ta_format.af_type = TMPL_AFS_ARRAY;
		}
		/* End of processing format for array attribute */
	} else {
		/* Should be a scalar attribute. */
		if (isStruct(att)) {
			semanticError("Cannot specify format for struct attribute %s",
				att->ta_name);
			return -1;
		}

		if (_evlStartsWith(fstr, "%b")) {
			tmpl_bitmap_t *bmaps = _evlTmplCollectBformat(fstr, 1, NULL);
			if (!bmaps) {
				return -1;
			}
			if (!isIntegerAtt(att)) {
				semanticError(
"Cannot use %%b format with non-integer attribute %s", att->ta_name);
				return -1;
			}
			att->ta_format.af_type = TMPL_AFS_BITMAP;
			att->ta_format.u.u_bitmaps = bmaps;
			/* Do NOT free fstr here. */
		} else if (_evlStartsWith(fstr, "%v")) {
			tmpl_bitmap_t *bmaps = _evlTmplCollectVformat(fstr, 1, NULL);
			if (!bmaps) {
				return -1;
			}
			if (!isIntegerAtt(att)) {
				semanticError(
"Cannot use %%v format with non-integer attribute %s", att->ta_name);
				return -1;
			}
			att->ta_format.af_type = TMPL_AFS_VALNM;
			att->ta_format.u.u_bitmaps = bmaps;
			/* Do NOT free fstr here. */
		} else {
			size_t flen;
			if (!_evlStartsWith(fstr, "%")
			    || (flen = validateScalarFormat(fstr+1, baseType(att))) < 0
			    || flen != strlen(fstr)-1) {
				semanticError(
"Malformed or inappropriate format string for %s", att->ta_name);
				return -1;
			}
			att->ta_format.af_type = TMPL_AFS_SCALAR;
		}
	}
	return 0;
}

/*
 * Free the initializer for the attribute statement currently being parsed.
 * By convention, the tmpl_attribute_t object doesn't keep pointers to
 * anything in this initializer list, so we call free everything.
 */
static void
freeInitializer(tmpl_type_and_value_t *tv)
{
	if (tv == NULL) {
		return;
	}

	switch (tv->tv_type) {
	default:
		break;
	case TY_STRING:
		free(tv->tv_value.val_string);
		break;
	case TY_LIST:
	    {
		evl_listnode_t *head, *end, *p;
		head = tv->tv_value.val_list;
		for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
			freeInitializer((tmpl_type_and_value_t*) p->li_data);
		}
		break;
	    }
	}

	free(tv);
}

/* Called by _evlTmplAddAttribute before adding the attribute. */
static int
check_R_notLast(template_t *template) {
	if ((template->tm_flags & TMPL_TF_EATSALL) != 0) {
		semanticError("Attribute with dimension [_R_] must be last.");
		return -1;
	}
	return 0;
}

/* Checks for must-be-aligned attribute in an unaligned record/struct. */
static int
validateNonalignment(template_t *template, tmpl_attribute_t *att)
{
	if (isBitField(att)) {
		semanticError("Attribute %s: unaligned record/struct cannot contain bit-fields.",
			att->ta_name);
		return -1;
	}
	return 0;
}

/* Checks for un-alignable attribute in an aligned record/struct. */
static int
validateAlignment(template_t *template, tmpl_attribute_t *att)
{
	tmpl_base_type_t type;

	if (!template || (template->tm_flags & TMPL_TF_ALIGNED) == 0) {
		return validateNonalignment(template, att);
	}

	if (isStruct(att)) {
		const template_t *stmpl = att->ta_type->u.st_template;
		assert(stmpl != NULL);
		if ((stmpl->tm_flags & TMPL_TF_ALIGNED) == 0) {
			semanticError("Attribute %s: aligned record/struct cannot contain unaligned struct.",
				att->ta_name);
			return -1;
		}
		/* TODO: Handle array of structs. */
	}

	if (isArray(att)) {
		if (att->ta_dimension->td_type != TMPL_DIM_CONST) {
			semanticError("Attribute %s: aligned record/struct cannot contain variable-length array.",
				att->ta_name);
			return -1;
		}
	}
	
	type = baseType(att);
	if (type == TY_STRING || type == TY_WSTRING) {
		semanticError("Aligned record/struct cannot contain variable-length attribute %s.",
			att->ta_name);
		return -1;
	}
	return 0;
}

/*
 * If the type of attribute att is a typedef, apply the typedef's parameters
 * to the attribute:
 *	- The typedef's type is always applied.
 *	- The typedef's dimension (if any) is always applied.  It is an error
 *	for both the typedef and the attribute to specify a dimension.
 *	- The typedef's format (if any) is applied only if the attribute
 *	doesn't specify one.
 */
static int
applyTypedefIfAny(template_t *template, tmpl_attribute_t *att)
{
	tmpl_attribute_t *td;
	tmpl_data_type_t *aty = att->ta_type;
	tmpl_dimension_t *adim, *tdim;
	tmpl_delimiter_t *ade, *tde;

	if (baseType(att) != TY_TYPEDEF) {
		return 0;
	}

	assert(att->ta_name != NULL);
	td = findTypedef(aty->u.td_name);
	if (!td) {
		semanticError("Unknown type %s for attribute %s.",
			aty->u.td_name, att->ta_name);
		return -1;
	}

	/*
	 * att is an attribute whose type and (possibly) other parameters
	 * are to be taken from typedef td.
	 */

	/* Copy td's type to att. */
	free(aty->u.td_name);
	memcpy(aty, td->ta_type, sizeof(tmpl_data_type_t));
	if (isStruct(att)) {
		extern void _evlRegisterStructRef(template_t *st);

		_evlTmplIncRef(aty->u.st_template);
		_evlRegisterStructRef(aty->u.st_template);
	}

	/* Copy td's dimension (if any) to att. */
	adim = att->ta_dimension;
	tdim = td->ta_dimension;
	if (adim) {
		if (tdim) {
			semanticError("Attribute %s: array of arrays not supported.",
				att->ta_name);
			return -1;
		}
	} else if (tdim) {
		adim = _evlTmplAllocDimension();
		att->ta_dimension = adim;
		memcpy(adim, tdim, sizeof(tmpl_dimension_t));
	}

	/* typedefs don't have initializers.  Nothing to do there.  */

	/*
	 * If the typedef specifies a format and the attribute doesn't,
	 * make a shallow copy of the typedef's format.  (It's just too
	 * ugly to contemplate copying all the stuff associated with a
	 * %v or %b format.)
	 */
	if (att->ta_format.af_type == TMPL_AFS_DEFAULT
	  && td->ta_format.af_type != TMPL_AFS_DEFAULT) {
		memcpy(&att->ta_format, &td->ta_format,
			sizeof(tmpl_att_format_t));
		att->ta_format.af_shared = 1;
	}

	/* A typedef's delimiter is either omitted or a string literal. */
	ade = att->ta_delimiter;
	tde = td->ta_delimiter;
	if (ade == NULL && tde != NULL) {
		ade = _evlTmplAllocDelimiter();
		assert (tde->de_type == TMPL_DELIM_CONST);
		ade->de_type = tde->de_type;
		ade->de_delimiter = strdup(tde->de_delimiter);
		att->ta_delimiter = ade;
	}

	return 0;
}

static void freeAttribute(template_t *t, tmpl_attribute_t *att);

/*
 * The parser has given us the indicated info about a new attribute.
 * Validate and normalize the info and add the attribute to the indicated
 * template.
 *
 * This function also handles typedef definitions, which have similar syntax
 * and semantics.  For typedefs, attrClass = EVL_ATTR_TYPEDEF, and the
 * template pointer is set to NULL.
 */
int
_evlTmplAddAttribute(template_t *template, int attrClass,
	tmpl_data_type_t *type,
	const char *name, tmpl_dimension_t *dim,
	tmpl_type_and_value_t *val, char *format, tmpl_delimiter_t *delimiter)
{
	int constAttr = (attrClass == EVL_ATTR_CONST);
	int typedefAttr = (attrClass == EVL_ATTR_TYPEDEF);
	tmpl_attribute_t *att = _evlTmplAllocAttribute();

	/* dumpAttribute(template, constAttr, type, name, dim, val, format); */

	if (constAttr) {
		att->ta_flags = EVL_ATTR_CONST | EVL_ATTR_EXISTS;
	} else if (typedefAttr) {
		att->ta_flags = EVL_ATTR_TYPEDEF;
		template = NULL;
	} else {
		att->ta_flags = 0;
	}
	att->ta_name = name;
	att->ta_type = type;
	att->ta_dimension = dim;
	att->ta_format.af_format = format;
	att->ta_format.af_type = (format ? TMPL_AFS_TBD : TMPL_AFS_DEFAULT);
	att->ta_delimiter = delimiter;

	if (dim && dim->td_type == TMPL_DIM_BITFIELD) {
		att->ta_flags |= EVL_ATTR_BITFIELD;
		if (constAttr) {
			semanticError("Const attribute %s cannot be bit-field",
				(name ? name : ""));
			goto badAttribute;
		}
		if (typedefAttr) {
			semanticError("typedef %s cannot be bit-field",
				(name ? name : ""));
			goto badAttribute;
		}
	}

	if (applyTypedefIfAny(template, att) < 0) {
		goto badAttribute;
	}

	if (template && check_R_notLast(template) < 0) {
		goto badAttribute;
	}

	if (validateAttName(template, att) < 0) {
		goto badAttribute;
	}

	if (validateAttType(template, att) < 0) {
		goto badAttribute;
	}

	if (validateAttDimension(template, att) < 0) {
		goto badAttribute;
	}

	if (isBitField(att) && validateAttBitField(template, att) < 0) {
		goto badAttribute;
	}

	if (validateAttInitializer(template, att, val) < 0) {
		goto badAttribute;
	}

	if (validateAttDelimiter(template, att) < 0) {
		goto badAttribute;
	}

	if (validateAttFormat(template, att) < 0) {
		goto badAttribute;
	}

	if (validateAlignment(template, att) < 0) {
		goto badAttribute;
	}

	/* Looks like a valid attribute.  Add it to the list. */
	if (isTypedef(att)) {
		typedefs = _evlAppendToList(typedefs, att);
	} else {
		assert(template != NULL);
		att->ta_index = _evlGetListSize(template->tm_attributes);
		template->tm_attributes =
			_evlAppendToList(template->tm_attributes, att);
	}
	freeInitializer(val);
	return 0;

badAttribute:
	if (template) {
		template->tm_flags |= TMPL_TF_ERROR;
	}
	freeInitializer(val);
	freeAttribute(template, att);
	return -1;
}

void
_evlTmplAddFormatSpec(template_t *template, char *fmtSpec)
{
	template->tm_format = fmtSpec;
	/* _evlTmplDprintf("Format spec =\n%s\n", fmtSpec); */

	if (template->tm_header.th_type == TMPL_TH_STRUCT) {
		/*
		 * By convention, we remove the trailing newline, if any,
		 * from the format specification of a struct template.
		 */
		int flen = strlen(fmtSpec);
		if (flen > 0 && fmtSpec[flen-1] == '\n') {
			fmtSpec[flen-1] = '\0';
		}
	}

	template->tm_parsed_format = _evlTmplParseFormat(template, fmtSpec);
	if (template->tm_parsed_format == NULL) {
		template->tm_flags |= TMPL_TF_ERROR;
	}
}

/*
 * Compute the alignment for the indicated attribute -- e.g., 1 for char,
 * 2 for short, 4 for bigger types.
 */
static int
computeAttAlignment(tmpl_attribute_t *att, int arch)
{
	if (isStruct(att)) {
		return att->ta_type->u.st_template->tm_alignment;
	} else {
		tmpl_base_type_t bt = baseType(att);
		//return _evlTmplTypeInfo[bt].ti_align;
		return _evlTmplArchTypeInfo[arch][bt].ti_align;
	}
}

/*
 * If this is an aligned template, compute the record's alignment as the
 * max of the alignments of the individual non-const components.  Note that
 * this is done once, after the template has been parsed.  To get the
 * alignment of a struct-type attribute, use computeAttAlignment().
 *
 * Note that alignment of any aligned template is always at least 1 (per
 * alignDataPtr()'s assert), even if the template contains only unnamed
 * bit-fields.  align=1 means unaligned, anyway.
 *
 * This function also computes the template's minimum size, which is non-zero
 * if the template contains any named bit-fields.
 */
static void
computeTmplAlignment(template_t *t)
{
	int tmplAlign = 1;
	int tmplMinSize = 0;
	evl_listnode_t *head = t->tm_attributes, *end, *p;

	if ((t->tm_flags & TMPL_TF_ALIGNED) == 0) {
		t->tm_alignment = 0;
		return;
	}

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		tmpl_attribute_t *att = (tmpl_attribute_t*) p->li_data;
		int attAlign;
		if (isConstAtt(att)) {
			continue;
		}
		if (att->ta_name == NULL) {
			/*
			 * Unnamed bit-fields do not affect a struct's
			 * alignment or minimum size.
			 */
			continue;
		} else if (isBitField(att)) {
			size_t bfSize = _evlTmplArchTypeInfo[t->tm_arch][baseType(att)].ti_size;
			if (tmplMinSize < bfSize) {
				tmplMinSize = bfSize;
			}
		}
		attAlign = computeAttAlignment(att, t->tm_arch);
		assert(attAlign > 0);
		if (tmplAlign < attAlign) {
			tmplAlign = attAlign;
		}
	}
	t->tm_alignment = (short) tmplAlign;
	t->tm_minsize = (short) tmplMinSize;
}

static void
flagEmptyStruct(template_t *t)
{
	evl_listnode_t *head, *p, *end;
	int anyDataAtts = 0;
	int anyData = 0;
	if (!t || t->tm_header.th_type != TMPL_TH_STRUCT) {
		return;
	}
	head = t->tm_attributes;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		tmpl_attribute_t *att = (tmpl_attribute_t*) p->li_data;
		if (isConstAtt(att)) {
			continue;
		}
		anyDataAtts = 1;
		if (!isBitField(att)
		    || att->ta_dimension->td_dimension != 0) {
			anyData = 1;
		}
	}
	if (isConstStructTmpl(t)) {
		if (anyDataAtts) {
			semanticError("const struct %s has non-const attributes.",
				t->tm_name);
		}
		/*
		 * Note that we don't flag an error if a const struct
		 * has no attributes at all.  After all, it could
		 * have a meaningful format.
		 */
	} else {
		if (!anyDataAtts) {
			semanticError("struct %s has no data attributes but is not declared const.",
				t->tm_name);
		} else if (!anyData) {
			semanticError("struct %s has size zero.", t->tm_name);
		}
	}
}

void
_evlTmplWrapup(template_t *t)
{
	flagEmptyStruct(t);
	computeTmplAlignment(t);
}

static void
freeTmplHeader(template_t *t)
{
	tmpl_header_t *th = &t->tm_header;

	if (!isMasterTmpl(t)) {
		return;
	}
	free((char*) th->th_description);

	/*
	 * Note that t->tm_name points to either th->u.u_struct.st_name or
	 * the event-template name manufactured in _evlMakeEvrecTemplate().
	 */
	free((char*) t->tm_name);
}

static void
freeTmplRefs(template_t *t)
{
	if (!isMasterTmpl(t)) {
		return;
	}

	if ((t->tm_flags & TMPL_TF_IMPORTED) != 0) {
		/*
		 * This template was created by importing rather than parsing.
		 * Therefore, it owns the tmpl_struct_ref_t objects pointed
		 * to by its imports list.
		 */
		evl_listnode_t *head, *end, *p;
		tmpl_struct_ref_t *sref;

		for (head=t->tm_imports, end=NULL, p=head; p!=end;
		    end=head, p=p->li_next) {
			sref = (tmpl_struct_ref_t*) p->li_data;
			free((char*)sref->sr_path);
			free(sref);
		}
	}
	_evlFreeList(t->tm_imports, 0);
}

static tmpl_attribute_t *
cloneAttribute(tmpl_attribute_t *att, template_t *tclone)
{
	tmpl_attribute_t *clone;
	tmpl_dimension_t *dim, *cldim;
	tmpl_delimiter_t *de, *clde;

	if (isConstAtt(att)) {
		return att;
	}

	clone = _evlTmplAllocAttribute();

	/*
	 * Copy everything from the original to the clone; then fix up the
	 * exceptions.  Note that we never clone a populated template, so
	 * we never clone the ta_value member.
	 */
	memcpy(clone, att, sizeof(tmpl_attribute_t));
	clone->ta_flags |= EVL_ATTR_CLONE;

	dim = att->ta_dimension;
	if (dim && dim->td_type == TMPL_DIM_ATTR) {
		/* Dimension is an attribute in the local template */
		tmpl_attribute_t *dimAtt = dim->u.u_attribute;
		cldim = _evlTmplAllocDimension();
		memcpy(cldim, dim, sizeof(tmpl_dimension_t));
		cldim->u.u_attribute = _evlTmplGetNthAttribute(tclone, dimAtt->ta_index);
		clone->ta_dimension = cldim;
	} else if (dim && dim->td_type == TMPL_DIM_IMPLIED) {
		/*
		 * Array with implied dimension in non-const struct.
		 * td_type will morph to TMPL_DIM_CONST.
		 */
		assert((att->ta_flags & EVL_ATTR_IMPLDIM) != 0);
		cldim = _evlTmplAllocDimension();
		cldim->td_type = TMPL_DIM_IMPLIED;
		cldim->td_dimension = 0;
		clone->ta_dimension = cldim;
	}

	de = att->ta_delimiter;
	if (de && de->de_type == TMPL_DELIM_ATTR) {
		/* Delimiter is an attribute in the local template */
		tmpl_attribute_t *delimAtt = de->u.u_attribute;
		clde = _evlTmplAllocDelimiter();
		clde->de_type = de->de_type;
		clde->u.u_attribute = _evlTmplGetNthAttribute(tclone,
			delimAtt->ta_index);
		clone->ta_delimiter = clde;
	}

	return clone;
}

/* Free an attribute in a depopulated template. */
static void
freeAttribute(template_t *t, tmpl_attribute_t *att)
{
	int isMaster;
	
	if (t) {
		isMaster = isMasterTmpl(t);
	} else {
		/*
		 * No template.  att must be a typedef.  Certainly no other
		 * template owns att.
		 */
		isMaster = 1;
	}

	if (!isMaster && !isClonedAtt(att)) {
		/* att points to the master template's copy. */
		return;
	}

	/*
	 * Discard the attribute's value first, and dimension and type
	 * later, since decisions about the former depend on the latter.
	 */

	/*
	 * Note that an attribute can be const even though it's cloned.
	 * This is the case when the const-attributes section declares a
	 * struct with an initializer list.  For that clone of the
	 * struct's template, all the attributes are made const.
	 */
	if (isConstAtt(att)) {
		if (baseType(att) == TY_STRING) {
			free(att->ta_value.val_string);
		} else if (isArray(att)) {
			if (isStruct(att)) {
				freeArrayOfStructs(att);
			} else {
				free(att->ta_value.val_array);
			}
		}
		/*
		 * Note: Structs (but not arrays thereof) are handled below.
		 */
	}

	/*
	 * If we get here and our val_struct points to a template, then we
	 * own it.  Either the template is populated with const values, or
	 * it's depopulated...
	 *
	 * ... unless att is a const struct, in which case val_struct points
	 * to the master and we just decrement the ref count.
	 */
	if (isStruct(att) && !isArray(att)) {
		template_t *st = att->ta_value.val_struct;
		if (st) {
			if (isMasterTmpl(st)) {
				_evlTmplDecRef(st);
			} else {
				_evlFreeTemplate(st);
			}
		}
	}

	/*
	 * Format.  Even for a master template, the guts of an attribute's
	 * format may be owned by a typedef, in which case af_shared==1.
	 */
	if (isMaster && !att->ta_format.af_shared) {
		switch(att->ta_format.af_type) {
		case TMPL_AFS_TBD:
		case TMPL_AFS_SCALAR:
		case TMPL_AFS_ARRAY:
		case TMPL_AFS_IZARRAY:
		case TMPL_AFS_CHARRSTR:
			free(att->ta_format.af_format);
			break;
		case TMPL_AFS_BITMAP:
		case TMPL_AFS_VALNM:
		    {
			tmpl_bitmap_t *b, *bmap = att->ta_format.u.u_bitmaps;
			char *nameGlob = (char*) att->ta_format.af_format;
			if (nameGlob) {
				/*
				 * This template was created by the parser.
				 * Freeing att->ta_format.af_format will free
				 * all the names.
				 */
				free(nameGlob);
			} else {
				/*
				 * This template was read from a binary file.
				 * Names were individually strduped.
				 */
				for (b = bmap; b->bm_name; b++) {
					free(b->bm_name);
				}
			}
			free(bmap);
			break;
		    }
		case TMPL_AFS_DEFAULT:
		case TMPL_AFS_DUMP:
			break;
		}
	}

	if (isMaster) {
		/* dimension */
		tmpl_dimension_t *dim = att->ta_dimension;
		if (dim) {
			if (dim->td_type == TMPL_DIM_ATTNAME) {
				free(dim->u.u_attname);
			}
			free(dim);
		}

		/* type */
		if (isStruct(att)) {
			_evlTmplDecRef(att->ta_type->u.st_template);
		} else if (baseType(att) == TY_STRUCTNAME) {
			/* Never got as far as converting name to ref. */
			free((char*)att->ta_type->u.st_name);
		}
		free(att->ta_type);

		/* delimiter */
		if (att->ta_delimiter) {
			tmpl_delimiter_t *de = att->ta_delimiter;
			switch (de->de_type) {
			case TMPL_DELIM_CONST:
				free(de->de_delimiter);
				break;
			case TMPL_DELIM_ATTNAME:
				free(de->u.u_attname);
				break;
			case TMPL_DELIM_ATTR:
				break;
      default: /* keep gcc happy */ ;
			}
			free(de);
		}

		free((char*) att->ta_name);
	} else {
		/* This attribute is a clone of the master. */
		tmpl_dimension_t *dim;
		tmpl_delimiter_t *de;
		
		/* The dimension is a copy if it's an attribute ref. */
		dim = att->ta_dimension;
		if (dim && dim->td_type == TMPL_DIM_ATTR) {
			free(dim);
		}
		/* It's also a copy if the dim was implied in the master. */
		if (dim && dim->td_type == TMPL_DIM_CONST
		    && (att->ta_flags & EVL_ATTR_IMPLDIM) != 0) {
			free(dim);
		}

		/* Ditto attribute ref handling for the delimiter. */
		de = att->ta_delimiter;
		if (de && de->de_type == TMPL_DELIM_ATTR) {
			free(de);
		}
	}

	free(att);
}

static void
freeTmplAttributes(template_t *t)
{
	evl_list_t *head = t->tm_attributes;
	evl_listnode_t *p = head;

	if (!head) {
		return;
	}

	do {
		tmpl_attribute_t *att = (tmpl_attribute_t*) p->li_data;
		freeAttribute(t, att);
		p = p->li_next;
	} while (p != head);
	_evlFreeList(head, 0);
}

static void
freeTmplFormat(template_t *t)
{
	evl_list_t *head = t->tm_parsed_format;
	evl_listnode_t *p = head;
	int isMaster = isMasterTmpl(t);

	if (!head) {
		return;
	}

	do {
		evl_fmt_segment_t *seg = (evl_fmt_segment_t*) p->li_data;
		if (isMaster || seg->fs_type == EVL_FS_ATTR) {
			/*
			 * Note that if this is a string segment,
			 * seg->fs_string is NOT strdup-ed.  It points into
			 * t->tm_format.
			 */
			if (seg->fs_type == EVL_FS_ATTNAME) {
				/* The name IS strdup-ed.  See util/format.c. */
				free(seg->u.fs_attname);
			}
			free(seg);
		}
		p = p->li_next;
	} while (p != head);
	_evlFreeList(head, 0);
}

static void
freeRedirectedAtt(struct redirectedAttribute *ra)
{
	if (ra) {
		if (ra->type == TMPL_RD_ATTNAME) {
			free (ra->u.attname);
		}
		free(ra);
	}
}

static void
freeTmplRedirection(template_t *t)
{
	tmpl_redirection_t *rd = t->tm_redirection;
	if (t->tm_master) {
		/* t is a clone. */
		return;
	}
	if (rd) {
		freeRedirectedAtt(rd->rd_fac);
		freeRedirectedAtt(rd->rd_evtype);
	}
}

template_t *
_evlCloneTemplate(template_t *t)
{
	template_t *clone;
	evl_list_t *head;
	evl_listnode_t *p;

	assert(isMasterTmpl(t));
	assert(! isPopulatedTmpl(t));
	_evlTmplIncRef(t);

	clone = _evlAllocTemplate();
	memcpy(clone, t, sizeof(template_t));
	clone->tm_master = t;
	clone->tm_ref_count = 0;
#ifdef _POSIX_THREADS
	(void) pthread_mutex_init(&t->tm_mutex, NULL);
#endif

	/* Clone attributes. */
	clone->tm_attributes = NULL;
	head = t->tm_attributes;
	if (head) {
		p = head;
		do {
			tmpl_attribute_t *att = (tmpl_attribute_t*) p->li_data;
			clone->tm_attributes = _evlAppendToList(
				clone->tm_attributes,
				cloneAttribute(att, clone));
			p = p->li_next;
		} while (p != head);
	}

	/* Clone format segments. */
	clone->tm_parsed_format = NULL;
	head = t->tm_parsed_format;
	if (head) {
		p = head;
		do {
			evl_fmt_segment_t *seg = (evl_fmt_segment_t*)
				p->li_data;
			if (seg->fs_type == EVL_FS_ATTR) {
				/*
				 * This segment contains an attribute ref,
				 * and so can't be shared.
				 */
				evl_fmt_segment_t *clseg =
					_evlAllocFormatSegment();
				clseg->fs_type = seg->fs_type;
				clseg->fs_userfmt = seg->fs_userfmt;
				clseg->u2.fs_attribute = _evlTmplGetNthAttribute(clone,
					seg->u2.fs_attribute->ta_index);
				clone->tm_parsed_format = _evlAppendToList(
					clone->tm_parsed_format, clseg);
			} else {
				/* This segment can be shared. */
				clone->tm_parsed_format = _evlAppendToList(
					clone->tm_parsed_format, seg);
			}
			p = p->li_next;
		} while (p != head);
	}
	return clone;
}

static int depopulateTemplate(template_t *template);

void
_evlFreeTemplate(template_t *t)
{
	assert(t->tm_ref_count == 0);

	depopulateTemplate(t);
	freeTmplHeader(t);
	freeTmplRefs(t);
	freeTmplAttributes(t);
	freeTmplFormat(t);
	freeTmplRedirection(t);

	if (t->tm_master) {
		_evlTmplDecRef(t->tm_master);
	}
#ifdef _POSIX_THREADS
	(void) pthread_mutex_destroy(&t->tm_mutex);
#endif
	free(t);
}

/*
 * att is an array of structs.  Free all the struct templates in the list,
 * then free the list and null out the pointer.
 */
static void
freeArrayOfStructs(tmpl_attribute_t *att)
{
	evl_list_t *head = att->ta_value.val_list;
	evl_listnode_t *end, *p;

	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		template_t *st = (template_t*) p->li_data;
		if (st) {
			_evlFreeTemplate(st);
		}
	}
	if (head) {
		_evlFreeList(head, 0);
	}
	att->ta_value.val_list = NULL;
}

static void
depopulateArrayOfStructs(tmpl_attribute_t *att)
{
	freeArrayOfStructs(att);
}

static int
depopulateTemplate(template_t *template)
{
	evl_listnode_t *head, *p, *end;
	tmpl_attribute_t *att;

	if (!isPopulatedTmpl(template)) {
		return 0;
	}

	zapExtraDataAtt(template);

	head = template->tm_attributes;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		att = (tmpl_attribute_t *) p->li_data;
		if (isConstAtt(att)) {
			continue;
		}
		if (isArray(att)) {
			if (baseType(att) == TY_STRUCT) {
				depopulateArrayOfStructs(att);
			} else if (att->ta_value.val_array != att->ta_rawdata) {
				/* Same array, different architectures */
				free(att->ta_value.val_array);
				att->ta_value.val_array = NULL;
			}
		} else if (isStruct(att) && att->ta_value.val_struct) {
			/*
			 * Note that we depopulate the struct's template,
			 * but we don't free it.  Normal usage is that we'll
			 * be populated and depopulated multiple times with
			 * data from different records.
			 */
			(void) depopulateTemplate(att->ta_value.val_struct);
		} else {
			/* Nothing to do. */
		}
		att->ta_flags &= (~EVL_ATTR_EXISTS);
		att->ta_rawdata = NULL;
	}
	template->tm_flags &= (~TMPL_TF_POPULATED);
	template->tm_recid = (posix_log_recid_t) -1;
	return 0;
}

/*
 * Copy the next value in the event record's data buffer into att->ta_value.
 * We use an aligned intermediate buffer, data[], to ensure that we don't
 * run afoul of alignment rules (e.g., on ia64).
 */
static void
populateScalar(tmpl_attribute_t *att, size_t size, tmpl_popl_notes_t *notes, int arch)
{
	long long data[4];	/* big, aligned buffer */
//	memset(data, 0, 4 * sizeof(long long));
	(void) memcpy(data, notes->pn_next, size);
	_evl_conv_scalar(arch, baseType(att),  data);
	switch (baseType(att)) {
	case TY_CHAR:
		att->ta_value.val_long = *((char*) data);
		break;
	case TY_UCHAR:
		att->ta_value.val_ulong = *((unsigned char*) data);
		break;
	case TY_SHORT:
		att->ta_value.val_long = *((short*) data);
		break;
	case TY_USHORT:
		att->ta_value.val_ulong = *((unsigned short*) data);
		break;
	case TY_INT:
		att->ta_value.val_long = *((int*) data);
		break;
	case TY_UINT:
		att->ta_value.val_ulong = *((unsigned int*) data);
		break;
	case TY_LONG:
		att->ta_value.val_long = *((long*) data);
		break;
	case TY_ULONG:
		att->ta_value.val_ulong = *((unsigned long*) data);
		break;
	case TY_LONGLONG:
		att->ta_value.val_longlong = *((long long*) data);
		break;
	case TY_ULONGLONG:
		att->ta_value.val_ulonglong = *((unsigned long long*) data);
		break;
	case TY_WCHAR:
		att->ta_value.val_long = *((wchar_t*) data);
		break;
	case TY_ADDRESS:
		att->ta_value.val_address = *((void**) data);
		break;
	case TY_FLOAT:
		att->ta_value.val_double = *((float*) data);
		break;
	case TY_DOUBLE:
		att->ta_value.val_double = *((double*) data);
		break;
	case TY_LDOUBLE:
		att->ta_value.val_longdouble = *((long double*) data);
		break;
  default: /* keep gcc happy */;
	}

	notes->pn_next += size;
}

/*
 * Called when populating an attribute that is a string or array of strings.
 * Collect the string pointed to by notes->pn_next.  Returns a pointer to
 * the string (i.e., the initial value of notes->pn_next).  If there is no
 * null character between notes->pn_next and the end of the record, returns
 * NULL.  Updates notes->pn_next or notes->pn_out_of_data as appropriate.
 */
static char *
collectString(tmpl_popl_notes_t *notes)
{
	char *c, *s = notes->pn_next;
	size_t attSize;
	size_t bytesRemaining = notes->pn_end - notes->pn_next;

	/* Count the chars in the string, including the null. */
	for (c=s, attSize = 1; attSize <= bytesRemaining && *c; c++, attSize++)
		;
	if (bytesRemaining < attSize) {
		/* Ran off the end of data; no null found. */
		notes->pn_out_of_data = 1;
		return NULL;
	}
	notes->pn_next += attSize;
	return s;
}

/*
 * Like collectString, except we collect a wide string.  Note that we return a
 * char* rather than a wchar_t*, in case alignment is an issue.
 */
static char *
collectWstring(tmpl_popl_notes_t *notes, size_t wcsz)
{
	char *c, *s = notes->pn_next;
	size_t attSize;
	size_t bytesRemaining = notes->pn_end - notes->pn_next;
	wchar_t wc;
	long long wnull;
	
	memset(&wnull, 0, wcsz);

	/* Count the wide chars in the string, including the null. */
	for (c=s, attSize = wcsz; attSize <= bytesRemaining;
	    c += wcsz, attSize += wcsz) {
		if (!memcmp(&wnull, c, wcsz)) {
			break;
		}
	}
	if (bytesRemaining < attSize) {
		/* Ran off the end of data; no null found. */
		notes->pn_out_of_data = 1;
		return NULL;
	}
	notes->pn_next += attSize;
	return s;
}

/*
 * Adjust adjustMe upward as necessary so that its offset from tmplBase is a
 * multiple of alignment.  Return the adjusted value.
 */
char *
alignDataPtr(const char *adjustMe, const char *tmplBase, size_t alignment)
{
	int mod;
	const char *result = adjustMe;
	assert(alignment > 0);

	mod = (adjustMe - tmplBase) % alignment;
	if (mod != 0) {
		result = adjustMe + (alignment - mod);
	}
	return (char*) result;
}

/*
 * Adjust notes->pn_next upwards as necessary so that (notes->pn_next-tmplBase)
 * is a multiple of alignment.  This is done in preparation for populating
 * an attribute in an aligned record, or for rounding out the end of an
 * aligned record.  Note that this works OK even if we're already out of data.
 */
static void
alignNextPtr(tmpl_popl_notes_t *notes, const char *tmplBase, int alignment)
{
	notes->pn_next = alignDataPtr(notes->pn_next, tmplBase, alignment);
	if (notes->pn_next >= notes->pn_end) {
		notes->pn_next = notes->pn_end;
		notes->pn_out_of_data = 1;
	}
}

/*
 * Adjust notes->pn_next upwards as necessary so that (notes->pn_next-tmplBase)
 * is at least minSize.
 */
static void
enforceMinSize(tmpl_popl_notes_t *notes, const char *tmplBase, int minSize)
{
	int curSize = notes->pn_next - tmplBase;
	if (curSize < minSize) {
		notes->pn_next += minSize - curSize;
		if (notes->pn_next >= notes->pn_end) {
			notes->pn_next = notes->pn_end;
			notes->pn_out_of_data = 1;
		}
	}
}

#define bitsToBytes(b) ((b+CHAR_BIT-1)/CHAR_BIT)

/*
 * We may have been accumulating bit fields.  Update notes->pn_next as
 * necessary to reflect the bytes we've eaten for bit-fields.
 */
static void
adjustNextPerBitFields(tmpl_popl_notes_t *notes, char *bfStart, size_t bfOffset)
{
	notes->pn_next = bfStart + bitsToBytes(bfOffset);
}

/*
 * Only the low-order bfBits bits of value are valid.  Convert it to the
 * corresponding long long value by sign-extending the high valid bit.
 */
static long long
signExtendLL(long long value, size_t bfBits)
{
	static int longlongBits = sizeof(long long) * CHAR_BIT;
	size_t extraHighBits = longlongBits - bfBits;
	if (extraHighBits != 0) {
		value <<= extraHighBits;
		value >>= extraHighBits;
	}
	return value;
}

/*
 * Only the low-order bfBits bits of value are valid.  Convert it to the
 * corresponding unsigned long long value by masking off the high bits.
 */
static unsigned long long
zeroExtendLL(unsigned long long value, size_t bfBits)
{
	static int longlongBits = sizeof(unsigned long long) * CHAR_BIT;
	if (bfBits < longlongBits) {
		value &= ((1ULL << bfBits) - 1);
	}
	return value;
}

/*
 * We're working on a bit-field of type [unsigned] long long.  If, when we add
 * this bit-field into the current storage unit, the storage unit would
 * occupy parts of two different 8-byte blocks (counting from the start
 * of the record/struct), return 1; else return 0.  *Sigh*
 */
static int
bigMisalignedBitField(template_t *t, char *bfStart, size_t bfOffset,
	size_t bfBits)
{
	char *recBase = (char*) t->tm_data;
	char *lastByte = bfStart + ((bfOffset + bfBits -1) / CHAR_BIT);
	int firstBlock = (bfStart - recBase) / sizeof(long long);
	int lastBlock = (lastByte - recBase) / sizeof(long long);
	return (firstBlock != lastBlock);
}

/*
 * att is a bit-field of bfBits bits, starting at offset bfOffset bits from
 * bfStart.  Extract the bits from the bit-field and store the resulting
 * value in att->ta_value.
 */
static void
extractDataFromBitField(tmpl_attribute_t *att, char *bfStart, size_t bfOffset,
	size_t bfBits)
{
	unsigned long long value = 0;
	char *v, *b;
	char *firstByte = bfStart + (bfOffset / CHAR_BIT);
	char *lastByte = bfStart + ((bfOffset + bfBits -1) / CHAR_BIT);

	/* A storage unit cannot be bigger than our biggest integer. */
	assert (lastByte-firstByte <= sizeof(unsigned long long)-1);

	/*
	 * Copy the byte(s) containing the bit-field into the low-numbered
	 * byte(s) of value.
	 */
	for (v = (char*) &value, b = firstByte; b <= lastByte; ) {
		*v++ = *b++;
	}

	/*
	 * Shift the bit-field into the low-order bits of value.
	 * For now, don't worry about garbage in the high-order bits.
	 */
#if __BYTE_ORDER == __BIG_ENDIAN
	value >>= (sizeof(value)*CHAR_BIT - ((bfOffset%CHAR_BIT) + bfBits));
#else
	value >>= (bfOffset % CHAR_BIT);
#endif

	switch (baseType(att)) {
	case TY_CHAR:
	case TY_SHORT:
	case TY_INT:
	case TY_LONG:
	case TY_WCHAR:
		att->ta_value.val_long = (long) signExtendLL((long long) value,
			bfBits);
		break;
	case TY_UCHAR:
	case TY_USHORT:
	case TY_UINT:
	case TY_ULONG:
		att->ta_value.val_ulong = (unsigned long) zeroExtendLL(value,
			bfBits);
		break;
	case TY_LONGLONG:
		att->ta_value.val_longlong = signExtendLL((long long) value,
			bfBits);
		break;
	case TY_ULONGLONG:
		att->ta_value.val_ulonglong = zeroExtendLL(value, bfBits);
		break;
  default: /* keep gcc happy */;
	}

	att->ta_flags |= EVL_ATTR_EXISTS;
}

/*
 * att is a (possibly unnamed) bit-field in template t.  bfStart points to
 * the address of the bit-field storage unit currently being accumulated,
 * and bfOffset points to the current bit offset from bfStart.  (I.e., att's
 * bit-field will start at (*bfStart + *bfOffset) if there's room.)
 *
 * If the bit-field is named, store its value in att.  In any case, update
 * *bfOffset (and possibly *bfStart and notes->pn_next, if we have to start
 * a new storage unit) to reflect consumption of att's bit-field.
 *
 * Note that notes->pn_next is not updated unless we overflow the current
 * storage unit.
 */
static void
populateBitField(tmpl_popl_notes_t *notes, template_t *t, tmpl_attribute_t *att,
	char **bfStart, size_t *bfOffset)
{
	tmpl_base_type_t ty = baseType(att);
	size_t attSize = _evlTmplArchTypeInfo[t->tm_arch][ty].ti_size;
	size_t attBits = CHAR_BIT * attSize;
	size_t bfBits = att->ta_dimension->td_dimension;
	static size_t intBits = CHAR_BIT * sizeof(int);
	size_t bytesRemaining;
	size_t stgUnitBytes;

	if (bfBits == 0) {
		/* Zero-length bit-field.  Just align to start of rec/struct. */
		size_t bytesEaten = bitsToBytes(*bfOffset);
		char *adjustMe = *bfStart + bytesEaten;
		adjustMe = alignDataPtr(adjustMe, t->tm_data, attSize);
		if (adjustMe >= notes->pn_end) {
			goto outOfData;
		}
		*bfOffset = CHAR_BIT * (adjustMe - *bfStart);
		return;
	}

	if (*bfOffset + bfBits > attBits
	    || (attBits > intBits
	    && bigMisalignedBitField(t, *bfStart, *bfOffset, bfBits))) {
		/*
		 * 1. The bits we've already accumulated, plus the bits we want
		 * to add, won't fit in an object of att's type; or
		 * 2. we'd have a bigMisalignedBitField (which see).
		 * Start a new storage unit.
		 */
		adjustNextPerBitFields(notes, *bfStart, *bfOffset);
		if (bfBits > intBits) {
			alignNextPtr(notes, t->tm_data,
				_evlTmplArchTypeInfo[t->tm_arch][ty].ti_align);
		} else {
			alignNextPtr(notes, t->tm_data,
				computeAttAlignment(att, t->tm_arch));
		}
		*bfStart = notes->pn_next;
		*bfOffset = 0;
	}

	bytesRemaining = notes->pn_end - *bfStart;
	stgUnitBytes = bitsToBytes(*bfOffset + bfBits);
	if (stgUnitBytes > bytesRemaining) {
		goto outOfData;
	}

	if (att->ta_name) {
		extractDataFromBitField(att, *bfStart, *bfOffset, bfBits);
	}
	*bfOffset += bfBits;

	if (*bfOffset >= intBits) {
		/*
		 * gcc seems to do this.  I'm note sure why.
		 * Move the start of this storage unit up sizeof(int) bytes.
		 */
		*bfOffset -= intBits;
		notes->pn_next += sizeof(int);
		*bfStart = notes->pn_next;
	}
	return;

outOfData:
	notes->pn_next = notes->pn_end;
	notes->pn_out_of_data = 1;
}

/*
 * Populate attribute att, which is an array of up to nel structs.
 * A separate template clone is populated for each struct in the array.
 * Pointers to all these clones are stored in a list, and
 * att->ta_value.val_list points to that list.
 */
static void
populateArrayOfStructs(tmpl_popl_notes_t *notes, template_t *template,
	tmpl_attribute_t *att, int nel)
{
	int ns;
	evl_list_t *list = NULL;

	for (ns = 0; ns < nel; ns++) {
		template_t *st = _evlCloneTemplate(att->ta_type->u.st_template);
		st->tm_arch = template->tm_arch;
		populateTmpl(st, notes);
		if (st->tm_data_size == 0) {
			_evlFreeTemplate(st);
			break;
		}
		list = _evlAppendToList(list, st);
		if (notes->pn_out_of_data) {
			break;
		}
	}

	att->ta_value.val_list = list;
	if (list) {
		att->ta_dimension->td_dimension2 = _evlGetListSize(list);
		att->ta_flags |= EVL_ATTR_EXISTS;
	}
}

/*
 * att is an array in template t.  If the value of att's formatting delimiter
 * is provided by another attribute, get it.
 */
static void
populateDelimiter(template_t *t, tmpl_attribute_t *att)
{
	tmpl_attribute_t *delimAtt;
	tmpl_delimiter_t *de = att->ta_delimiter;

	if (!de || de->de_type == TMPL_DELIM_CONST) {
		return;
	}
	if (de->de_type == TMPL_DELIM_ATTR) {
		delimAtt = de->u.u_attribute;
	} else {
		assert(de->de_type == TMPL_DELIM_ATTNAME);
		delimAtt = _evlTmplFindAttribute(t, de->u.u_attname);
	}
	if (!delimAtt || !attExists(delimAtt)) {
		de->de_delimiter = NULL;
	} else {
		de->de_delimiter = evl_getStringAttVal(delimAtt);
	}
}

/*
 * Populate the indicated template from the data specified by the notes
 * argument.  Return with notes updated to reflect the data we've consumed.
 */
static void
populateTmpl(template_t *template, tmpl_popl_notes_t *notes)
{
	evl_listnode_t *head = template->tm_attributes;
	evl_listnode_t *p, *end;
	tmpl_attribute_t *att;
	int aligned = ((template->tm_flags & TMPL_TF_ALIGNED) != 0);
	char *bfStart;	/* address of current bit-field storage unit */
	size_t bfOffset;/* offset of next bit-field relative to bfStart */

	template->tm_data = notes->pn_next;

	/* Anything could get pulled into a bit-field storage unit. */
	bfStart = notes->pn_next;
	bfOffset = 0;

	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		tmpl_base_type_t atype;
		size_t bytesRemaining;

		att = (tmpl_attribute_t *) p->li_data;

		if (isConstAtt(att)) {
			if (isArray(att)) {
				/* This may be redundant nowadays. */
				att->ta_dimension->td_dimension2 =
					att->ta_dimension->td_dimension;
			}
			goto nextAttr;
		}

		if (!isBitField(att)) {
			if (aligned) {
				adjustNextPerBitFields(notes, bfStart,
					bfOffset);
				alignNextPtr(notes, template->tm_data,
					computeAttAlignment(att, template->tm_arch));
			}
			bfStart = notes->pn_next;
			bfOffset = 0;
		}

		bytesRemaining = notes->pn_end - notes->pn_next;
		if (bytesRemaining == 0) {
			notes->pn_out_of_data = 1;
			break;
		}
		if (notes->pn_out_of_data) {
			break;
		}

		/* Note: ta_rawdata can be non-null even if !attExists(att) */
		att->ta_rawdata = notes->pn_next;

		atype = baseType(att);
		if (isArray(att)) {
			/*
			 * nel is the number of elements we expect to see in
			 * the array.  There could be fewer elements if the
			 * record is truncated.
			 */
			int nel;
			tmpl_arch_type_info_t *ti = &_evlTmplArchTypeInfo[template->tm_arch][atype];
			tmpl_dimension_t *dim = att->ta_dimension;
			assert(dim != NULL);

			populateDelimiter(template, att);

			nel = dim->td_dimension;
			if (dim->td_type == TMPL_DIM_ATTR) {
				nel = getDimensionFromAttribute(
					dim->u.u_attribute);
				if (nel <= 0) {
					goto nextAttr;
				}
			} else if (dim->td_type == TMPL_DIM_ATTNAME) {
				tmpl_attribute_t *dimAtt = 
					_evlTmplFindAttribute(template,
					dim->u.u_attname);
				if (!dimAtt || (nel = getDimensionFromAttribute(dimAtt)) <= 0) {
					goto nextAttr;
				}
			} else if (dim->td_type == TMPL_DIM_REST) {
				/*
				 * This array occupies the rest of the record.
				 * Make nel big enough to ensure that it takes
				 * in the rest of the record.
				 */
				nel = bytesRemaining;
			}

			if (atype == TY_STRING) {
				/* Array of strings */
				int ns;
				for (ns = 0; ns < nel; ns++) {
					if (!collectString(notes)) {
						break;
					}
				}
				dim->td_dimension2 = ns;
				if (ns > 0) {
					att->ta_flags |= EVL_ATTR_EXISTS;
					att->ta_value.val_array = att->ta_rawdata;
				}
			} else if (atype == TY_WSTRING) {
				/* Array of wide strings */
				int ns;
				for (ns = 0; ns < nel; ns++) {
					if (!collectWstring(notes, _evlTmplArchTypeInfo[template->tm_arch][TY_WCHAR].ti_size)) {
						break;
					}
				}
				dim->td_dimension2 = ns;
				if (ns > 0) {
					att->ta_flags |= EVL_ATTR_EXISTS;
					att->ta_value.val_array = _evl_conv_wstring_array(template->tm_arch, att->ta_rawdata, ns);
				}
			} else if (atype == TY_STRUCT) {
				/* Array of structs */
				populateArrayOfStructs(notes, template, att,
					nel);
			} else {
				/* Array of scalars */
				int size = ti->ti_size;
				if (size * nel > bytesRemaining) {
					nel = bytesRemaining / size;
					notes->pn_out_of_data = 1;
				}
				dim->td_dimension2 = nel;
				if (nel > 0) {
					att->ta_value.val_array = (void *) _evl_conv_scalar_array(template->tm_arch, atype, notes->pn_next, nel, size); 
					//att->ta_value.val_array = notes->pn_next;
					notes->pn_next += size * nel;
					att->ta_flags |= EVL_ATTR_EXISTS;
				}
			}
		} else if (isStruct(att)) {
			template_t *st;
			if (!att->ta_value.val_struct) {
				/*
				 * There is no previously-populated struct
				 * to reuse.
				 */
				att->ta_value.val_struct = _evlCloneTemplate(
					att->ta_type->u.st_template);
			}
			st = att->ta_value.val_struct;
			/* inherit the architecture */
			st->tm_arch = template->tm_arch;
			populateTmpl(st, notes);
			if (st->tm_data_size > 0) {
				att->ta_flags |= EVL_ATTR_EXISTS;
			}
		} else if (isBitField(att)) {
			populateBitField(notes, template, att, &bfStart,
				&bfOffset);
		} else if (atype == TY_STRING) {
			char *s = collectString(notes);
			if (s) {
				att->ta_value.val_string = s;
				att->ta_flags |= EVL_ATTR_EXISTS;
			} else {
				break;
			}
		} else if (atype == TY_WSTRING) {
			char *s = collectWstring(notes, _evlTmplArchTypeInfo[template->tm_arch][TY_WCHAR].ti_size);
			if (s) {
				att->ta_rawdata = (wchar_t*) s; 
				att->ta_value.val_wstring = (wchar_t*) _evl_conv_wstring(template->tm_arch, s);
				att->ta_flags |= EVL_ATTR_EXISTS;
			} else {
				break;
			}
		} else {
			/* Should be a scalar attribute with a fixed size. */
			size_t attSize = _evlTmplArchTypeInfo[template->tm_arch][atype].ti_size;
			if (bytesRemaining < attSize) {
				notes->pn_out_of_data = 1;
				break;
			}
			populateScalar(att, attSize, notes, template->tm_arch);
			att->ta_flags |= EVL_ATTR_EXISTS;
		}

nextAttr:
		if (aligned && !isConstAtt(att) && !isBitField(att)) {
			/*
			 * In general, a member of integer, array (!), or
			 * struct (!) type T1 that is not a bit-field may
			 * be treated as a bit-field if it is followed by a
			 * bit-field of type T2, and sizeof(T2) > sizeof(T1).
			 * So if we have just populated an object of size N
			 * bytes, set the bit-field offset to 8*N.
			 */
			bfOffset = CHAR_BIT * (notes->pn_next - bfStart);
		}
	}

	if (aligned) {
		/* Move past any trailing bit-fields. */
		adjustNextPerBitFields(notes, bfStart, bfOffset);
	}

	/*
	 * Per ANSI C, the size of an aligned (i.e., C-style) struct is
	 * always a multiple of its alignment.
	 */
	if (aligned) {
		alignNextPtr(notes, template->tm_data, template->tm_alignment);
	}
	
	/*
	 * In gcc, apparently, if a struct has a bit-field of type T, the
	 * size of the struct will be at least sizeof(T).
	 */
	if (aligned) {
		enforceMinSize(notes, template->tm_data, template->tm_minsize);
	}

	template->tm_data_size = notes->pn_next - (char*) template->tm_data;
	template->tm_flags |= TMPL_TF_POPULATED;
}

/*
 * The indicated template has been populated from a record that has left-over
 * bytes that we can't associate with any of the other attributes.
 * Create a pseudo-attribute with name EXTRA_DATA_ATT_NAME and with type
 * and dimension char[_R_], and append this attribute to the template's list.
 * NOTE: This attribute should be deallocated only via zapExtraDataAtt().
 */
static void
createExtraDataAtt(template_t *template, tmpl_popl_notes_t *notes)
{
	tmpl_attribute_t *att;
	ptrdiff_t nextra = notes->pn_end - notes->pn_next;
	assert(nextra > 0);

	att = _evlTmplAllocAttribute();
	att->ta_name = EXTRA_DATA_ATT_NAME;	/* NOT strduped */
	att->ta_type = _evlTmplAllocDataType();
	att->ta_type->tt_base_type = TY_CHAR;
	att->ta_dimension = _evlTmplAllocDimension();
	att->ta_dimension->td_type = TMPL_DIM_REST;
	att->ta_dimension->td_dimension = -1;
	att->ta_dimension->td_dimension2 = nextra;
	att->ta_format.af_type = TMPL_AFS_DUMP;
	att->ta_delimiter = NULL;
	att->ta_value.val_array = notes->pn_next;
	att->ta_flags = EVL_ATTR_EXISTS;
	att->ta_index = _evlGetListSize(template->tm_attributes);
	template->tm_attributes = _evlAppendToList(template->tm_attributes, att);
	notes->pn_next = notes->pn_end;
}

/*
 * Called when depopulating a template.  If the indicated template's last
 * attribute is named EXTRA_DATA_ATT_NAME, then remove that attribute from
 * the list and recycle it.
 */
static void
zapExtraDataAtt(template_t *template)
{
	evl_listnode_t *head, *tail;
	tmpl_attribute_t *lastAtt;

	head = template->tm_attributes;
	if (!head) {
		return;
	}
	tail = head->li_prev;
	lastAtt = (tmpl_attribute_t*) tail->li_data;
	if (lastAtt->ta_name && !strcmp(lastAtt->ta_name, EXTRA_DATA_ATT_NAME)) {
		free(lastAtt->ta_type);
		free(lastAtt->ta_dimension);
		free(lastAtt);
		template->tm_attributes = _evlRemoveNode(tail, head, NULL);
	}
}

static size_t
populateTemplate(template_t *template, const void *data, size_t dataSize)
{
	tmpl_popl_notes_t notes;

	depopulateTemplate(template);

	if (data == NULL || dataSize == 0) {
		/* No data with which to populate template. */
		template->tm_data = NULL;
		template->tm_data_size = 0;
		template->tm_flags |= TMPL_TF_POPULATED;
		return 0;
	}

	notes.pn_next = (char*) data;
	notes.pn_end = notes.pn_next + dataSize;
	notes.pn_out_of_data = 0;

	populateTmpl(template, &notes);

	if (notes.pn_next < notes.pn_end) {
		createExtraDataAtt(template, &notes);
	}
	return notes.pn_next - (char*)data;
}

/***** API functions *****/

int
evl_clonetemplate(evltemplate_t *master, evltemplate_t **clone)
{
	if (!master || !clone) {
		return EINVAL;
	}

	if (!isMasterTmpl(master)) {
		return EINVAL;
	}

	*clone = _evlCloneTemplate(master);
	return 0;
}

int
evl_populatetemplate(evltemplate_t *template,
	const struct posix_log_entry *entry, const void *buf)
{
	if (!template || !entry) {
		return EINVAL;
	}

	if (buf == NULL && entry->log_size != 0) {
		return EINVAL;
	}

	if (isMasterTmpl(template)) {
		return EINVAL;
	}
	template->tm_arch = entry->log_magic & 0x0000ffff;
	
	template->tm_recid = entry->log_recid;
	template->tm_entry = entry;
	template->tm_data = buf;
	(void) populateTemplate(template, buf, entry->log_size);
	return 0;
}

int
evl_depopulatetemplate(evltemplate_t *template)
{
	if (!template) {
		return EINVAL;
	}
	(void) depopulateTemplate(template);
	return 0;
}

int
evltemplate_getatt(const evltemplate_t *template, const char *attName,
	evlattribute_t **att)
{
	tmpl_attribute_t *attr;
	
	if (!template || !attName || !att) {
		return EINVAL;
	}

	attr = _evlTmplFindAttribute(template, attName);

	if (attr) {
		*att = attr;
		return 0;
	} else {
		return ENOENT;
	}
}

int
evltemplate_getatts(const evltemplate_t *template, evlattribute_t *buf[],
	unsigned int bufatts, unsigned int *natts)
{
	evl_listnode_t *head, *p;
	unsigned int na, i;

	if (template == NULL) {
		return EINVAL;
	}
	head = template->tm_attributes;
	na = _evlGetListSize(head);
	if (natts) {
		*natts = na;
	}
	if (buf == NULL || bufatts == 0) {
		return EINVAL;
	}
	if (bufatts < na) {
		return EMSGSIZE;
	}

	for (i=0, p=head; i < na; i++, p=p->li_next) {
		buf[i] = (evlattribute_t *) p->li_data;
	}
	return 0;
}

int
evlatt_gettype(const evlattribute_t *att)
{
	return att->ta_type->tt_base_type;
}

int
evlatt_getinfo(const evlattribute_t *att, evlatt_info_t *info)
{
	if (!att || !info) {
		return EINVAL;
	}

	info->att_flags = att->ta_flags;
	info->att_name = att->ta_name;
	info->att_type = att->ta_type->tt_base_type;

	if (isArray(att)) {
		tmpl_dimension_t *dim = att->ta_dimension;
		info->att_isarray = 1;
		if (dim->td_type == TMPL_DIM_CONST) {
			info->att_dimfixed = dim->td_dimension;
		} else {
			info->att_dimfixed = -1;
		}
		if (attExists(att)) {
			info->att_dimpop = dim->td_dimension2;
		} else {
			info->att_dimpop = -1;
		}
	} else {
		info->att_isarray = 0;
		info->att_dimfixed = 0;
		info->att_dimpop = 0;
	}
	return 0;
}

int
evlatt_getstructtmpls(const evlattribute_t *att,
	const evltemplate_t **master, const evltemplate_t **clone)
{
	if (!att || !isStruct(att) || isArray(att)) {
		return EINVAL;
	}
	if (!master && !clone) {
		return EINVAL;
	}
	if (master) {
		*master = att->ta_type->u.st_template;
	}
	if (clone) {
		*clone = att->ta_value.val_struct;
	}
	return 0;
}

/*
 * att is a populated array of structs.  index selects one element in this
 * array.  Set *template to point to the populated template that represents
 * that particular struct.  Returns 0 on sucess, or an error code.
 */
int
evlatt_getstructfromarray(const evlattribute_t *att, int index,
	const evltemplate_t **tmpl)
{
	evl_list_t *elements;

	if (!att || !attExists(att) || !isStruct(att) || !isArray(att)) {
		return EINVAL;
	}
	if (!tmpl) {
		return EINVAL;
	}
	if (index < 0) {
		return EINVAL;
	}
	
	elements = att->ta_value.val_list;
	assert(elements != NULL);
	*tmpl = (const template_t*) _evlGetNthValue(elements, index);
	if (*tmpl == NULL) {
		return ENOENT;
	}
	return 0;
}

static char *
getRedirAttName(template_t *t, evl_list_t *scopedId)
{
	char *attName = _evlMakeDotPathFromList(scopedId);
	evlattribute_t *att;

	_evlFreeList(scopedId, 1);
	att = _evlTmplFindAttribute(t, attName);
	if (att) {
		if (isArray(att)
		    || (!isIntegerAtt(att) && baseType(att) != TY_STRING)) {
			semanticError(
				"Redirection attribute %s must be integer or string.",
				attName);
			free(attName);
			return NULL;
		}
	} else {
		semanticError("Unknown attribute: %s", attName);
		free(attName);
		return NULL;
	}
	return attName;
}

struct redirectedAttribute *
_evlMkRedirAtt(template_t *t, unsigned int n, evl_list_t *scopedId, int att)
{
	struct redirectedAttribute *r = (struct redirectedAttribute *)
		malloc(sizeof(struct redirectedAttribute));
	assert(r != NULL);
	if (scopedId) {
		r->u.attname = getRedirAttName(t, scopedId);
		if (r->u.attname) {
			r->type = TMPL_RD_ATTNAME;
			/* TODO: If this is a const att, convert to TMPL_RD_CONST */
		} else {
			/* Error has been reported. */
			r->type = TMPL_RD_NONE;
		}
	} else {
		r->type = TMPL_RD_CONST;
		if (att == POSIX_LOG_ENTRY_FACILITY) {
			r->u.faccode = (posix_log_facility_t) n;
		} else {
			r->u.evtype = (int) n;
		}
	}
	return r;
}

void
_evlAddRedirection(template_t *t, struct redirectedAttribute *rf,
	struct redirectedAttribute *ret)
{
	tmpl_redirection_t *rd = (tmpl_redirection_t*)
		malloc(sizeof(tmpl_redirection_t));
	assert(rd != NULL);
	rd->rd_fac = rf;
	rd->rd_evtype = ret;
	t->tm_redirection = rd;
	t->tm_flags |= TMPL_TF_REDIRECT;
	if (t->tm_header.th_type == TMPL_TH_STRUCT) {
		semanticError("struct template cannot have redirection");
	}
	/*
	 * TODO: Check for redirection to self -- e.g.,
	 * facility 1; event_type 2;
	 * redirect { facility 1; }
	 *	or
	 * facility 1; event_type 2;
	 * const { int f = 1; }
	 * redirect { facility f; }
	 */
}

/*
 * WARNING ABOUT MULTI-LINE MACROS:
 * These macros and associated functions are at the end of the file because
 * the multi-line macros really mess up line numbering in the compiler.
 */
#define ASSIGN_NUMBER(t) switch(valType) {\
case TY_LONG:		*((t*)loc) = (t) val->tv_value.val_long; break;\
case TY_ULONG:		*((t*)loc) = (t) val->tv_value.val_ulong; break;\
case TY_LONGLONG:	*((t*)loc) = (t) val->tv_value.val_longlong; break;\
case TY_ULONGLONG:	*((t*)loc) = (t) val->tv_value.val_ulonglong; break;\
case TY_DOUBLE:		*((t*)loc) = (t) val->tv_value.val_double; break;\
case TY_LDOUBLE:	*((t*)loc) = (t) val->tv_value.val_longdouble; break;\
default: cantAssign(valType, attType); return -1; }

/*
 * Copy the value in val into the location specified by loc, which is a pointer
 * to the type of value specified by attType.  The value in val is cast
 * appropriately.
 */
static int
initScalarValue(tmpl_base_type_t attType, void *loc, tmpl_type_and_value_t *val)
{
	tmpl_base_type_t valType = val->tv_type;
	if (attType == valType) {
		memcpy(loc, &val->tv_value, _evlTmplTypeInfo[valType].ti_size);
		return 0;
	}

	switch (attType) {
	case TY_CHAR:
		ASSIGN_NUMBER(signed char)
		break;
	case TY_WCHAR:
		ASSIGN_NUMBER(wchar_t)
		break;
	case TY_UCHAR:
		ASSIGN_NUMBER(unsigned char)
		break;
	case TY_SHORT:
		ASSIGN_NUMBER(short)
		break;
	case TY_USHORT:
		ASSIGN_NUMBER(unsigned short)
		break;
	case TY_INT:
		ASSIGN_NUMBER(int)
		break;
	case TY_UINT:
		ASSIGN_NUMBER(unsigned int)
		break;
	case TY_LONG:
		ASSIGN_NUMBER(long)
		break;
	case TY_ULONG:
		ASSIGN_NUMBER(unsigned long)
		break;
	case TY_LONGLONG:
		ASSIGN_NUMBER(long long)
		break;
	case TY_ULONGLONG:
		ASSIGN_NUMBER(unsigned long long)
		break;
	case TY_FLOAT:
		ASSIGN_NUMBER(float)
		break;
	case TY_DOUBLE: 
		ASSIGN_NUMBER(double)
		break;
	case TY_LDOUBLE: 
		ASSIGN_NUMBER(long double)
		break;
	case TY_ADDRESS: 
		switch (valType) {
		case TY_LONG:
			*((void**)loc) = (void*) val->tv_value.val_long;
			break;
		case TY_ULONG:
			*((void**)loc) = (void*) val->tv_value.val_ulong;
			break;
		default:
			cantAssign(valType, attType);
			return -1;
		}
		break;
	default:
		/*
		 * Note: You come here if you try to assign anything but
		 * a string to a string.
		 */
		cantAssign(valType, attType);
		return -1;
	}

	return 0;
}

#define COPY_NUMBER(m,t) switch(valType) {\
case TY_LONG:		att->ta_value.m = (t) val->tv_value.val_long; break;\
case TY_ULONG:		att->ta_value.m = (t) val->tv_value.val_ulong; break;\
case TY_LONGLONG:	att->ta_value.m = (t) val->tv_value.val_longlong; break;\
case TY_ULONGLONG:	att->ta_value.m = (t) val->tv_value.val_ulonglong; break;\
case TY_DOUBLE:		att->ta_value.m = (t) val->tv_value.val_double; break;\
case TY_LDOUBLE:	att->ta_value.m = (t) val->tv_value.val_longdouble; break;\
default: cantAssign(valType, attType); return -1; }

/*TODO: Check for integer initializers out of range.  Don't forget bit-fields.*/
static int
initConstScalar(tmpl_attribute_t *att, tmpl_type_and_value_t *val)
{
	tmpl_base_type_t attType = baseType(att);
	tmpl_base_type_t valType = val->tv_type;

	if (valType == attType) {
		if (valType == TY_STRING) {
			att->ta_value.val_string =
				strdup(val->tv_value.val_string);
		} else if (valType == TY_WSTRING) {
			/* basically a wide strdup */
			size_t nwc = wcslen(val->tv_value.val_wstring);
			att->ta_value.val_wstring = (wchar_t*) malloc(
				(nwc+1) * sizeof(wchar_t));
			memcpy(att->ta_value.val_wstring,
				val->tv_value.val_wstring,
				(nwc+1) * sizeof(wchar_t));
		} else {
			att->ta_value = val->tv_value;
		}
		return 0;
	}

	switch (attType) {
	case TY_CHAR:
		COPY_NUMBER(val_long, signed char)
		break;
	case TY_WCHAR:
		COPY_NUMBER(val_long, wchar_t)
		break;
	case TY_UCHAR:
		COPY_NUMBER(val_ulong, unsigned char)
		break;
	case TY_SHORT:
		COPY_NUMBER(val_long, short)
		break;
	case TY_USHORT:
		COPY_NUMBER(val_ulong, unsigned short)
		break;
	case TY_INT:
		COPY_NUMBER(val_long, int)
		break;
	case TY_UINT:
		COPY_NUMBER(val_ulong, unsigned int)
		break;
	case TY_LONG:
		COPY_NUMBER(val_long, long)
		break;
	case TY_ULONG:
		COPY_NUMBER(val_ulong, unsigned long)
		break;
	case TY_LONGLONG:
		COPY_NUMBER(val_longlong, long long)
		break;
	case TY_ULONGLONG:
		COPY_NUMBER(val_ulonglong, unsigned long long)
		break;
	case TY_FLOAT:
		COPY_NUMBER(val_double, float)
		break;
	case TY_DOUBLE: 
		COPY_NUMBER(val_double, double)
		break;
	case TY_LDOUBLE: 
		COPY_NUMBER(val_longdouble, long double)
		break;
	case TY_ADDRESS:
		switch (valType) {
		case TY_LONG:
			att->ta_value.val_address =
				(void*) val->tv_value.val_long;
			break;
		case TY_ULONG:
			att->ta_value.val_address =
				(void*) val->tv_value.val_ulong;
			break;
		default:
			cantAssign(valType, attType);
			return -1;
		}
		break;
	default:
		/*
		 * Note: You come here if you try to assign anything but
		 * a string to a string.
		 */
		cantAssign(valType, attType);
		return -1;
	}
	return 0;
}

/***
 *** Note: Avoid adding code here at the end of the file.  See the
 *** WARNING ABOUT MULTI-LINE MACROS above.
 ***/
