## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += evlogmgr.cron Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

sbin_PROGRAMS := evlogmgr
evlogmgr_SOURCES := evl_logmgmt.h evlogmgr.c evl_logmgmt.c
evlogmgr_LDADD := ../../lib/.libs/libevl.a

logcrondir := @CRONDIR@
logcron_DATA := evlogmgr.cron

SUBDIRS := . test

