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

/*
Template grammar

Note:
This grammar has 1 shift-reduce conflict, which arises from the ambiguity
in the following valid statement:
	string s = "Help!" "%-20s";
This statement is correctly (but perhaps counter-intuitively) parsed as
	string s = "Help!%-20s";
To suppress string concatenation in this case, do
	string s = {"Help!"} "%-20s";
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <wchar.h>
#include <limits.h>

#include "evl_template.h"

void yyerror(const char *);
posix_log_facility_t evltc_default_facility = EVL_INVALID_FACILITY;

static char *wcsToMbs(wchar_t *ws);
static int makeIntType(evl_list_t *modifiers);
static tmpl_parser_context_t parserContext;

static template_t *template = 0;	/* The template we build */
static int attrClass = 0;
static int earlyTmplFlags = 0x0; /* Flags accumulated before template created */
%}

%union {
	int		ival;
	unsigned int	uival;
	unsigned long	ulval;
	long		lval;
	double		dval;
	char		*sval;
	char		**sarrval;
	int		cval;	/* character */
	wchar_t		wcval;
	wchar_t		*wsval;
	tmpl_data_type_t	*tyval;		/* attribute's data type */
	tmpl_dimension_t	*dimval;	/* attribute's dimension */
	tmpl_type_and_value_t	*valval;	/* initializer */
	tmpl_delimiter_t	*delimval;	/* attribute's delimiter */
	tmpl_redirection_t	*redir;
	struct redirectedAttribute *rdatt;
	evl_list_t	*listval;
	posix_log_facility_t facval;
}

%token <ival> TK_DELIMITER
%token <ival> TK_TYPENAME
%token <sval> TK_NAME
%token <ulval> TK_INTEGER
%token <dval> TK_DOUBLE
%token <sval> TK_STRING
/* TK_CHAR = char constant; TK_TYCHAR = "char" keyword */
%token <cval> TK_CHAR
%token <wsval> TK_WSTRING
%token <wcval> TK_WCHAR
%token <sval> TK_FORMAT
%token <ival> TK_FORMAT_STRING
%token <ival> TK_FORMAT_WSTRING

%token <ival> TK_ALIGNED TK_ATTRIBUTES TK_CONST TK_DESCRIPTION TK_DEFAULT
%token <ival> TK_EVENT_TYPE TK_FACILITY TK_IMPORT TK_STRUCT TK_TYPEDEF
%token <ival> TK_REDIRECT

%token <ival> TK_TYCHAR TK_TYDOUBLE TK_TYINT TK_TYLONG TK_TYSHORT
%token <ival> TK_TYSIGNED TK_TYUNSIGNED

%token <ival> ERRTOK

%type <delimval> delimiterSpec
%type <facval> facilityStmt facilityInt facilityString
%type <ival> eventTypeStmt eventTypeInt eventTypeString
%type <sval> description optionalDescription
%type <sval> structHeaderStmt
%type <ival> typeName intModifier
%type <sval> attFormatSpec
%type <sval> identifier
%type <uival> integerConstant
%type <cval> charConstant
%type <wcval> wcharConstant
%type <lval> signedIntegerConstant
%type <sval> stringLiteral
%type <wsval> wstringLiteral
%type <dval> doubleConstant
%type <tyval> typeSpec
%type <dimval> dimensionSpec
%type <ival> bitField
%type <valval> scalarConstant initializer initializerList initSpec
%type <listval> intModifiers scopedIdentifier
%type <cval> optionalStar
%type <ival> optionalStructFlags
%type <rdatt> redirectedFacility
%type <rdatt> redirectedEventType

%%
template	: importSection headerSection attributesSection
			optionalRedirect tmplFormatSpec
		;

/*
 * "import" statements
 */
importSection	: /* NULL */
		| importSection importOrTypedef
		;

importOrTypedef	: importStmt
		| typedefStmt
		;

importStmt	: TK_IMPORT importPredicate	{}
		;
		
importPredicate	: scopedIdentifier optionalStar ';'	{
				(void) _evlImportTemplateFromIdList($1, $2);
				_evlFreeList($1, 1);
			}
		| error ';'
		;

optionalStar	: '.' '*'	{ $$ = 1;}
		| /*NULL*/	{ $$ = 0; }
		;

/* Let semantic analysis weed out bit-fields, initializers, etc. */
typedefStmt	: TK_TYPEDEF { attrClass=EVL_ATTR_TYPEDEF; } attributeStmt
		;

/*
 * Template header
 */

headerSection	: recordHeader
		| structHeader
		| error ';' {
				template = _evlMakeEvrecTemplate(0, 0, NULL);
				template->tm_flags |= TMPL_TF_ERROR;
				parserContext.pc_template = template;
			}
		;

recordHeader	: facilityStmt eventTypeStmt optionalDescription {
				template = _evlMakeEvrecTemplate($1, $2, $3);
				if (parserContext.pc_errors) {
					template->tm_flags |= TMPL_TF_ERROR;
				}
				parserContext.pc_template = template;
			}
		;

structHeader	: structHeaderStmt optionalDescription {
				template = _evlMakeStructTemplate($1, $2);
				if (parserContext.pc_errors) {
					template->tm_flags |= TMPL_TF_ERROR;
				}
				template->tm_flags |= earlyTmplFlags;
				parserContext.pc_template = template;
			}
		;

facilityStmt	: facilityInt {
				if ($1 != EVL_INVALID_FACILITY) {
					evltc_default_facility = $1;
				}
			}
		| facilityString {
				if ($1 != EVL_INVALID_FACILITY) {
					evltc_default_facility = $1;
				}
			}
		| /*NULL*/ {
				if (evltc_default_facility == 
				    EVL_INVALID_FACILITY) {
					_evlTmplSemanticError(
						"No valid facility specified.");
				}
				$$ = evltc_default_facility;
			}
		;

facilityInt	: TK_FACILITY integerConstant ';'	{
				posix_log_facility_t fac = (posix_log_facility_t) $2;
				
				if ($2 > UINT_MAX) {
					_evlTmplSemanticError(
						"Facility code out of range: %lu", $2);
				}
				$$ = fac;
				if (fac == EVL_INVALID_FACILITY) {
					_evlTmplSemanticError(
						"Invalid facility code: %u", fac);
				}
			}
		;

facilityString	: TK_FACILITY stringLiteral ';'	{
				posix_log_facility_t fac;
				if (posix_log_strtofac($2, &fac) != 0) {
					_evlTmplSemanticError(
						"Unknown facility: %s", $2);
					$$ = EVL_INVALID_FACILITY;
				} else {
					$$ = fac;
				}
				free($2);
			}
		;

eventTypeStmt	: eventTypeInt
		| eventTypeString
		| TK_EVENT_TYPE TK_DEFAULT ';' {
				$$ = EVL_DEFAULT_EVENT_TYPE;
			}
		;

eventTypeInt	: TK_EVENT_TYPE signedIntegerConstant ';' {
				$$ = (int) $2;
				if ($2 == EVL_INVALID_EVENT_TYPE) {
					_evlTmplSemanticError(
						"Invalid event type: %d", $2);
				}
			}
		;

eventTypeString	: TK_EVENT_TYPE stringLiteral ';' {
				$$ = evl_gen_event_type_v2($2);
				if ($$ == EVL_INVALID_EVENT_TYPE) {
					_evlTmplSemanticError(
						"String maps to invalid event type (%d)",
						EVL_INVALID_EVENT_TYPE);
				}
				if (!parserContext.pc_template) {
					/* still collecting header */
					parserContext.pc_dfltdesc = $2;
				} else {
					free($2);
				}
			}
		;

description	: TK_DESCRIPTION stringLiteral ';' {
				$$ = $2;
			}
		;

optionalDescription	: description { free(parserContext.pc_dfltdesc); }
		| /*NULL*/	{ $$ = parserContext.pc_dfltdesc; }
		;

structHeaderStmt	: optionalStructFlags TK_STRUCT identifier ';' {
				$$ = $3;
				if (_evlFindStructRef($3)) {
					_evlTmplSemanticError(
"struct %s previously defined or imported", $3);
				}
			}
		;

optionalStructFlags	: TK_CONST { earlyTmplFlags = TMPL_TF_CONST; }
		| /*NULL*/	{ earlyTmplFlags = 0x0; }
		;

/*
 * Attributes section
 */

attributesSection	: constAttributes recordAttributes
		| constAttributes
		| recordAttributes
		| /*NULL*/
		;

constAttributes	: TK_CONST '{' { attrClass=EVL_ATTR_CONST; } attributeStmts '}'
		;

optionalAligned	: TK_ALIGNED	{ template->tm_flags |= TMPL_TF_ALIGNED; }
		| /*NULL*/
		;

recordAttributes  : optionalAligned TK_ATTRIBUTES '{' { attrClass=0; } attributeStmts '}'
		;

attributeStmts	: attributeStmt
		| attributeStmts attributeStmt
		;

attributeStmt	: typeSpec identifier dimensionSpec initSpec attFormatSpec delimiterSpec ';' {
				_evlTmplAddAttribute(template, attrClass,
					$1, $2, $3, $4, $5, $6);
			}
		| typeName bitField ';'	{
				/* Unnamed bit-field */
				tmpl_data_type_t *t = _evlTmplAllocDataType();
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				t->tt_base_type = $1;
				dim->td_type = TMPL_DIM_BITFIELD;
				dim->td_dimension = $2;
				_evlTmplAddAttribute(template, attrClass, t,
					NULL, dim, NULL, NULL, NULL);
			}
		| error ';'
		;

typeSpec	: typeName {
				tmpl_data_type_t *t = _evlTmplAllocDataType();
				t->tt_base_type = $1;
				$$ = t;
			}
		| TK_STRUCT scopedIdentifier {
				extern char *_evlMakeSlashPathFromList(
					evl_list_t *list);
				tmpl_data_type_t *t = _evlTmplAllocDataType();
				t->tt_base_type = TY_STRUCTNAME;
				t->u.st_name = _evlMakeSlashPathFromList($2);
				_evlFreeList($2, 1);
				$$ = t;
			}
		| identifier {
				tmpl_data_type_t *t = _evlTmplAllocDataType();
				t->tt_base_type = TY_TYPEDEF;
				t->u.td_name = $1;
				$$ = t;
			}
		;

dimensionSpec	: '[' integerConstant ']' {
				tmpl_dimension_t *dim;
				if ( $2 > INT_MAX ) {
					_evlTmplSemanticError("Dimension out of range: %lu", $2);
				}
				dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_CONST;
				dim->td_dimension = (int) $2;
				$$ = dim;
			}
		| '[' scopedIdentifier ']' {
				/*
				 * Note that we throw away some info here by
				 * changing { x, y, z } back to "x.y.z".
				 * This shouldn't be a big deal so long as
				 * the use of struct members as dimensions
				 * is fairly rare OR _evlTmplFindAttribute()
				 * is quick.  The latter, at least, seems to
				 * be true.  If we go to a more efficient
				 * implementation here, also consider using
				 * it for attribute refs in the template's
				 * format section.
				 */
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_ATTNAME;
				dim->u.u_attname = _evlMakeDotPathFromList($2);
				_evlFreeList($2, 1);
				$$ = dim;
			}
		| '[' '*' ']' {
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_REST;
				$$ = dim;
			}
		| '[' ']' {
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_IMPLIED;
				dim->td_dimension = 0;
				$$ = dim;
			}
		| bitField {
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_BITFIELD;
				dim->td_dimension = $1;
				$$ = dim;
			}
		| /*NULL*/	{ $$ = NULL; }
		;

bitField	: ':' integerConstant	{
				if ($2 > INT_MAX) {
					_evlTmplSemanticError(
					"bit field size out of range: %lu", $2);
				} 
				$$ = (int) $2; 
			}

		;

initSpec	: '=' initializer	{ $$ = $2; }
		| /*NULL*/		{ $$ = NULL; }
		;

initializer	: scalarConstant
		| '{' initializerList '}'	{ $$ = $2; }
		;

initializerList	: initializer	{
				/* Initializer -> initializer list */
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				evl_list_t *list = _evlMkListNode($1);
				tv->tv_type = TY_LIST;
				tv->tv_value.val_list = list;
				$$ = tv;
			}
		| initializerList ',' initializer {
				evl_list_t *list = $1->tv_value.val_list;
				(void) _evlAppendToList(list, $3);
				$$ = $1;
			}
		;

scalarConstant	: TK_INTEGER	{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_ULONG;
				tv->tv_value.val_ulong = $1;
				$$ = tv;
			}
		| '-' TK_INTEGER	{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_LONG;
				tv->tv_value.val_long = (long) -$2;
				$$ = tv;
			}
		| charConstant {
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_LONG;
				tv->tv_value.val_long = $1;
				$$ = tv;
			}
		| wcharConstant {
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_LONG;
				tv->tv_value.val_long = $1;
				$$ = tv;
			}
		| doubleConstant {
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_DOUBLE;
				tv->tv_value.val_double = $1;
				$$ = tv;
			}
		| stringLiteral {
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_STRING;
				tv->tv_value.val_string = $1;
				$$ = tv;
			}
		| wstringLiteral {
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_WSTRING;
				tv->tv_value.val_wstring = $1;
				$$ = tv;
			}
		;

attFormatSpec	: stringLiteral
		| /* NULL */	{ $$ = NULL; }
		;

delimiterSpec 	: TK_DELIMITER '=' stringLiteral	{
				tmpl_delimiter_t *de = _evlTmplAllocDelimiter();
				de->de_type = TMPL_DELIM_CONST;
				de->de_delimiter = $3;
				$$ = de;
			}
		| TK_DELIMITER '=' scopedIdentifier {
				/* Handle like an array dimension. */
				tmpl_delimiter_t *de = _evlTmplAllocDelimiter();
				de->de_type = TMPL_DELIM_ATTNAME; /* for now */
				de->de_delimiter = NULL;
				de->u.u_attname = _evlMakeDotPathFromList($3);
				_evlFreeList($3, 1);
				$$ = de;
			}
		| /* NULL */	{ $$ = NULL; }
		;

/*
 * The TK_FORMAT token carries the whole template format specification
 * that follows it.
 */
tmplFormatSpec	: TK_FORMAT	{
				_evlTmplAddFormatSpec(template, $1);
				_evlTmplWrapup(template);
			}
		| TK_FORMAT_STRING stringLiteral	{
				_evlTmplAddFormatSpec(template, $2);
				_evlTmplWrapup(template);
			}
		| TK_FORMAT_WSTRING wstringLiteral	{
				char *s = wcsToMbs($2);
				if (s) {
					_evlTmplAddFormatSpec(template, s);
				} else {
					fprintf(stderr,
"Internal error: Cannot convert wstring format specification.\n");
					template->tm_flags |= TMPL_TF_ERROR;
				}
				_evlTmplWrapup(template);
			}
		;

optionalRedirect : redirect
		| /*NULL*/
		;

redirect	: TK_REDIRECT redirection {
				_evlTmplWrapup(template);
			}
		;

redirection	: '{' redirectBody '}'
		;

redirectBody	: redirectedFacility {
				_evlAddRedirection(template, $1, NULL);
			}
		| redirectedFacility redirectedEventType {
				_evlAddRedirection(template, $1, $2);
			}
		| redirectedEventType redirectedFacility {
				_evlAddRedirection(template, $2, $1);
			}
		| redirectedEventType {
				_evlAddRedirection(template, NULL, $1);
			}
		;

redirectedFacility : facilityInt {
				$$ = _evlMkRedirAtt(template, $1, NULL,
					POSIX_LOG_ENTRY_FACILITY);
			}
		| facilityString {
				$$ = _evlMkRedirAtt(template, $1, NULL,
					POSIX_LOG_ENTRY_FACILITY);
			}
		| TK_FACILITY scopedIdentifier ';' {
				$$ = _evlMkRedirAtt(template, 0, $2,
					POSIX_LOG_ENTRY_FACILITY);
			}
		;

redirectedEventType : eventTypeInt {
				$$ = _evlMkRedirAtt(template, $1, NULL,
					POSIX_LOG_ENTRY_EVENT_TYPE);
			}
		| eventTypeString {
				$$ = _evlMkRedirAtt(template, $1, NULL,
					POSIX_LOG_ENTRY_EVENT_TYPE);
			}
		| TK_EVENT_TYPE scopedIdentifier ';' {
				$$ = _evlMkRedirAtt(template, 0, $2,
					POSIX_LOG_ENTRY_EVENT_TYPE);
			}
		;

/*
 * Fundamental elements
 */

integerConstant	: TK_INTEGER	{ $$ = $1; }
		;

signedIntegerConstant	: TK_INTEGER	{ $$ = $1; }
		| '-' TK_INTEGER	{ $$ = -$2; }
		;

charConstant	: TK_CHAR
		;

wcharConstant	: TK_WCHAR
		;

stringLiteral	: TK_STRING
		| stringLiteral TK_STRING {
				/* Concatenate adjacent string literals. */
				char *s1 = $1;
				char *s2 = $2;
				char *snew = realloc(s1,
					strlen(s1)+strlen(s2)+1);
				assert(snew != NULL);
				(void) strcat(snew, s2);
				free(s2);
				$$ = snew;
			}
		;

wstringLiteral	: TK_WSTRING
		| wstringLiteral TK_WSTRING {
				/* Concatenate adjacent wide string literals. */
				wchar_t *s1 = $1;
				wchar_t *s2 = $2;
				size_t len1 = wcslen(s1);
				size_t len2 = wcslen(s2);
				wchar_t *snew = realloc(s1,
					(len1+len2+1) * sizeof(wchar_t));
				assert(snew != NULL);
				(void) memcpy(snew + len1, s2,
					(len2+1)*sizeof(wchar_t));
				free(s2);
				$$ = snew;
			}
		;

doubleConstant	: TK_DOUBLE
		| '-' TK_DOUBLE	{ $$ = -$2; }
		;

identifier	: TK_NAME
		;

typeName	: TK_TYPENAME
		| TK_TYCHAR	{
#if (CHAR_MIN == 0)
				$$ = TY_UCHAR;
#else
				$$ = TY_CHAR;
#endif
			}
		| TK_TYSIGNED TK_TYCHAR		{ $$ = TY_CHAR; }
		| TK_TYUNSIGNED TK_TYCHAR	{ $$ = TY_UCHAR; }
		| TK_TYDOUBLE			{ $$ = TY_DOUBLE; }
		| TK_TYLONG TK_TYDOUBLE		{ $$ = TY_LDOUBLE; }
		| TK_TYINT			{ $$ = TY_INT; }
		| intModifiers optionalInt	{
				$$ = makeIntType($1);
				_evlFreeList($1, 0);
			}
		;

optionalInt	: TK_TYINT 	{}
		| /* NULL */	{}
		;

intModifier	: TK_TYLONG	{ $$ = TK_TYLONG; }
		| TK_TYSHORT	{ $$ = TK_TYSHORT; }
		| TK_TYSIGNED	{ $$ = TK_TYSIGNED; }
		| TK_TYUNSIGNED	{ $$ = TK_TYUNSIGNED; }
		;

intModifiers	: intModifier {
				$$ = _evlMkListNode((void*)($1));
			}
		| intModifiers intModifier {
				$$ = _evlAppendToList($1, (void*)($2));
			}
		;

scopedIdentifier	: identifier	{
				$$ = _evlMkListNode($1);
			}
		| scopedIdentifier '.' identifier	{
				$$ = _evlAppendToList($1, $3);
			}
		;
%%

void
yyerror(const char *s)
{
	fprintf(parserContext.pc_errfile, "%s:%d: %s\n",
		parserContext.pc_pathname, parserContext.pc_lineno, s);
}

void
_evlTmplClearParserErrors()
{
	parserContext.pc_errors = 0;
}

tmpl_parser_context_t *
_evlTmplGetParserContext()
{
	return &parserContext;
}

/*
 * ws is an array of wide characters terminating in L'\0'.  Malloc room
 * for the corresponding multibyte string, write the multibyte string there,
 * and return a pointer to it.  Returns NULL if we can't do the conversion.
 */
static char *
wcsToMbs(wchar_t *ws)
{
	size_t nb1, nb2;
	char *mbs;
	nb1 = wcstombs(NULL, ws, 0);
	if (nb1 == (size_t)-1) {
		return NULL;
	}
	mbs = (char*) malloc(nb1+1);
	assert(mbs != NULL);
	nb2 = wcstombs(mbs, ws, nb1+1);
	assert(nb1 == nb2);
	return mbs;
}

/*
 * modifiers is a list of words from the following set: long, short, signed,
 * unsigned.  Return the implied type code -- e.g., TY_ULONGLONG for
 * {unsigned,long,long} or {long,unsigned,long} or {long,long,unsigned}.
 * On error, call _evlTmplSemanticError() and return TY_INT.
 */
static int
makeIntType(evl_list_t *modifiers)
{
	int size = TY_INT;	/* or TY_LONG, TY_LONGLONG, or TY_SHORT */
	int sign = -1;		/* or TK_TYSIGNED or TK_TYUNSIGNED */
	evl_list_t *head = modifiers, *end, *p;

	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		/* Note: s390x requires double cast here. */
		int modifier = (int) (long) p->li_data;
		switch (modifier) {
		case TK_TYLONG:
			if (size == TY_LONG) {
				size = TY_LONGLONG;
			} else if (size == TY_INT) {
				size = TY_LONG;
			} else {
				goto badType;
			}
			break;
		case TK_TYSHORT:
			if (size == TY_INT) {
				size = TY_SHORT;
			} else {
				goto badType;
			}
			break;
		case TK_TYSIGNED:
		case TK_TYUNSIGNED:
			if (sign == -1) {
				sign = modifier;
			} else {
				goto badType;
			}
			break;
		default:
			assert(0);
		}
	}
	if (sign == TK_TYUNSIGNED) {
		switch (size) {
		case TY_INT:		return TY_UINT;
		case TY_SHORT:		return TY_USHORT;
		case TY_LONG:		return TY_ULONG;
		case TY_LONGLONG:	return TY_ULONGLONG;
		default:		assert(0);
		}
	} else {
		return size;
	}

badType:
	_evlTmplSemanticError("Nonsensical integer type");
	return TY_INT;
}
