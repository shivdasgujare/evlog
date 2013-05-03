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
#define yyparse qqparse
#define yylex qqlex
#define yyerror qqerror
#define yychar qqchar
#define yyval qqval
#define yylval qqlval
#define yydebug qqdebug
#define yynerrs qqnerrs
#define yyerrflag qqerrflag
#define yyss qqss
#define yyssp qqssp
#define yyvs qqvs
#define yyvsp qqvsp
#define yylhs qqlhs
#define yylen qqlen
#define yydefred qqdefred
#define yydgoto qqdgoto
#define yysindex qqsindex
#define yyrindex qqrindex
#define yygindex qqgindex
#define yytable qqtable
#define yycheck qqcheck
#define yyname qqname
#define yyrule qqrule
#define yysslim qqsslim
#define yystacksize qqstacksize
#define YYPREFIX "qq"
/*
 * Copyright (C) 2001 IBM
 */

#ident "$Header: /cvsroot/evlog/evlog/user/lib/query/query.y,v 1.8 2004/08/03 23:01:54 nguyhien Exp $"

/* 
 * query.y, the POSIX query expression parser grammar
 * Based on ees.y, the EES filter expression parser grammar
 */

#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <regex.h>
#include <stdlib.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_parse.h"

void yyerror(const char *);
int lexerr;

pnode_t *_evlQParseTree = NULL;

typedef union {
	unsigned long	ival;
	/* double	dval; */
	char	*sval;
	pnode_t	*nval;
} YYSTYPE;
#define YYERRCODE 256
#define NAME 257
#define STRING 258
#define OP 259
#define INTEGER 260
#define GE 261
#define LE 262
#define NE 263
#define OR 264
#define AND 265
#define CONTAINS 266
#define RENOMATCH 267
#define AND_LATER 268
#define OR_LATER 269
#define ERRTOK 270
const short qqlhs[] = {                                        -1,
    0,    0,    6,    6,    5,    5,    4,    4,    4,    1,
    2,    2,    3,    3,    3,    3,
};
const short qqlen[] = {                                         2,
    1,    3,    1,    3,    1,    2,    3,    1,    3,    1,
    1,    3,    1,    1,    2,    1,
};
const short qqdefred[] = {                                      0,
   10,    0,    0,    0,    0,    0,    5,    3,    0,    6,
    0,    0,    0,    0,    0,    9,    0,   16,   14,    0,
   13,    7,   12,    4,   15,
};
const short qqdgoto[] = {                                       4,
    5,    6,   22,    7,    8,    9,
};
const short qqsindex[] = {                                    -33,
    0,  -33,  -33, -258,    0,  -38,    0,    0, -255,    0,
  -39,  -33,  -40, -246,  -33,    0, -255,    0,    0, -248,
    0,    0,    0,    0,    0,
};
const short qqrindex[] = {                                      0,
    0,    0,    0,    0,    1,    0,    0,    0,    3,    0,
    0,    0,    0,    0,    0,    0,    4,    0,    0,    0,
    0,    0,    0,    0,    0,
};
const short qqgindex[] = {                                     10,
    2,    0,    0,    0,    7,    5,
};
#define YYTABLESIZE 268
const short qqtable[] = {                                       2,
    8,   16,    1,    2,   20,   12,    3,   14,   10,   15,
   23,   25,   11,    0,   21,    0,   17,    0,    0,    0,
    0,   24,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    8,    0,    1,    2,    0,   11,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    1,   18,    0,   19,
   13,    0,    0,    1,   12,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   11,
    0,    0,    0,    0,    8,    8,    1,    2,
};
const short qqcheck[] = {                                      33,
    0,   41,    0,    0,   45,  264,   40,   46,    2,  265,
  257,  260,    3,   -1,   13,   -1,   12,   -1,   -1,   -1,
   -1,   15,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   41,   -1,   41,   41,   -1,   46,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  257,  258,   -1,  260,
  259,   -1,   -1,  257,  264,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  259,
   -1,   -1,   -1,   -1,  264,  265,  264,  264,
};
#define YYFINAL 4
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 270
#if YYDEBUG
const char * const qqname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'!'",0,0,0,0,0,0,"'('","')'",0,0,0,"'-'","'.'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"NAME",
"STRING","OP","INTEGER","GE","LE","NE","OR","AND","CONTAINS","RENOMATCH",
"AND_LATER","OR_LATER","ERRTOK",
};
const char * const qqrule[] = {
"$accept : filter",
"filter : and_exp",
"filter : filter OR and_exp",
"and_exp : un_exp",
"and_exp : and_exp AND un_exp",
"un_exp : exp",
"un_exp : '!' un_exp",
"exp : scoped_name OP val",
"exp : name",
"exp : '(' filter ')'",
"name : NAME",
"scoped_name : name",
"scoped_name : scoped_name '.' NAME",
"val : name",
"val : INTEGER",
"val : '-' INTEGER",
"val : STRING",
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
_evlQPrName (int attrCode)
{
	char attrName[100];
	_evlGetNameByValue(_evlAttributes, attrCode, attrName, sizeof(attrName),
		"unknownAttribute");
	(void) printf("%s ", attrName);
}

static void
indent(int level)
{
	int i;
	printf("\n");
	for (i = 1; i <= level; i++) {
		printf(". ");
	}
}

void
_evlQPrOp (pnode_t *this_node)
{
	int optr = this_node->attr_flag;

	if (optr==AND) (void) printf("&& ");
	else if (optr==OR) (void) printf("|| ");
	else if (optr==GE) (void) printf(">= ");
	else if (optr==NE) (void) printf("!= ");
	else if (optr==LE) (void) printf("<= ");
	else if (optr==CONTAINS) (void) printf("contains ");
	else if (optr==RENOMATCH) (void) printf("!~ ");
	else if (optr==AND_LATER) (void) printf("AND_LATER ");
	else if (optr==OR_LATER) (void) printf("OR_LATER ");
	else (void) printf("%c ", optr);
}

void
prTree (pnode_t *this_node, int level)
{
	if (this_node == NULL) return;
	switch (this_node->node_type)
	{
		case nt_name:
		case nt_attname:
		case nt_nsaname:
			(void) printf("%s ", this_node->u_att.attr_name);
			break;
		case nt_val:
			(void) printf("%ld ", this_node->u_att.attr_val);
			break;
		case nt_string:
			(void) printf("\"%s\" ", this_node->u_att.attr_string);
			break;
		case nt_regex:
			(void) printf("<regex>");
			break;
		case nt_op:
			switch (this_node->attr_flag) {
			case AND:
			case AND_LATER:
			case OR:
			case OR_LATER:
				_evlQPrOp(this_node);
				level++;
				indent(level);
				prTree(this_node->u_att.op_node.left, level);
				indent(level);
				prTree(this_node->u_att.op_node.right, level);
				break;
			default:
				_evlQPrOp(this_node);
				prTree(this_node->u_att.op_node.left, level);
				prTree(this_node->u_att.op_node.right, level);
				break;
			}
			break;
		default	: (void) printf("_evlQPrTree: unknown node type %d\n", this_node->node_type);
	}
}

void
_evlQPrTree (pnode_t *this_node)
{
	prTree(this_node, 0);
	printf("\n");
}

void
_evlQFreeTree (pnode_t *this_node)
{
	if (this_node == NULL) return;
	switch (this_node->node_type)
	{
		case nt_name:
		case nt_attname:
		case nt_nsaname:
			free(this_node->u_att.attr_name);
			free(this_node);
			break;
		case nt_string:
			free(this_node->u_att.attr_string);
			free(this_node);
			break;
		case nt_val:
			free(this_node);
			break;
		case nt_regex:
			regfree(this_node->u_att.attr_regex);
			free(this_node);
			break;
		case nt_op:
			_evlQFreeTree(this_node->u_att.op_node.left);
			_evlQFreeTree(this_node->u_att.op_node.right);
			free(this_node);
			break;
		default	: (void) printf("_evlQFreeTree: unknown node type %d\n", this_node->node_type);
	}
}

/*
 * Build a tree that is a duplicate of the tree rooted at orig, and
 * return a pointer to it.
 */
pnode_t *
_evlQCloneTree(const pnode_t *orig)
{
	pnode_t *clone;

	if (orig == NULL) {
		return NULL;
	}
	
	clone = (pnode_t*) malloc(sizeof(pnode_t));
	assert(clone != NULL);
	memcpy(clone, orig, sizeof(pnode_t));
	if (orig->node_type == nt_op) {
		pnLeft(clone) = _evlQCloneTree(pnLeft(orig));
		pnRight(clone) = _evlQCloneTree(pnRight(orig));
	}
	return clone;
}

void
yyerror (const char *s)
{
	(void) strcpy(_evlQueryErrmsg, s);
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
case 1:
{_evlQParseTree = yyvsp[0].nval;}
break;
case 2:
{ pnode_t *tnode;

			  tnode = (pnode_t *) malloc(sizeof(pnode_t));
			  tnode->node_type = nt_op;
			  tnode->attr_flag = OR;
			  tnode->u_att.op_node.left = yyvsp[-2].nval; 
			  tnode->u_att.op_node.right = yyvsp[0].nval; 
			  _evlQParseTree = yyval.nval = tnode; 
			}
break;
case 4:
{ pnode_t *tnode;

			  tnode = (pnode_t *) malloc(sizeof(pnode_t));
			  tnode->node_type = nt_op;
			  tnode->attr_flag = AND;
			  tnode->u_att.op_node.left = yyvsp[-2].nval; 
			  tnode->u_att.op_node.right = yyvsp[0].nval; 
			  yyval.nval = tnode; 
			}
break;
case 6:
{ pnode_t *tnode;

			  tnode = (pnode_t *) malloc(sizeof(pnode_t));
			  tnode->node_type = nt_op;
			  tnode->attr_flag = '!';	
			  tnode->u_att.op_node.left = yyvsp[0].nval;
			  tnode->u_att.op_node.right = NULL;
			  yyval.nval = tnode;
			}
break;
case 7:
{ pnode_t *tnode;

			  tnode = (pnode_t *) malloc(sizeof(pnode_t));
			  tnode->node_type = nt_op;
			  tnode->attr_flag = yyvsp[-1].ival;	
			  tnode->u_att.op_node.left = yyvsp[-2].nval;
			  tnode->u_att.op_node.right = yyvsp[0].nval;
			  yyval.nval = tnode;
			}
break;
case 9:
{
			  yyval.nval = yyvsp[-1].nval;
			}
break;
case 10:
{ pnode_t *tnode;

                          tnode = (pnode_t *) malloc(sizeof(pnode_t));
                          tnode->node_type = nt_name;
                          tnode->attr_flag = -1;
                          tnode->u_att.attr_name = yyvsp[0].sval;
                          yyval.nval = tnode;
                        }
break;
case 12:
{ /*
			   * Concatenate "scoped_name" "." "NAME" back into
			   * "scoped_name.NAME".
			   */
			  pnode_t *tnode = yyvsp[-2].nval;
			  char *s1 = tnode->u_att.attr_name;
			  char *s2 = yyvsp[0].sval;
			  char *snew = realloc(s1, strlen(s1)+1+strlen(s2)+1);

			  assert(snew != NULL);
			  (void) strcat(snew, ".");
			  (void) strcat(snew, s2);
			  free(s2);
			  tnode->u_att.attr_name = snew;
			  yyval.nval = tnode;
			}
break;
case 14:
{ pnode_t *node = (pnode_t *) malloc(sizeof(pnode_t));
			  node->node_type = nt_val;
			  node->attr_flag = 1;
			  node->u_att.attr_uval = yyvsp[0].ival;
			  yyval.nval = node;
			}
break;
case 15:
{ pnode_t *node = (pnode_t *) malloc(sizeof(pnode_t));
			  node->node_type = nt_val;
			  node->attr_flag = -1;
			  node->u_att.attr_val = -((long) yyvsp[0].ival);
			  yyval.nval = node;
			}
break;
case 16:
{ pnode_t *node = (pnode_t *) malloc(sizeof(pnode_t));
			  node->node_type = nt_string;
			  node->attr_flag = 0;
			  node->u_att.attr_string = yyvsp[0].sval;
			  yyval.nval = node;
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
