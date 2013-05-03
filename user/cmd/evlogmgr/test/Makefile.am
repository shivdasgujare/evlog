## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += logmgmt1.sh

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/kernel/@KERN_VERSION@/include

logmgrtestdir := $(localstatedir)/test
logmgrtest_SCRIPTS := logmgmt1.sh


