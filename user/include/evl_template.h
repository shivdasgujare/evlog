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

#ifndef _EVL_TEMPLATE_H_
#define _EVL_TEMPLATE_H_

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <wchar.h>
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_list.h"

/*
 * A template contains the following sections:
 * - imports and typedefs
 * - header (with facility/event_type or struct name)
 * - attributes (const and record attributes are stored in a single list)
 * - formatting section
 * 
 * Imports and typedefs are stored in global lists, because they are of file
 * scope.  The rest of the info is stored as part of struct _template
 * (= template_t = evl_template_t).
 */

#define EVL_ATTRSTR_MAXLEN (8*1024)
#define EVL_DEFAULT_EVENT_TYPE EVL_INVALID_EVENT_TYPE

#ifdef __cplusplus
extern "C" {
#endif

/*
 * An attribute (tmpl_attribute_t = evl_attribute_t) has the following
 * components:
 * - type
 * - name
 * - optional dimension or bit-field width
 * - value (provided by an initializer, if it's a const attribute)
 * - optional format
 * 
 * A typedef also has most of these components, and so it is represented
 * using the same data structure, but with a flag of EVL_ATTR_TYPEDEF.
 */

/*** ATTRIBUTE TYPE ***/

typedef enum tmpl_base_type {
	TY_NONE,	/* absence of value */
	TY_CHAR,	/* signed char, actually */
	TY_UCHAR,
	TY_SHORT,
	TY_USHORT,
	TY_INT,
	TY_UINT,
	TY_LONG,
	TY_ULONG,
	TY_LONGLONG,
	TY_ULONGLONG,
	TY_FLOAT,
	TY_DOUBLE,
	TY_LDOUBLE,
	TY_STRING,
	TY_WCHAR,
	TY_WSTRING,
	TY_ADDRESS,
	TY_STRUCT,
	TY_PREFIX3,
	TY_LIST,	/*for arrays of structs, and when parsing initializers*/
	TY_STRUCTNAME,		/* converted to TY_STRUCT after lookup */
	TY_TYPEDEF,		/* type name better be a typedef name */
	TY_SPECIAL		/* catch-all for special cases */
} tmpl_base_type_t;

/*
 * NOTE: For an attribute of type struct, att->ta_type->u.st_template
 * points to the struct's master template, and att->ta_value.val_struct
 * will point to a clone when the attribute is populated.
 */

typedef struct tmpl_data_type {
	tmpl_base_type_t	tt_base_type;
	union {
		char		*st_name;	/* Use this... */
		struct _template *st_template;	/* ... to find this. */
		char		*td_name;	/* typedef name */
	}			u;
} tmpl_data_type_t;

typedef struct tmpl_prefix3 {
	unsigned short	p3_size;
	unsigned char	p3_format;
} tmpl_prefix3_t;

/*** ATTRIBUTE DIMENSION ***/

typedef enum tmpl_dimension_type {
	TMPL_DIM_NONE,		/* no dimension -- defaults to 1 */
	TMPL_DIM_CONST,		/* explicit dimension in template */
	TMPL_DIM_IMPLIED,	/* e.g., int x[] = {1,2,3}; */
	TMPL_DIM_REST,		/* remainder of record */
	TMPL_DIM_ATTR,		/* another attribute holds dimension; */
				/*	u_attribute points to it */
	TMPL_DIM_ATTNAME,	/* ditto, except u_attname is its name */
	TMPL_DIM_PREFIX3,	/* 3-byte prefix holds array size */
	TMPL_DIM_BITFIELD	/* td_dimension=nbits */
} tmpl_dimension_type_t;

/*
 * Overloading "dimension" to include bit-fields is sort of bizarre.
 * This was done mostly because the dimension info and the bit-field info
 * occupy the same part of the attribute statement.
 * A bit-field's width (in bits) is stored in td_dimension.
 */
typedef struct tmpl_dimension {
	tmpl_dimension_type_t	td_type;
	int			td_dimension;	/* explicit or computed */
	int			td_dimension2;	/* set when populated */
	union {
		struct tmpl_attribute	*u_attribute;	/* TMPL_DIM_ATTR */
		struct tmpl_prefix3	u_prefix3;	/* TMPL_DIM_PREFIX3 */
		char	*u_attname;	/* may become TMPL_DIM_ATTR */
	}			u;
} tmpl_dimension_t;

/*** ATTRIBUTE VALUE ***/

typedef union tmpl_value {
	long			val_long;	/* also char, short, int */
	unsigned long		val_ulong;	/* also uchar, ushort, uint */
	void			*val_address;	/* address of something */
	double			val_double;	/* also float */
	long double		val_longdouble;
	long long		val_longlong;
	unsigned long long	val_ulonglong;
	struct _template	*val_struct;
	char			*val_string;
	wchar_t			*val_wstring;
	void			*val_array;
	evl_list_t		*val_list;	/* used during parsing */
} tmpl_value_t;

/*** ATTRIBUTE FORMAT ***/

/* An attribute's format specification */
typedef enum tmpl_att_fmt_type {
	TMPL_AFS_DEFAULT,	/* No format specified */
	TMPL_AFS_TBD,		/* Format specified; needs analysis */
	TMPL_AFS_SCALAR,	/* Scalar attribute; format specified */
	TMPL_AFS_DUMP,		/* %t */
	TMPL_AFS_BITMAP,	/* %b */
	TMPL_AFS_VALNM,		/* %v */
	TMPL_AFS_ARRAY,		/* Array of scalars, no %I.  () stripped */
	TMPL_AFS_IZARRAY,	/* Array with %I and/or %Z */
	TMPL_AFS_CHARRSTR	/* Array of [u]char formatted with %s */
} tmpl_att_fmt_type_t;

/*
 * Value matches if ((val & bm_1bits) == bm_1bits) && ((val & bm_0bits) == 0).
 * For %v, we just test val == bm_1bits.
 */
typedef struct tmpl_bitmap {
	long	bm_1bits;	/* bit=1 -> must be 1 in matching val */
	long	bm_0bits;	/* bit=1 -> must be 0 in matching val */
	char	*bm_name;
} tmpl_bitmap_t;

typedef struct tmpl_att_format {
	tmpl_att_fmt_type_t	af_type;
	int			af_shared;	/* shared with typedef */
	char			*af_format;	/* TBD, SCALAR, ARRAY, IZARRAY*/
	union {
		tmpl_bitmap_t	*u_bitmaps;	/* BITMAP, VALNM */
		char		u_izorder[3];	/* e.g., Z, IZ, IS, SI, ZI */
	}			u;
} tmpl_att_format_t;

/*** ATTRIBUTE DELIMITER ***/

typedef enum tmpl_delim_type {
	TMPL_DELIM_NONE,	/* used in serialization */
	TMPL_DELIM_CONST,	/* explicit delimiter in template */
	TMPL_DELIM_ATTR,	/* another attribute holds delimiter */
				/*	u_attribute points to it */
	TMPL_DELIM_ATTNAME	/* ditto, except u_attname is its name */
} tmpl_delim_type_t;

/*
 * The delimiter clause of an attribute specification, used when formatting an
 * array.
 * If no delimiter is specified, att->ta_delimiter is null.
 * If a string literal is specified, type=CONST.
 * If a simple attribute name is specified, type=ATTR.
 * Otherwise (scoped attribute name), type=ATTNAME.
 */
typedef struct tmpl_delimiter {
	tmpl_delim_type_t	de_type;
	char			*de_delimiter;
	union {
		struct tmpl_attribute	*u_attribute;	/* TMPL_DELIM_ATTR */
		char	*u_attname;	/* may become TMPL_DELIM_ATTR */
	}			u;
} tmpl_delimiter_t;

/*** ATTRIBUTE ***/

/* Attribute flags */
#define EVL_ATTR_EXISTS		0x1
#define EVL_ATTR_CONST		0x2
#define EVL_ATTR_BITFIELD	0x4
#define EVL_ATTR_CLONE		0x8	/* not shared with master template */
#define EVL_ATTR_TYPEDEF	0x10	/* a typedef, not an attribute */
#define EVL_ATTR_IMPLDIM	0x20	/* implied dimension, master ! CONST */

typedef struct tmpl_attribute {
	const char		*ta_name;
	tmpl_data_type_t	*ta_type;
	tmpl_dimension_t	*ta_dimension;
	tmpl_att_format_t	ta_format;
	tmpl_delimiter_t	*ta_delimiter;
	void 				*ta_rawdata;
	tmpl_value_t		ta_value;
	unsigned int		ta_flags;
	int			ta_index;
} tmpl_attribute_t;

typedef tmpl_attribute_t evlattribute_t;

/* User-visible info about an attribute */
typedef struct evlatt_info {
	unsigned int	att_flags;		/* ta_flags */
	const char	*att_name;		/* ta_name */
	int		att_type;		/* ta_type */
	int		att_isarray;		/* 1 if an array */
	int		att_dimfixed;		/* ta_dimension, sort of */
	int		att_dimpop;		/* ta_dimension2, sort of */
} evlatt_info_t;

typedef struct tmpl_struct_ref {
	const char	*sr_path;	/* struct's import path */
	struct _template *sr_template;	/* struct's template */
	int		sr_used;	/* 1 if used by current template */
} tmpl_struct_ref_t;

/*** TEMPLATE HEADER ***/

typedef enum tmpl_header_type {
	TMPL_TH_EVLOG,
	TMPL_TH_TRACE,
	TMPL_TH_STRUCT
} tmpl_header_type_t;

typedef struct tmpl_header {
	tmpl_header_type_t	th_type;
	const char		*th_description;
	union {
		struct {
			posix_log_facility_t evl_facility;
			int	evl_event_type;
		} u_evl;
		struct {
			int	tra_major;
			int	tra_minor;
		} u_trace;
		struct {
			const char	*st_name;
		} u_struct;
	}			u;
} tmpl_header_t;

/*** TEMPLATE REDIRECTION ***/
enum redirection_type {
	TMPL_RD_NONE,
	TMPL_RD_CONST,
	TMPL_RD_ATTNAME
};

struct redirectedAttribute {
	enum redirection_type	type;
	union {
		posix_log_facility_t	faccode;
		int			evtype;
		char			*attname;
	}			u;
};

typedef struct tmpl_redirection {
	struct redirectedAttribute	*rd_fac;
	struct redirectedAttribute	*rd_evtype;
} tmpl_redirection_t;

/*** TEMPLATE ***/

/* Template flags */
#define TMPL_TF_POPULATED	0x1
#define TMPL_TF_FIXEDSIZE	0x2	/* not currently used */
#define TMPL_TF_ERROR		0x4
#define TMPL_TF_EATSALL		0x8	/* Contains an [_R_] attribute */
#define TMPL_TF_IMPORTED	0x10	/* Created by _evlReadTemplate() */
#define TMPL_TF_ALIGNED		0x20	/* Padded like a C struct */
#define TMPL_TF_CONST		0x40	/* const struct */
					/* 0x80 reserved */
#define TMPL_TF_IMPLDIM		0x100	/* Contains implied-dimension array */
#define TMPL_TF_REDIRECT	0x200	/* Contains redirection(s) */

/* List of evl_fmt_segment */
typedef evl_list_t tmpl_parsed_fmt_t;

/*
 * The device and inode number of a binary template file uniquely
 * identify an imported template.  We use these rather than a pathname
 * because there can be multiple paths (e.g., through symbolic links or
 * through . and ..) to the same template file.
 */
typedef struct tmpl_context {
	dev_t	tc_device;
	ino_t	tc_inode;
} tmpl_context_t;

typedef struct _template {
    /* Components of an unpopulated template */
	tmpl_header_t		tm_header;
	int			tm_flags;
	evl_list_t		*tm_attributes;
	const char		*tm_format;	/* copied from template file */
	tmpl_parsed_fmt_t	*tm_parsed_format;
	short			tm_alignment;	/* 1, 2, 4, etc. */
	short			tm_minsize;	/*gcc weirdness wrt bitfields*/
	int 			tm_arch;
    	tmpl_redirection_t	*tm_redirection;
    /* Additional components of a populated template */
	size_t			tm_data_size;
	const struct posix_log_entry *tm_entry;	/* fixed part of event rec */
	const void		*tm_data;	/* optional part */
	posix_log_recid_t	tm_recid;	/* cross-check with tm_entry */
    /* Master/clone information */
	struct _template	*tm_master;	/* who we're cloned from */
	int			tm_ref_count;	/* how many clones or other
						 * templates reference this */
#ifdef _POSIX_THREADS
	pthread_mutex_t		tm_mutex;	/* protects ref count */
#endif
    /* Stuff needed by import fussing. */
	const char		*tm_name;	/* for lookup */
    	tmpl_context_t		tm_context;	/* Unique ID */
	evl_list_t		*tm_imports;	/* Structs I import */
} template_t;

typedef template_t evltemplate_t;

/* 
 * Information about a data type - Holds types information for each 
 * supported architectures
 */
typedef struct tmpl_arch_type_info {
	char	ti_size;	/* in bytes */
	char	ti_align;	/* from gcc's __alignof__ */
} tmpl_arch_type_info_t;

/* Information about a data type.  See the table early in template.c. */
typedef struct tmpl_type_info {
	char	ti_size;	/* in bytes */
	char	ti_align;	/* from gcc's __alignof__ */
	char	ti_isScalar;
	char	ti_isInteger;
	const char	*ti_name;
	const char	*ti_format;	/* default format */
} tmpl_type_info_t;

/*
 * Stuff used in parsing...
 */
typedef struct tmpl_type_and_value {
	tmpl_base_type_t	tv_type;
	tmpl_value_t		tv_value;
} tmpl_type_and_value_t;

typedef struct tmpl_parser_context {
	FILE		*pc_errfile;	/* where to report errors */
	const char	*pc_pathname;	/* the file we're parsing */
	int		pc_lineno;	/* lex's line number */
	template_t	*pc_template;	/* the template under construction */
	const char	*pc_progname;	/* argv[0] */
	int		pc_errors;	/* errors not yet hung on a template */
	char		*pc_dfltdesc;	/* default description from event_type*/
} tmpl_parser_context_t;

/* Under what circumstances is this template being imported? */
#define TMPL_IMPORT_EXPLICIT 1	/* import statement or compiled template */
#define TMPL_IMPORT_IMPLICIT 2	/* ref from attribute statement */
#define TMPL_IMPORT_BINARY 3	/* ref from binary template file */

extern int _evlTmplMgmtFlags;
#define TMPL_REUSE1CLONE	0x1	/* Reuse the same clone repeatedly */
#define TMPL_LAZYDEPOP		0x2	/* Don't depopulate on release */
#define TMPL_IGNOREALL		0x4	/* evl_readtemplate returns ENOENT */

#define evl_getLongAttVal(att) ((att)->ta_value.val_long)
#define evl_getUlongAttVal(att) ((att)->ta_value.val_ulong)
#define evl_getDoubleAttVal(att) ((att)->ta_value.val_double)
#define evl_getLongdoubleAttVal(att) ((att)->ta_value.val_longdouble)
#define evl_getLonglongAttVal(att) ((att)->ta_value.val_longlong)
#define evl_getUlonglongAttVal(att) ((att)->ta_value.val_ulonglong)
#define evl_getStringAttVal(att) ((att)->ta_value.val_string)
#define evl_getAddressAttVal(att) ((att)->ta_value.val_address)
#define evl_getArrayAttVal(att) ((att)->ta_value.val_array)

/*
 * TODO: The macros in these next two sections don't adhere to the
 * convention that all our externally visible symbols start with evl_
 * or _evl or tmpl.  We need to hide them behind an ifdef.
 */
#define baseType(att) ((att)->ta_type->tt_base_type)
#define isConstAtt(att) (((att)->ta_flags & EVL_ATTR_CONST) != 0)
#define isClonedAtt(att) (((att)->ta_flags & EVL_ATTR_CLONE) != 0)
#define attExists(att) (((att)->ta_flags & EVL_ATTR_EXISTS) != 0)
#define isBitField(att) (((att)->ta_flags & EVL_ATTR_BITFIELD) != 0)
#define isTypedef(att) (((att)->ta_flags & EVL_ATTR_TYPEDEF) != 0)
#define isArray(att) ((att)->ta_dimension && !isBitField(att))
#define isStruct(att) (baseType(att) == TY_STRUCT)
#define isIntegerAtt(att) (_evlTmplTypeInfo[baseType(att)].ti_isInteger)

#define isFixedSizeTmpl(tmpl) (((tmpl)->tm_flags & TMPL_TF_FIXEDSIZE) != 0)
#define isPopulatedTmpl(tmpl) (((tmpl)->tm_flags & TMPL_TF_POPULATED) != 0)
#define isConstStructTmpl(tmpl) (((tmpl)->tm_flags & TMPL_TF_CONST) != 0)
#define isMasterTmpl(tmpl) ((tmpl)->tm_master == NULL)
#define isRedirectTmpl(tmpl) (((tmpl)->tm_flags & TMPL_TF_REDIRECT) != 0)

/* template.c */
extern int evl_clonetemplate(evltemplate_t *master, evltemplate_t **clone);
extern int evl_populatetemplate(evltemplate_t *tmpl,
	const struct posix_log_entry *entry, const void *buf);
extern int evl_depopulatetemplate(evltemplate_t *tmpl);
extern int evltemplate_getatt(const evltemplate_t *tmpl,
	const char *attName, evlattribute_t **att);
extern int evltemplate_getatts(const evltemplate_t *tmpl,
	evlattribute_t *buf[], unsigned int bufatts, unsigned int *natts);
extern int evlatt_gettype(const evlattribute_t *att);
extern int evlatt_getinfo(const evlattribute_t *att, evlatt_info_t *info);
extern int evlatt_getstructtmpls(const evlattribute_t *att,
	const evltemplate_t **master, const evltemplate_t **clone);
extern int evlatt_getstructfromarray(const evlattribute_t *att, int index,
	const evltemplate_t **tmpl);

extern void _evlTmplSemanticError(const char *fmt, ...);
extern tmpl_type_info_t _evlTmplTypeInfo[];

extern tmpl_arch_type_info_t _evlTmplArchTypeInfo[][];

extern void _evlTmplDprintf(const char *fmt, ...);
extern int _evlEndsWith(const char *s, const char *suffix);
extern tmpl_attribute_t *_evlTmplGetNthAttribute(template_t *t, int n);
extern template_t *_evlAllocTemplate();
extern void _evlFreeTemplate(template_t *t);
extern template_t *_evlMakeEvrecTemplate(posix_log_facility_t facility,
	int eventType, const char *description);
extern template_t *_evlMakeStructTemplate(const char *structName,
	const char *description);
extern tmpl_data_type_t *_evlTmplAllocDataType();
extern tmpl_dimension_t *_evlTmplAllocDimension();
extern tmpl_delimiter_t *_evlTmplAllocDelimiter();
extern tmpl_type_and_value_t *_evlTmplAllocTypeAndValue();
extern tmpl_attribute_t *_evlTmplAllocAttribute();
extern int _evlTmplAddAttribute(template_t *tmpl, int constAttr,
	tmpl_data_type_t *type, const char *name, tmpl_dimension_t *dim,
	tmpl_type_and_value_t *val, char *format, tmpl_delimiter_t *delimiter);
extern void _evlTmplAddFormatSpec(template_t *tmpl, char *fmtSpec);
extern template_t *_evlCloneTemplate(template_t *t);
extern long _evlTmplGetValueOfIntegerAttribute(const tmpl_attribute_t *att);
extern tmpl_attribute_t *_evlTmplFindAttribute(const template_t *tmpl,
	const char *name);
extern void _evlTmplWrapup(template_t *t);
extern void _evlTmplIncRef(template_t *t);
extern void _evlTmplDecRef(template_t *t);
extern struct redirectedAttribute *_evlMkRedirAtt(template_t *t,
	unsigned int n, evl_list_t *scopedId, int att);
extern void _evlAddRedirection(template_t *t, struct redirectedAttribute *rf,
	struct redirectedAttribute *ret);

/* tmplmgmt.c */
extern int evl_parsetemplates(const char *source_filename,
	evltemplate_t *template_list[], size_t listsize, size_t *ntemplates,
	FILE *error_file, const char *prog_name);
extern int evl_writetemplates(const char *directory,
	const evltemplate_t *template_list[], size_t listsize);
extern int evl_readtemplate(posix_log_facility_t facility, int event_type,
        evltemplate_t **tmpl, int clone);
extern int evl_freetemplate(evltemplate_t *tmpl);
extern int evl_releasetemplate(evltemplate_t *tmpl);
extern int evltemplate_initmgr(int flags);

extern template_t *_evlFindStructTemplate(const char *structName);
extern tmpl_struct_ref_t *_evlFindStructRef(const char *structPath);
extern int _evlImportTemplateFromIdList(evl_list_t *list, int dotStar);
extern template_t *_evlImportTemplate(const char *primaryDir,
	const char *structPath, int purpose);
extern char *_evlGetParentDir(const char *path);
extern tmpl_struct_ref_t *_evlTmplMakeStructRef(template_t *t,
	const char *structPath);
extern char *_evlMakeDotPathFromList(evl_list_t *list);
extern void _evlTmplMakeEvlogName(posix_log_facility_t fac, int eventType,
        char *buf, size_t size);

/* tmplfmt.c */
extern int evlatt_getstring(const evlattribute_t *att, char *buf,
	size_t buflen);
extern int evltemplate_formatrec(const evltemplate_t *t, char *buf,
	size_t buflen);
extern int evltemplate_neqvdump(const evltemplate_t *t, char *buf,
	size_t buflen);

extern evl_list_t *_evlTmplParseFormat(template_t *t, char *fmtSpec);
extern int _evlSpecialFormatEvrec(const struct posix_log_entry *entry,
	const void *evBuf, const evltemplate_t *tmpl, const evl_list_t *list,
	char *fmtBuf, size_t fmtBufLen, size_t *reqLen);

/* bvfmt.c */
extern tmpl_bitmap_t *_evlTmplCollectVformat(char *fmt, int vFmtOnly, char **endFmt);
extern tmpl_bitmap_t *_evlTmplCollectBformat(char *fmt, int bFmtOnly, char **endFmt);

/* serial.c */
extern int _evlWriteTemplate(const template_t *t, const char *path);
extern template_t *_evlReadTemplate(const char *path);

/* tmplgram.y */
extern tmpl_parser_context_t *_evlTmplGetParserContext();
void _evlTmplClearParserErrors(void);

/* convert */
void _evl_conv_scalar(int arch, tmpl_base_type_t type,  void * data);
void * _evl_conv_scalar_array(int arch, tmpl_base_type_t type, char * data, 
	int nel, int el_size);
char * _evl_conv_wstring(int arch, char * data); 
char * _evl_conv_wstring_array(int arch, char * data, int nel); 

#ifdef __cplusplus
}
#endif

#endif /* _EVL_TEMPLATE_H_ */
