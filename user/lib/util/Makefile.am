## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

evlutil_sources := scanner.c evl_list.c format.c fmtbuf.c evl_common.c

# worry about this later
# nodist_evlutil_sources := q.tab.c lex.qq.c

## LIBRARIES: static
#noinst_LIBRARIES = libevlutil.a
#
#libevlutil_a_SOURCES := $(evlutil_sources)
#libevlutil_a_CFLAGS := $(AM_CFLAGS)

# LTLIBRARIES is for shared+static libs
noinst_LTLIBRARIES = libevlutil.la

libevlutil_la_SOURCES := $(evlutil_sources)

libevlutil_la_LDFLAGS :=

libevlutil_la_LIBADD :=


