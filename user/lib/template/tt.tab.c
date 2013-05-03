#ifndef lint
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28 2000/01/17 02:04:06 bde Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
static int yygrowstack();
#define yyparse ttparse
#define yylex ttlex
#define yyerror tterror
#define yychar ttchar
#define yyval ttval
#define yylval ttlval
#define yydebug ttdebug
#define yynerrs ttnerrs
#define yyerrflag tterrflag
#define yyss ttss
#define yyssp ttssp
#define yyvs ttvs
#define yyvsp ttvsp
#define yylhs ttlhs
#define yylen ttlen
#define yydefred ttdefred
#define yydgoto ttdgoto
#define yysindex ttsindex
#define yyrindex ttrindex
#define yygindex ttgindex
#define yytable tttable
#define yycheck ttcheck
#define yyname ttname
#define yyrule ttrule
#define yysslim ttsslim
#define yystacksize ttstacksize
#define YYPREFIX "tt"
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
typedef union {
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
} YYSTYPE;
#define YYERRCODE 256
#define TK_DELIMITER 257
#define TK_TYPENAME 258
#define TK_NAME 259
#define TK_INTEGER 260
#define TK_DOUBLE 261
#define TK_STRING 262
#define TK_CHAR 263
#define TK_WSTRING 264
#define TK_WCHAR 265
#define TK_FORMAT 266
#define TK_FORMAT_STRING 267
#define TK_FORMAT_WSTRING 268
#define TK_ALIGNED 269
#define TK_ATTRIBUTES 270
#define TK_CONST 271
#define TK_DESCRIPTION 272
#define TK_DEFAULT 273
#define TK_EVENT_TYPE 274
#define TK_FACILITY 275
#define TK_IMPORT 276
#define TK_STRUCT 277
#define TK_TYPEDEF 278
#define TK_REDIRECT 279
#define TK_TYCHAR 280
#define TK_TYDOUBLE 281
#define TK_TYINT 282
#define TK_TYLONG 283
#define TK_TYSHORT 284
#define TK_TYSIGNED 285
#define TK_TYUNSIGNED 286
#define ERRTOK 287
const short ttlhs[] = {                                        -1,
    0,   35,   35,   40,   40,   41,   43,   43,   31,   31,
   45,   42,   36,   36,   36,   46,   47,    2,    2,    2,
    3,    4,    5,    5,    5,    6,    7,    8,    9,    9,
   10,   32,   32,   37,   37,   37,   37,   51,   48,   52,
   52,   53,   49,   50,   50,   44,   44,   44,   22,   22,
   22,   23,   23,   23,   23,   23,   23,   24,   28,   28,
   26,   26,   27,   27,   25,   25,   25,   25,   25,   25,
   25,   13,   13,    1,    1,    1,   39,   39,   39,   38,
   38,   54,   55,   56,   56,   56,   56,   33,   33,   33,
   34,   34,   34,   15,   18,   18,   16,   17,   19,   19,
   20,   20,   21,   21,   14,   11,   11,   11,   11,   11,
   11,   11,   11,   57,   57,   12,   12,   12,   12,   29,
   29,   30,   30,
};
const short ttlen[] = {                                         2,
    5,    0,    2,    1,    1,    2,    3,    2,    2,    0,
    0,    3,    1,    1,    2,    3,    2,    1,    1,    0,
    3,    3,    1,    1,    3,    3,    3,    3,    1,    0,
    4,    1,    0,    2,    1,    1,    0,    0,    5,    1,
    0,    0,    6,    1,    2,    7,    3,    2,    1,    2,
    1,    3,    3,    3,    2,    1,    0,    2,    2,    0,
    1,    3,    1,    3,    1,    2,    1,    1,    1,    1,
    1,    1,    0,    3,    3,    0,    1,    2,    2,    1,
    0,    2,    3,    1,    2,    2,    1,    1,    1,    3,
    1,    1,    3,    1,    1,    2,    1,    1,    1,    2,
    1,    2,    1,    2,    1,    1,    1,    2,    2,    1,
    2,    1,    2,    1,    0,    1,    1,    1,    1,    1,
    2,    1,    3,
};
const short ttdefred[] = {                                      2,
    0,    0,    0,   32,    0,    0,   11,    0,   18,   19,
    0,    0,    0,    3,    4,    5,   13,   14,   15,   94,
   99,    0,    0,    0,  105,  122,    0,    6,    0,    0,
    0,   23,   24,    0,   29,   17,    0,   40,    0,    0,
    0,   36,    0,   21,  100,   22,    8,    0,    0,    0,
  106,    0,  107,  110,  112,    0,  117,    0,    0,    0,
  120,   51,    0,    0,   12,   95,    0,    0,    0,    0,
   16,    0,    0,   38,    0,    0,   80,   34,    0,    9,
  123,    7,   48,    0,  111,  108,  109,    0,    0,    0,
  114,  116,  118,  119,  121,  113,   25,   96,   26,   27,
   28,   31,    0,    0,   82,   77,    0,    0,    1,   42,
    0,   58,   47,    0,    0,   56,   44,    0,    0,    0,
   88,   89,   91,   92,    0,    0,    0,    0,  101,    0,
    0,    0,   55,    0,    0,    0,    0,   39,   45,    0,
    0,   85,   86,   83,  102,    0,   54,   52,   53,   65,
  103,   97,   98,    0,    0,   67,   68,    0,    0,   69,
   61,   59,    0,    0,   93,   90,   43,   63,    0,   66,
  104,    0,    0,   62,    0,    0,   46,   64,    0,    0,
};
const short ttdgoto[] = {                                       1,
  173,    8,  121,  122,   31,  123,  124,   35,   36,   11,
   60,   61,  163,   26,   22,  156,  157,   69,  158,  159,
  160,   63,  115,   89,  161,  162,  169,  137,   64,   27,
   49,   12,  125,  126,    2,   13,   40,   76,  109,   14,
   15,   16,   28,  117,   29,   17,   18,   41,   42,  118,
  103,   43,  131,   77,  105,  127,   96,
};
const short ttsindex[] = {                                      0,
    0, -221,  -30,    0, -178, -231,    0, -241,    0,    0,
 -234, -196, -162,    0,    0,    0,    0,    0,    0,    0,
    0,   35,  -40,   36,    0,    0,   -3,    0, -212,  -21,
 -234,    0,    0, -156,    0,    0, -147,    0,   -5, -152,
 -134,    0, -130,    0,    0,    0,    0,  -36,   64,   82,
    0, -147,    0,    0,    0, -136,    0, -132, -131,   92,
    0,    0, -147, -153,    0,    0,   93, -109,   94,  -20,
    0,  -14,   95,    0,   33, -169,    0,    0,   34,    0,
    0,    0,    0,  120,    0,    0,    0,  -93,  116,  -38,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -212, -216,    0,    0, -156,  -88,    0,    0,
 -147,    0,    0,  -16,  117,    0,    0, -122,  -13, -184,
    0,    0,    0,    0,  -97,  -96,   55,  -81,    0,  -80,
 -212,   90,    0,   96,  -31,  -44, -156,    0,    0,    2,
    5,    0,    0,    0,    0, -112,    0,    0,    0,    0,
    0,    0,    0,  -44, -194,    0,    0,  -81,  -80,    0,
    0,    0,  -72,  -81,    0,    0,    0,    0,  -32,    0,
    0,  125,  128,    0,  -44, -225,    0,    0,  -81,  120,
};
const short ttrindex[] = {                                      0,
    0, -214,    0,    0,    0,    0,    0,    0,    0,    0,
 -179,    0, -165,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  129,    0,    0,    0,
 -179,    0,    0,    0,    0,    0,    0,    0,    0, -146,
 -151,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  -58,    0,  -53,  -48,  -69,
    0,    0,    0,  -50,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -68,    0,    0,    0,    0,    0,  -52,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -55,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   67,   68,    0,  194,    0,  195,
    0,    0,    0,    0,    0,    0,  -45,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -42,  -17,    0,
    0,    0,  137,  -43,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  138,  139,
};
const short ttgindex[] = {                                      0,
    0,    0,  197,  198,    0,  196,  200,    0,  172,    0,
    0,  149,    0,   -7,  -65,    0,    0,    0,    6,  142,
    0,    0,    0,  161,    0, -123,    0,    0,    0,  -34,
    0,    0,  127,  130,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   -8,    0,    0,    0,    0,  213,  126,
    0,    0,    0,    0,    0,    0,    0,
};
#define YYTABLESIZE 257
const short tttable[] = {                                     116,
  155,   70,  138,   60,  118,   80,   57,  115,   57,  119,
   23,  175,  167,   73,  111,   72,   70,   84,   46,   88,
   65,   62,  112,   68,   24,  132,   71,   25,   19,   73,
  168,   68,   30,   25,    3,   70,   21,   34,  100,   72,
   81,   71,   48,   50,  101,   51,   25,  111,  134,    4,
  111,  178,  114,    5,    6,   90,    7,  119,  120,   20,
  165,  149,   33,  166,   52,  170,  171,   53,   54,   55,
   56,   57,   58,   59,   25,   20,  133,   21,  154,  135,
   37,   20,   70,   21,  140,  141,   30,   30,   30,   30,
   30,   30,  174,   44,   47,   62,  106,  107,  108,   30,
   37,   37,   37,   81,   41,   21,   38,   71,   39,  139,
   62,   25,  128,   37,   35,   35,   35,   74,   41,   81,
   81,   81,   82,   62,   70,   23,   75,   35,   91,   92,
   57,   93,   94,   50,   38,   51,   25,  139,   62,   79,
   83,  180,  164,   50,   85,   51,   25,   86,   87,   88,
   98,   97,   99,  102,   52,  104,  110,   53,   54,   55,
   56,   57,   58,   59,   52,  111,   20,   53,   54,   55,
   56,   57,   58,   59,  113,  129,  119,  136,  120,  144,
   45,  179,  147,  145,  172,  176,  177,   10,  148,   49,
   50,   84,   87,   78,   79,   76,   74,   75,    9,   10,
  116,   60,   71,   32,   57,  118,   60,   33,  115,   57,
  119,   73,   95,   72,   70,  150,  151,   21,  152,  129,
  153,   45,   25,  116,  116,  116,  116,  116,  118,  118,
  118,  118,  118,  119,  119,  119,  119,  119,   66,   71,
   21,   45,   25,   20,   71,   25,   66,   45,   21,  130,
  116,   67,  143,   78,  142,    0,  146,
};
const short ttcheck[] = {                                      58,
   45,   44,  125,   59,   58,   42,   59,   58,   61,   58,
    5,   44,  125,   59,   46,   59,   59,   52,   59,   58,
   29,   29,   88,   45,  256,   42,   44,  259,   59,   37,
  154,   45,  274,  259,  256,   30,  262,  272,   59,   34,
   48,   59,   46,  256,   59,  258,  259,   46,  114,  271,
   46,  175,   91,  275,  276,   63,  278,  274,  275,  274,
   59,   93,  277,   59,  277,  260,  261,  280,  281,  282,
  283,  284,  285,  286,  259,  260,   93,  262,  123,  114,
  277,  260,  125,  262,  119,  120,  266,  267,  268,  269,
  270,  271,  125,   59,   59,  103,  266,  267,  268,  279,
  266,  267,  268,  111,  270,  262,  269,  125,  271,  118,
  118,  259,  107,  279,  266,  267,  268,  123,  270,  266,
  267,  268,   59,  131,  119,  120,  279,  279,  282,  283,
  284,  285,  286,  256,  269,  258,  259,  146,  146,  270,
   59,  176,  137,  256,  281,  258,  259,  280,  280,   58,
  260,   59,   59,   59,  277,  123,  123,  280,  281,  282,
  283,  284,  285,  286,  277,   46,  260,  280,  281,  282,
  283,  284,  285,  286,   59,  264,  274,   61,  275,  125,
  262,  176,   93,  264,  257,   61,   59,   59,   93,  259,
  259,  125,  125,    0,    0,   59,   59,   59,    2,    2,
  259,  257,   31,    8,  257,  259,  262,    8,  259,  262,
  259,  257,   64,  257,  257,  260,  261,  262,  263,  264,
  265,  262,  259,  282,  283,  284,  285,  286,  282,  283,
  284,  285,  286,  282,  283,  284,  285,  286,  260,  257,
  262,  262,  259,  260,  262,  259,  260,  262,  262,  108,
   90,  273,  126,   41,  125,   -1,  131,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 287
#if YYDEBUG
const char * const ttname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,"'*'",0,"','","'-'","'.'",0,0,0,0,0,0,0,0,0,0,0,"':'","';'",0,
"'='",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'['",0,"']'",0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"TK_DELIMITER","TK_TYPENAME","TK_NAME","TK_INTEGER","TK_DOUBLE",
"TK_STRING","TK_CHAR","TK_WSTRING","TK_WCHAR","TK_FORMAT","TK_FORMAT_STRING",
"TK_FORMAT_WSTRING","TK_ALIGNED","TK_ATTRIBUTES","TK_CONST","TK_DESCRIPTION",
"TK_DEFAULT","TK_EVENT_TYPE","TK_FACILITY","TK_IMPORT","TK_STRUCT","TK_TYPEDEF",
"TK_REDIRECT","TK_TYCHAR","TK_TYDOUBLE","TK_TYINT","TK_TYLONG","TK_TYSHORT",
"TK_TYSIGNED","TK_TYUNSIGNED","ERRTOK",
};
const char * const ttrule[] = {
"$accept : template",
"template : importSection headerSection attributesSection optionalRedirect tmplFormatSpec",
"importSection :",
"importSection : importSection importOrTypedef",
"importOrTypedef : importStmt",
"importOrTypedef : typedefStmt",
"importStmt : TK_IMPORT importPredicate",
"importPredicate : scopedIdentifier optionalStar ';'",
"importPredicate : error ';'",
"optionalStar : '.' '*'",
"optionalStar :",
"$$1 :",
"typedefStmt : TK_TYPEDEF $$1 attributeStmt",
"headerSection : recordHeader",
"headerSection : structHeader",
"headerSection : error ';'",
"recordHeader : facilityStmt eventTypeStmt optionalDescription",
"structHeader : structHeaderStmt optionalDescription",
"facilityStmt : facilityInt",
"facilityStmt : facilityString",
"facilityStmt :",
"facilityInt : TK_FACILITY integerConstant ';'",
"facilityString : TK_FACILITY stringLiteral ';'",
"eventTypeStmt : eventTypeInt",
"eventTypeStmt : eventTypeString",
"eventTypeStmt : TK_EVENT_TYPE TK_DEFAULT ';'",
"eventTypeInt : TK_EVENT_TYPE signedIntegerConstant ';'",
"eventTypeString : TK_EVENT_TYPE stringLiteral ';'",
"description : TK_DESCRIPTION stringLiteral ';'",
"optionalDescription : description",
"optionalDescription :",
"structHeaderStmt : optionalStructFlags TK_STRUCT identifier ';'",
"optionalStructFlags : TK_CONST",
"optionalStructFlags :",
"attributesSection : constAttributes recordAttributes",
"attributesSection : constAttributes",
"attributesSection : recordAttributes",
"attributesSection :",
"$$2 :",
"constAttributes : TK_CONST '{' $$2 attributeStmts '}'",
"optionalAligned : TK_ALIGNED",
"optionalAligned :",
"$$3 :",
"recordAttributes : optionalAligned TK_ATTRIBUTES '{' $$3 attributeStmts '}'",
"attributeStmts : attributeStmt",
"attributeStmts : attributeStmts attributeStmt",
"attributeStmt : typeSpec identifier dimensionSpec initSpec attFormatSpec delimiterSpec ';'",
"attributeStmt : typeName bitField ';'",
"attributeStmt : error ';'",
"typeSpec : typeName",
"typeSpec : TK_STRUCT scopedIdentifier",
"typeSpec : identifier",
"dimensionSpec : '[' integerConstant ']'",
"dimensionSpec : '[' scopedIdentifier ']'",
"dimensionSpec : '[' '*' ']'",
"dimensionSpec : '[' ']'",
"dimensionSpec : bitField",
"dimensionSpec :",
"bitField : ':' integerConstant",
"initSpec : '=' initializer",
"initSpec :",
"initializer : scalarConstant",
"initializer : '{' initializerList '}'",
"initializerList : initializer",
"initializerList : initializerList ',' initializer",
"scalarConstant : TK_INTEGER",
"scalarConstant : '-' TK_INTEGER",
"scalarConstant : charConstant",
"scalarConstant : wcharConstant",
"scalarConstant : doubleConstant",
"scalarConstant : stringLiteral",
"scalarConstant : wstringLiteral",
"attFormatSpec : stringLiteral",
"attFormatSpec :",
"delimiterSpec : TK_DELIMITER '=' stringLiteral",
"delimiterSpec : TK_DELIMITER '=' scopedIdentifier",
"delimiterSpec :",
"tmplFormatSpec : TK_FORMAT",
"tmplFormatSpec : TK_FORMAT_STRING stringLiteral",
"tmplFormatSpec : TK_FORMAT_WSTRING wstringLiteral",
"optionalRedirect : redirect",
"optionalRedirect :",
"redirect : TK_REDIRECT redirection",
"redirection : '{' redirectBody '}'",
"redirectBody : redirectedFacility",
"redirectBody : redirectedFacility redirectedEventType",
"redirectBody : redirectedEventType redirectedFacility",
"redirectBody : redirectedEventType",
"redirectedFacility : facilityInt",
"redirectedFacility : facilityString",
"redirectedFacility : TK_FACILITY scopedIdentifier ';'",
"redirectedEventType : eventTypeInt",
"redirectedEventType : eventTypeString",
"redirectedEventType : TK_EVENT_TYPE scopedIdentifier ';'",
"integerConstant : TK_INTEGER",
"signedIntegerConstant : TK_INTEGER",
"signedIntegerConstant : '-' TK_INTEGER",
"charConstant : TK_CHAR",
"wcharConstant : TK_WCHAR",
"stringLiteral : TK_STRING",
"stringLiteral : stringLiteral TK_STRING",
"wstringLiteral : TK_WSTRING",
"wstringLiteral : wstringLiteral TK_WSTRING",
"doubleConstant : TK_DOUBLE",
"doubleConstant : '-' TK_DOUBLE",
"identifier : TK_NAME",
"typeName : TK_TYPENAME",
"typeName : TK_TYCHAR",
"typeName : TK_TYSIGNED TK_TYCHAR",
"typeName : TK_TYUNSIGNED TK_TYCHAR",
"typeName : TK_TYDOUBLE",
"typeName : TK_TYLONG TK_TYDOUBLE",
"typeName : TK_TYINT",
"typeName : intModifiers optionalInt",
"optionalInt : TK_TYINT",
"optionalInt :",
"intModifier : TK_TYLONG",
"intModifier : TK_TYSHORT",
"intModifier : TK_TYSIGNED",
"intModifier : TK_TYUNSIGNED",
"intModifiers : intModifier",
"intModifiers : intModifiers intModifier",
"scopedIdentifier : identifier",
"scopedIdentifier : scopedIdentifier '.' identifier",
};
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;

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
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#if defined(lint) || defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 6:
{}
break;
case 7:
{
				(void) _evlImportTemplateFromIdList(yyvsp[-2].listval, yyvsp[-1].cval);
				_evlFreeList(yyvsp[-2].listval, 1);
			}
break;
case 9:
{ yyval.cval = 1;}
break;
case 10:
{ yyval.cval = 0; }
break;
case 11:
{ attrClass=EVL_ATTR_TYPEDEF; }
break;
case 15:
{
				template = _evlMakeEvrecTemplate(0, 0, NULL);
				template->tm_flags |= TMPL_TF_ERROR;
				parserContext.pc_template = template;
			}
break;
case 16:
{
				template = _evlMakeEvrecTemplate(yyvsp[-2].facval, yyvsp[-1].ival, yyvsp[0].sval);
				if (parserContext.pc_errors) {
					template->tm_flags |= TMPL_TF_ERROR;
				}
				parserContext.pc_template = template;
			}
break;
case 17:
{
				template = _evlMakeStructTemplate(yyvsp[-1].sval, yyvsp[0].sval);
				if (parserContext.pc_errors) {
					template->tm_flags |= TMPL_TF_ERROR;
				}
				template->tm_flags |= earlyTmplFlags;
				parserContext.pc_template = template;
			}
break;
case 18:
{
				if (yyvsp[0].facval != EVL_INVALID_FACILITY) {
					evltc_default_facility = yyvsp[0].facval;
				}
			}
break;
case 19:
{
				if (yyvsp[0].facval != EVL_INVALID_FACILITY) {
					evltc_default_facility = yyvsp[0].facval;
				}
			}
break;
case 20:
{
				if (evltc_default_facility == 
				    EVL_INVALID_FACILITY) {
					_evlTmplSemanticError(
						"No valid facility specified.");
				}
				yyval.facval = evltc_default_facility;
			}
break;
case 21:
{
				posix_log_facility_t fac = (posix_log_facility_t) yyvsp[-1].uival;
				
				if (yyvsp[-1].uival > UINT_MAX) {
					_evlTmplSemanticError(
						"Facility code out of range: %lu", yyvsp[-1].uival);
				}
				yyval.facval = fac;
				if (fac == EVL_INVALID_FACILITY) {
					_evlTmplSemanticError(
						"Invalid facility code: %u", fac);
				}
			}
break;
case 22:
{
				posix_log_facility_t fac;
				if (posix_log_strtofac(yyvsp[-1].sval, &fac) != 0) {
					_evlTmplSemanticError(
						"Unknown facility: %s", yyvsp[-1].sval);
					yyval.facval = EVL_INVALID_FACILITY;
				} else {
					yyval.facval = fac;
				}
				free(yyvsp[-1].sval);
			}
break;
case 25:
{
				yyval.ival = EVL_DEFAULT_EVENT_TYPE;
			}
break;
case 26:
{
				yyval.ival = (int) yyvsp[-1].lval;
				if (yyvsp[-1].lval == EVL_INVALID_EVENT_TYPE) {
					_evlTmplSemanticError(
						"Invalid event type: %d", yyvsp[-1].lval);
				}
			}
break;
case 27:
{
				yyval.ival = evl_gen_event_type_v2(yyvsp[-1].sval);
				if (yyval.ival == EVL_INVALID_EVENT_TYPE) {
					_evlTmplSemanticError(
						"String maps to invalid event type (%d)",
						EVL_INVALID_EVENT_TYPE);
				}
				if (!parserContext.pc_template) {
					/* still collecting header */
					parserContext.pc_dfltdesc = yyvsp[-1].sval;
				} else {
					free(yyvsp[-1].sval);
				}
			}
break;
case 28:
{
				yyval.sval = yyvsp[-1].sval;
			}
break;
case 29:
{ free(parserContext.pc_dfltdesc); }
break;
case 30:
{ yyval.sval = parserContext.pc_dfltdesc; }
break;
case 31:
{
				yyval.sval = yyvsp[-1].sval;
				if (_evlFindStructRef(yyvsp[-1].sval)) {
					_evlTmplSemanticError(
"struct %s previously defined or imported", yyvsp[-1].sval);
				}
			}
break;
case 32:
{ earlyTmplFlags = TMPL_TF_CONST; }
break;
case 33:
{ earlyTmplFlags = 0x0; }
break;
case 38:
{ attrClass=EVL_ATTR_CONST; }
break;
case 40:
{ template->tm_flags |= TMPL_TF_ALIGNED; }
break;
case 42:
{ attrClass=0; }
break;
case 46:
{
				_evlTmplAddAttribute(template, attrClass,
					yyvsp[-6].tyval, yyvsp[-5].sval, yyvsp[-4].dimval, yyvsp[-3].valval, yyvsp[-2].sval, yyvsp[-1].delimval);
			}
break;
case 47:
{
				/* Unnamed bit-field */
				tmpl_data_type_t *t = _evlTmplAllocDataType();
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				t->tt_base_type = yyvsp[-2].ival;
				dim->td_type = TMPL_DIM_BITFIELD;
				dim->td_dimension = yyvsp[-1].ival;
				_evlTmplAddAttribute(template, attrClass, t,
					NULL, dim, NULL, NULL, NULL);
			}
break;
case 49:
{
				tmpl_data_type_t *t = _evlTmplAllocDataType();
				t->tt_base_type = yyvsp[0].ival;
				yyval.tyval = t;
			}
break;
case 50:
{
				extern char *_evlMakeSlashPathFromList(
					evl_list_t *list);
				tmpl_data_type_t *t = _evlTmplAllocDataType();
				t->tt_base_type = TY_STRUCTNAME;
				t->u.st_name = _evlMakeSlashPathFromList(yyvsp[0].listval);
				_evlFreeList(yyvsp[0].listval, 1);
				yyval.tyval = t;
			}
break;
case 51:
{
				tmpl_data_type_t *t = _evlTmplAllocDataType();
				t->tt_base_type = TY_TYPEDEF;
				t->u.td_name = yyvsp[0].sval;
				yyval.tyval = t;
			}
break;
case 52:
{
				tmpl_dimension_t *dim;
				if ( yyvsp[-1].uival > INT_MAX ) {
					_evlTmplSemanticError("Dimension out of range: %lu", yyvsp[-1].uival);
				}
				dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_CONST;
				dim->td_dimension = (int) yyvsp[-1].uival;
				yyval.dimval = dim;
			}
break;
case 53:
{
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
				dim->u.u_attname = _evlMakeDotPathFromList(yyvsp[-1].listval);
				_evlFreeList(yyvsp[-1].listval, 1);
				yyval.dimval = dim;
			}
break;
case 54:
{
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_REST;
				yyval.dimval = dim;
			}
break;
case 55:
{
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_IMPLIED;
				dim->td_dimension = 0;
				yyval.dimval = dim;
			}
break;
case 56:
{
				tmpl_dimension_t *dim = _evlTmplAllocDimension();
				dim->td_type = TMPL_DIM_BITFIELD;
				dim->td_dimension = yyvsp[0].ival;
				yyval.dimval = dim;
			}
break;
case 57:
{ yyval.dimval = NULL; }
break;
case 58:
{
				if (yyvsp[0].uival > INT_MAX) {
					_evlTmplSemanticError(
					"bit field size out of range: %lu", yyvsp[0].uival);
				} 
				yyval.ival = (int) yyvsp[0].uival; 
			}
break;
case 59:
{ yyval.valval = yyvsp[0].valval; }
break;
case 60:
{ yyval.valval = NULL; }
break;
case 62:
{ yyval.valval = yyvsp[-1].valval; }
break;
case 63:
{
				/* Initializer -> initializer list */
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				evl_list_t *list = _evlMkListNode(yyvsp[0].valval);
				tv->tv_type = TY_LIST;
				tv->tv_value.val_list = list;
				yyval.valval = tv;
			}
break;
case 64:
{
				evl_list_t *list = yyvsp[-2].valval->tv_value.val_list;
				(void) _evlAppendToList(list, yyvsp[0].valval);
				yyval.valval = yyvsp[-2].valval;
			}
break;
case 65:
{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_ULONG;
				tv->tv_value.val_ulong = yyvsp[0].ulval;
				yyval.valval = tv;
			}
break;
case 66:
{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_LONG;
				tv->tv_value.val_long = (long) -yyvsp[0].ulval;
				yyval.valval = tv;
			}
break;
case 67:
{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_LONG;
				tv->tv_value.val_long = yyvsp[0].cval;
				yyval.valval = tv;
			}
break;
case 68:
{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_LONG;
				tv->tv_value.val_long = yyvsp[0].wcval;
				yyval.valval = tv;
			}
break;
case 69:
{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_DOUBLE;
				tv->tv_value.val_double = yyvsp[0].dval;
				yyval.valval = tv;
			}
break;
case 70:
{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_STRING;
				tv->tv_value.val_string = yyvsp[0].sval;
				yyval.valval = tv;
			}
break;
case 71:
{
				tmpl_type_and_value_t *tv = _evlTmplAllocTypeAndValue();
				tv->tv_type = TY_WSTRING;
				tv->tv_value.val_wstring = yyvsp[0].wsval;
				yyval.valval = tv;
			}
break;
case 73:
{ yyval.sval = NULL; }
break;
case 74:
{
				tmpl_delimiter_t *de = _evlTmplAllocDelimiter();
				de->de_type = TMPL_DELIM_CONST;
				de->de_delimiter = yyvsp[0].sval;
				yyval.delimval = de;
			}
break;
case 75:
{
				/* Handle like an array dimension. */
				tmpl_delimiter_t *de = _evlTmplAllocDelimiter();
				de->de_type = TMPL_DELIM_ATTNAME; /* for now */
				de->de_delimiter = NULL;
				de->u.u_attname = _evlMakeDotPathFromList(yyvsp[0].listval);
				_evlFreeList(yyvsp[0].listval, 1);
				yyval.delimval = de;
			}
break;
case 76:
{ yyval.delimval = NULL; }
break;
case 77:
{
				_evlTmplAddFormatSpec(template, yyvsp[0].sval);
				_evlTmplWrapup(template);
			}
break;
case 78:
{
				_evlTmplAddFormatSpec(template, yyvsp[0].sval);
				_evlTmplWrapup(template);
			}
break;
case 79:
{
				char *s = wcsToMbs(yyvsp[0].wsval);
				if (s) {
					_evlTmplAddFormatSpec(template, s);
				} else {
					fprintf(stderr,
"Internal error: Cannot convert wstring format specification.\n");
					template->tm_flags |= TMPL_TF_ERROR;
				}
				_evlTmplWrapup(template);
			}
break;
case 82:
{
				_evlTmplWrapup(template);
			}
break;
case 84:
{
				_evlAddRedirection(template, yyvsp[0].rdatt, NULL);
			}
break;
case 85:
{
				_evlAddRedirection(template, yyvsp[-1].rdatt, yyvsp[0].rdatt);
			}
break;
case 86:
{
				_evlAddRedirection(template, yyvsp[0].rdatt, yyvsp[-1].rdatt);
			}
break;
case 87:
{
				_evlAddRedirection(template, NULL, yyvsp[0].rdatt);
			}
break;
case 88:
{
				yyval.rdatt = _evlMkRedirAtt(template, yyvsp[0].facval, NULL,
					POSIX_LOG_ENTRY_FACILITY);
			}
break;
case 89:
{
				yyval.rdatt = _evlMkRedirAtt(template, yyvsp[0].facval, NULL,
					POSIX_LOG_ENTRY_FACILITY);
			}
break;
case 90:
{
				yyval.rdatt = _evlMkRedirAtt(template, 0, yyvsp[-1].listval,
					POSIX_LOG_ENTRY_FACILITY);
			}
break;
case 91:
{
				yyval.rdatt = _evlMkRedirAtt(template, yyvsp[0].ival, NULL,
					POSIX_LOG_ENTRY_EVENT_TYPE);
			}
break;
case 92:
{
				yyval.rdatt = _evlMkRedirAtt(template, yyvsp[0].ival, NULL,
					POSIX_LOG_ENTRY_EVENT_TYPE);
			}
break;
case 93:
{
				yyval.rdatt = _evlMkRedirAtt(template, 0, yyvsp[-1].listval,
					POSIX_LOG_ENTRY_EVENT_TYPE);
			}
break;
case 94:
{ yyval.uival = yyvsp[0].ulval; }
break;
case 95:
{ yyval.lval = yyvsp[0].ulval; }
break;
case 96:
{ yyval.lval = -yyvsp[0].ulval; }
break;
case 100:
{
				/* Concatenate adjacent string literals. */
				char *s1 = yyvsp[-1].sval;
				char *s2 = yyvsp[0].sval;
				char *snew = realloc(s1,
					strlen(s1)+strlen(s2)+1);
				assert(snew != NULL);
				(void) strcat(snew, s2);
				free(s2);
				yyval.sval = snew;
			}
break;
case 102:
{
				/* Concatenate adjacent wide string literals. */
				wchar_t *s1 = yyvsp[-1].wsval;
				wchar_t *s2 = yyvsp[0].wsval;
				size_t len1 = wcslen(s1);
				size_t len2 = wcslen(s2);
				wchar_t *snew = realloc(s1,
					(len1+len2+1) * sizeof(wchar_t));
				assert(snew != NULL);
				(void) memcpy(snew + len1, s2,
					(len2+1)*sizeof(wchar_t));
				free(s2);
				yyval.wsval = snew;
			}
break;
case 104:
{ yyval.dval = -yyvsp[0].dval; }
break;
case 107:
{
#if (CHAR_MIN == 0)
				yyval.ival = TY_UCHAR;
#else
				yyval.ival = TY_CHAR;
#endif
			}
break;
case 108:
{ yyval.ival = TY_CHAR; }
break;
case 109:
{ yyval.ival = TY_UCHAR; }
break;
case 110:
{ yyval.ival = TY_DOUBLE; }
break;
case 111:
{ yyval.ival = TY_LDOUBLE; }
break;
case 112:
{ yyval.ival = TY_INT; }
break;
case 113:
{
				yyval.ival = makeIntType(yyvsp[-1].listval);
				_evlFreeList(yyvsp[-1].listval, 0);
			}
break;
case 114:
{}
break;
case 115:
{}
break;
case 116:
{ yyval.ival = TK_TYLONG; }
break;
case 117:
{ yyval.ival = TK_TYSHORT; }
break;
case 118:
{ yyval.ival = TK_TYSIGNED; }
break;
case 119:
{ yyval.ival = TK_TYUNSIGNED; }
break;
case 120:
{
				yyval.listval = _evlMkListNode((void*)(yyvsp[0].ival));
			}
break;
case 121:
{
				yyval.listval = _evlAppendToList(yyvsp[-1].listval, (void*)(yyvsp[0].ival));
			}
break;
case 122:
{
				yyval.listval = _evlMkListNode(yyvsp[0].sval);
			}
break;
case 123:
{
				yyval.listval = _evlAppendToList(yyvsp[-2].listval, yyvsp[0].sval);
			}
break;
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
