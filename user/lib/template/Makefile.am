## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += tmpllex.l tmplgram.y Makefile.mk

MOSTLYCLEANFILES += lex.tt.c tt.tab.h tt.tab.c 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

lex.tt.c: tmpllex.l
	flex -Ptt $(top_srcdir)/user/lib/template/tmpllex.l

tt.tab.c: tt.tab.h

tt.tab.h: tmplgram.y
	yacc -dl -b tt -p tt $(top_srcdir)/user/lib/template/tmplgram.y

evltemplate_sources := tt.tab.c tt.tab.h lex.tt.c serial.c serialio.inc bvfmt.c template.c tmplfmt.c tmplmgmt.c convert.c 

# worry about this later
# nodist_evltemplate_sources := lex.tt.c tt.tab.c tt.tab.h

## LIBRARIES: static
#noinst_LIBRARIES = libevltemplate.a
#
#libevltemplate_a_SOURCES := $(evltemplate_sources)
#libevltemplate_a_CFLAGS := $(AM_CFLAGS)

# LTLIBRARIES is for shared+static libs
noinst_LTLIBRARIES = libevltemplate.la

libevltemplate_la_SOURCES := $(evltemplate_sources)

libevltemplate_la_LDFLAGS :=

libevltemplate_la_LIBADD :=


