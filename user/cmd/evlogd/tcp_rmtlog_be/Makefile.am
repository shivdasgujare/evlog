## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES +=
MAINTAINERCLEANFILES += tcp_rmtlog_be.spec
conf_file = tcp_rmtlog_be.conf
EXTRA_DIST += tcp_rmtlog_be.spec $(conf_file) Makefile.mk

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/cmd/evlogd -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux


tcp_rmtlog_be_sources := tcp_rmtlog_be.c ../shared/rmt_common.c

# LTLIBRARIES is for shared+static libs
plugindir = @plugindir@
plugin_LTLIBRARIES = tcp_rmtlog_be.la

tcp_rmtlog_be_la_SOURCES := $(tcp_rmtlog_be_sources)
tcp_rmtlog_be_la_LDFLAGS := -module -Wc,-nostartfiles -avoid-version

tcp_rmtlog_be_la_confdir := @pluginconfdir@
tcp_rmtlog_be_la_conf_DATA := $(conf_file)

#rpm: dist
#	@RPM_BUILD@ -ta @THE_TAR_FILE@
