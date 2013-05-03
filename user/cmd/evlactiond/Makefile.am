## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

action_conf_files := action_registry action_profile 

EXTRA_DIST += $(action_conf_files) Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

sbin_PROGRAMS := evlactiond 
evlactiond_SOURCES := evlactiond.h evlactiond.c
evlactiond_LDADD := ../../lib/.libs/libevl.a

evlaction_confdir := @EVLOG_CONF_DIR@
evlaction_conf_DATA := $(action_conf_files)
