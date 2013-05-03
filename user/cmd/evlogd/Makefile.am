## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += Makefile.mk

conf_files = evlhosts evlogrmtd.conf 

EXTRA_DIST += $(conf_files) shared/rmt_common.c shared/rmt_common.h \
		tcp_rmtlog_be/Makefile.am \
		tcp_rmtlog_be/Makefile.in \
		tcp_rmtlog_be/tcp_rmtlog_be.c \
		tcp_rmtlog_be/tcp_rmtlog_be.conf \
		tcp_rmtlog_be/tcp_rmtlog_be.spec.in \
		udp_rmtlog_be/Makefile.am \
		udp_rmtlog_be/Makefile.in \
		udp_rmtlog_be/udp_rmtlog_be.c \
		udp_rmtlog_be/udp_rmtlog_be.conf \
		udp_rmtlog_be/udp_rmtlog_be.spec.in


MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

sbin_PROGRAMS := evlogd evlogrmtd

evlogd_SOURCES := evlogd.c backendmgr.c ksym.c ksym_mod.c ksyms.h callback.h evl_kernel.h
evlogd_LDADD := ../../lib/.libs/libevl.a 

if PPC64
evlogd_CFLAGS := -D_PPC_64KERN_32USER_
else
evlogd_CFLAGS := 
endif

evlogrmtd_SOURCES := evlogrmtd.c shared/rmt_common.c shared/rmt_common.h
evlogrmtd_LDADD := ../../lib/.libs/libevl.a

evlog_confdir := @EVLOG_CONF_DIR@
evlog_conf_DATA := $(conf_files)

SUBDIRS := . test

