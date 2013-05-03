## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += testevlog1.out testevlog1.sh Makefile.mk

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/kernel/@KERN_VERSION@/include

logdtestdir := $(localstatedir)/test
logdtest_PROGRAMS := testdup testdup2 testdup3
logdtest_DATA := testevlog1.out
logdtest_SCRIPTS := testevlog1.sh

LDADD := ../../../lib/.libs/libevl.a

