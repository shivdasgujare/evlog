## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += Makefile.mk

MOSTLYCLEANFILES += 

testscripts := testevlwrite.sh \
		 testevlwrite2.sh \
		 aligntest.sh \
		 aostest.sh \
		 aosApiTest.sh \
		 defaultTest.sh \
		 degenerate.sh \
		 delimiter.sh \
		 stattest.sh \
		 tmplMiscTest.sh \
		 syslogatTest.sh \
                testevlsend_b.sh \
                evlsend_b.sh \
		 runtests.sh 

testtemplates :=    evl_log_write_test2.t \
		       aligntest.t \
		       aostest.t \
		       delimiter.t \
		       degenerate.t \
		       stattest.t \
		       stat.t \
		       tmplMiscTest.t 

testoutputfiles := evl_log_write.out \
	      evl_log_write.ppc.out \
	      evl_log_write.s390.out \
	      evl_log_write.s390x.out \
	      evl_log_write.ia64.out \
	      evl_log_write2.out \
	      aligntest.out \
	      aostest.out \
	      aosApiTest.out \
	      defaultTest.out \
	      defaultTest.ia64.out \
	      defaultTest.ppc.out \
	      defaultTest.s390.out \
	      defaultTest.s390x.out \
	      degenerate.out \
	      delimiter.out \
	      tmplMiscTest.out \
	      syslogatTest.out 

# .out is a default Makefile target. But we don't want automake to
# mess with our .out files. So we have to override it here.
$(testoutputfiles):
	@:

EXTRA_DIST += $(testscripts) $(testtemplates) $(testoutputfiles) defaultTest.t hr.t


# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

testsdir = $(localstatedir)/test

tests_PROGRAMS := evl_log_write_test \
		  evl_log_write_test2 \
		  aligntest \
		  aostest \
		  aosApiTest \
		  defaultTest \
		  degenerate \
		  delimiter \
		  stattest \
		  tmplMiscTest \
		  syslogatTest \
		  printfArray \
		  syslogatArray

tests_SCRIPTS := $(testscripts)

testsoutputdir = $(localstatedir)/test
testsoutput_DATA := $(testoutputfiles)

tests_templatedir = $(localstatedir)/test/templates/user
tests_template_DATA := $(testtemplates)

default_templatedir = $(localstatedir)/test/templates/local7
default_template_DATA := defaultTest.t

hr_templatedir = $(localstatedir)/test/templates/hr
hr_template_DATA := hr.t

tests_template_HEADERS := statdefs.h 

noinst_PROGRAMS :=  dumpTest \
		  fmtTest \
		  fussLog \
		  attttest \
		  seektest \
		  threadTest \
		  threadTest2 \
		  tweakLogGen \
		  printfTest 

LDADD := ../.libs/libevl.a

SUBDIRS := bitfields 

DIST_SUBDIRS := bitfields facreg
