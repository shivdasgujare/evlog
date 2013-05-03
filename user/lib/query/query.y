%{
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

%}

%union {
	unsigned long	ival;
	/* double	dval; */
	char	*sval;
	pnode_t	*nval;
}

%token <sval> NAME STRING
%token <ival> OP INTEGER GE LE NE OR AND CONTAINS RENOMATCH
/* These next two don't show up in the grammar, but are used in qopt.c. */
%token <ival> AND_LATER OR_LATER
%type <nval> name scoped_name val
%type <nval> exp un_exp and_exp filter
%token <ival> ERRTOK
%%

	filter : and_exp
			{_evlQParseTree = $1;}
		| filter OR and_exp
			{ pnode_t *tnode;

			  tnode = (pnode_t *) malloc(sizeof(pnode_t));
			  tnode->node_type = nt_op;
			  tnode->attr_flag = OR;
			  tnode->u_att.op_node.left = $1; 
			  tnode->u_att.op_node.right = $3; 
			  _evlQParseTree = $$ = tnode; 
			}
		;

	and_exp : un_exp
		| and_exp AND un_exp
			{ pnode_t *tnode;

			  tnode = (pnode_t *) malloc(sizeof(pnode_t));
			  tnode->node_type = nt_op;
			  tnode->attr_flag = AND;
			  tnode->u_att.op_node.left = $1; 
			  tnode->u_att.op_node.right = $3; 
			  $$ = tnode; 
			}
		;

	un_exp	: exp
		| '!' un_exp
			{ pnode_t *tnode;

			  tnode = (pnode_t *) malloc(sizeof(pnode_t));
			  tnode->node_type = nt_op;
			  tnode->attr_flag = '!';	
			  tnode->u_att.op_node.left = $2;
			  tnode->u_att.op_node.right = NULL;
			  $$ = tnode;
			}
		;

	exp	: scoped_name OP val
			{ pnode_t *tnode;

			  tnode = (pnode_t *) malloc(sizeof(pnode_t));
			  tnode->node_type = nt_op;
			  tnode->attr_flag = $2;	
			  tnode->u_att.op_node.left = $1;
			  tnode->u_att.op_node.right = $3;
			  $$ = tnode;
			}
		| name
		| '(' filter ')'
			{
			  $$ = $2;
			}
		;

	name	: NAME
                        { pnode_t *tnode;

                          tnode = (pnode_t *) malloc(sizeof(pnode_t));
                          tnode->node_type = nt_name;
                          tnode->attr_flag = -1;
                          tnode->u_att.attr_name = $1;
                          $$ = tnode;
                        }

	scoped_name	: name
		| scoped_name '.' NAME
			{ /*
			   * Concatenate "scoped_name" "." "NAME" back into
			   * "scoped_name.NAME".
			   */
			  pnode_t *tnode = $1;
			  char *s1 = tnode->u_att.attr_name;
			  char *s2 = $3;
			  char *snew = realloc(s1, strlen(s1)+1+strlen(s2)+1);

			  assert(snew != NULL);
			  (void) strcat(snew, ".");
			  (void) strcat(snew, s2);
			  free(s2);
			  tnode->u_att.attr_name = snew;
			  $$ = tnode;
			}
		;

	val	: name
		| INTEGER
			{ pnode_t *node = (pnode_t *) malloc(sizeof(pnode_t));
			  node->node_type = nt_val;
			  node->attr_flag = 1;
			  node->u_att.attr_uval = $1;
			  $$ = node;
			}
		| '-' INTEGER
			{ pnode_t *node = (pnode_t *) malloc(sizeof(pnode_t));
			  node->node_type = nt_val;
			  node->attr_flag = -1;
			  node->u_att.attr_val = -((long) $2);
			  $$ = node;
			}
		| STRING
			{ pnode_t *node = (pnode_t *) malloc(sizeof(pnode_t));
			  node->node_type = nt_string;
			  node->attr_flag = 0;
			  node->u_att.attr_string = $1;
			  $$ = node;
			}
		;

%%

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
