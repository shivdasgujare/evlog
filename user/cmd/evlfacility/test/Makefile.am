## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += test.sed $(facility_test_files)

MOSTLYCLEANFILES += 

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/kernel/@KERN_VERSION@/include

facility_test_files := testfac1.sh \
		       testfac2.sh \
		       testfac3.sh \
		       testfac1.tpl.out \
		       testfac2.tpl.out \
		       testfac3.tpl.out 

facility_testdir := $(localstatedir)/test
facility_test_SCRIPTS := $(facility_test_files)
