## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 
MOSTLYCLEANFILES += 

EXTRA_DIST += Makefile.mk

TEMPLATFILES = templates/Makefile \
	templates/Makefile.mk \
	templates/logmgmt/Makefile \
	templates/logmgmt/logmgmt.t \
	templates/kern/kern.t 

templatedir = $(localstatedir)
nobase_template_DATA = $(TEMPLATFILES)

EXTRA_DIST += $(TEMPLATFILES)

# bins or src products may be required to generate stuff in etc and or
# doc. Stuff in bin, intl and lib may be required to generate stuff in
# the rest (so, this is a possible dependency order).
SUBDIRS = include lib libevlsyslog cmd sysfiles man 

