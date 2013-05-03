## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += $(notify_test_files)

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/kernel/@KERN_VERSION@/include

notify_test_files := runacttests.sh \
		     test1.sh \
		     test2.sh \
		     test3.sh \
		     test4.sh \
		     test5.sh \
		     test6.sh \
		     test7.sh \
		     actions7.nfy \
		     rmactions.sh \
		     actions.nfy \
		     loadactions.sh 

notify_testdir := $(localstatedir)/test
notify_test_SCRIPTS := $(notify_test_files)
