MAINTAINERCLEANFILES    = Makefile.in

manpages = evlview.1 evlog.1 evlsend.1 evlconfig.1 evlnotify.1 evlquery.1 evlfacility.1 evltc.1 evlogmgr.1 evlremote.1 evlgentmpls.1
EXTRA_DIST = $(manpages) Makefile.mk

MOSTLYCLEANFILES = *.gz

# Can't get compressed man page to work correctly with automake
# Well, no big deal
#
# manfiles = $(manpages:.1=.1.gz)
# 
# %.1.gz: %.1
# 	gzip -c $< > $@

man1_MANS = $(manpages)

