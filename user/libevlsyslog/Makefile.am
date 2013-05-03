## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += slog_fwd libevlsyslog.conf Makefile.mk

MOSTLYCLEANFILES += 

syslog_forwarddir := $(sbindir)
syslog_forward_SCRIPTS := slog_fwd

evlsyslog_confdir := @EVLOG_CONF_DIR@
evlsyslog_conf_DATA := libevlsyslog.conf

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux
AM_CFLAGS = @CFLAGS@

# Libraries
evlsyslog_sources := libevlsyslog.c

evlsyslogdir := /lib

# LIBRARIES: static
evlsyslog_LIBRARIES = libevlsyslog.a

libevlsyslog_a_SOURCES := $(evlsyslog_sources)
libevlsyslog_a_CFLAGS := $(AM_CFLAGS)

libevlsyslog_a_LIBADD := 

# LTLIBRARIES is for shared+static libs
evlsyslog_LTLIBRARIES = libevlsyslog.la

libevlsyslog_la_SOURCES := $(evlsyslog_sources)

# Another wired automake/libtool bug
libevlsyslog_la_LDFLAGS := -Xcompiler -nostartfiles


