## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES +=
MAINTAINERCLEANFILES +=

EXTRA_DIST += $(ela_test_files)

MOSTLYCLEANFILES +=

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux 

ela_test_files := ela.h \
		  log_evt_test1.sh \
		  elatest1.t \
		  elatest2.t \
		  ela_test1.sh \
		  ela_test2.sh \
		  run_ela_tests.sh 

ela_testdir := $(localstatedir)/test
ela_test_SCRIPTS := $(ela_test_files)
