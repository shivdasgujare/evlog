## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

sbin_PROGRAMS := evlfacility 
evlfacility_SOURCES := evlfacility.c
evlfacility_LDADD := ../../lib/.libs/libevl.a

SUBDIRS := . test
