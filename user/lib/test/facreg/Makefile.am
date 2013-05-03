## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

noinst_PROGRAMS := frtest2

LDADD := $(top_srcdir)/user/lib/.libs/libevl.a

DEPENDENCIES := $(top_srcdir)/user/lib/libevl.la

