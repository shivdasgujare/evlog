## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += evlog.conf Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux
AM_CFLAGS = @CFLAGS@

sbin_PROGRAMS := evlconfig 
evlconfig_SOURCES := evlconfig.c
evlconfig_LDADD := ../../lib/.libs/libevl.a
evlconfig_CFLAGS := $(AM_CFLAGS)

evlog_confdir := @EVLOG_CONF_DIR@
evlog_conf_DATA := evlog.conf

#  
# noinst_LTLIBRARIES = libevlconfig.la
# libevlconfig_la_SOURCES := libevlconfig.h libevlconfig.c evlconfig.c
# libevlconfig_la_LIBADD := $(top_srcdir)/user/lib/libevl.la
# libevlconfig_la_CFLAGS := -DLIBEVLCONFIG
# 
# testlib_SOURCES := testlib.c
# testlib_LDADD := $(top_srcdir)/user/cmd/evlconfig/libevlconfig.la
# 
