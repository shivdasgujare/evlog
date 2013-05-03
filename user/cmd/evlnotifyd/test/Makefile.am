## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/kernel/@KERN_VERSION@/include

noinst_PROGRAMS := testclient test testnull
LDADD := ../../../lib/.libs/libevl.a

#
# Specify these if needed. Otherwise, let automake do all the work
#
#testclient_SOURCES := testclient.c
#testclient_LDADD := $(top_srcdir)/user/lib/.libs/libevl.a
#
#test_SOURCES := test.c
#test_LDADD := $(top_srcdir)/user/lib/.libs/libevl.a
#
#testnull_SOURCES := testnull.c
#testnull_LDADD := $(top_srcdir)/user/lib/.libs/libevl.a
