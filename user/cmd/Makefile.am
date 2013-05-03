## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

EXTRA_DIST += Makefile.mk

MOSTLYCLEANFILES += 

# bins or src products may be required to generate stuff in etc and or
# doc. Stuff in bin, intl and lib may be required to generate stuff in
# the rest (so, this is a possible dependency order).
SUBDIRS = evlogd evlsend evlview evlconfig evlnotifyd evlactiond evlnotify \
	evlfacility evltc evlogmgr evlgentmpls ela
