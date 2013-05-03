## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += query.y lex Makefile.mk

MOSTLYCLEANFILES += lex.qq.c q.tab.h q.tab.c 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

lex.qq.c: lex
	flex -Pqq $(top_srcdir)/user/lib/query/lex

q.tab.h: query.y
	yacc -dl -b q -p qq $(top_srcdir)/user/lib/query/query.y

q.tab.c: q.tab.h

evlquery_sources := evl_parse.h q.tab.c lex.qq.c normalize.c evaluate.c qopt.c 

# worry about this later
# nodist_evlquery_sources := q.tab.c lex.qq.c

## LIBRARIES: static
#noinst_LIBRARIES = libevlquery.a
#
#libevlquery_a_SOURCES := $(evlquery_sources)
#libevlquery_a_CFLAGS := $(AM_CFLAGS)

# LTLIBRARIES is for shared+static libs
noinst_LTLIBRARIES = libevlquery.la

libevlquery_la_SOURCES := $(evlquery_sources)

libevlquery_la_LDFLAGS :=

libevlquery_la_LIBADD :=


