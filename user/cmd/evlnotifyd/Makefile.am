## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux
AM_CFLAGS = @CFLAGS@ -D_EVL_WRITE_DIRECT

sbin_PROGRAMS := evlnotifyd
evlnotifyd_SOURCES := evlnotifyd.h evlnotifyd.c serialize.c
evlnotifyd_LDADD := ../../lib/.libs/libevl.a

SUBDIRS := . test

