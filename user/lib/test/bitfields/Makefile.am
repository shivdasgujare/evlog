## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += bftest.t bftestS.t bftestU.t

EXTRA_DIST += mktest.sh rules bftest.cpre bftest.cpost bftest.tpre bftest.tpost\
		mkc.awk mkt.awk bftest.sh bftestSN.sh bftest.t bftestS.t \
		bftest.out bftestSN.out Makefile.mk

MOSTLYCLEANFILES += bftest.c bftestN.c bftestU.c bftestUN.c bftestS.c bftestSN.c

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

bitfields_testdir := $(localstatedir)/test
bitfields_test_PROGRAMS := bftest bftestSN
bitfields_test_SCRIPTS := bftest.sh bftestSN.sh 

bitfields_outputdir := $(localstatedir)/test
bitfields_output_DATA := bftest.out bftestSN.out

# .out is a default Makefile target. But we don't want automake to
# mess with our .out files. So we have to override it here.
bftest.out bftestSN.out:
	@:

bitfields_test_templatesdir := $(localstatedir)/test/templates/user
bitfields_test_templates_DATA := bftest.t bftestS.t

noinst_PROGRAMS := bftestN bftestU bftestUN bftestS 

LDADD := ../../libevl.la

# FIXME: This should be done at configure time.
# These rules also create the corresponding .t files (bftest.t or bftestU.t).
bftest.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh

bftestN.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -n

bftestU.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -u

bftestUN.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -u -n

bftestS.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -s

bftestSN.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -s -n

