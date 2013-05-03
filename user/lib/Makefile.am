## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES += 
MAINTAINERCLEANFILES += 

MOSTLYCLEANFILES += 

LIBEVLEXTRAFILES = facility_registry

EXTRA_DIST += $(LIBEVLEXTRAFILES) Makefile.mk

# General compilation flags
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux
AM_CFLAGS = @CFLAGS@ 

# Libraries

evl_sources := posix1.c posix2.c posix_evlsup.c formatrec.c facreg.c \
	evl_log_write.c syslogat.c

# LIBRARIES: static
lib_LIBRARIES = libevl.a

libevl_a_SOURCES := $(evl_sources)
libevl_a_CFLAGS := $(AM_CFLAGS) 

libevl_a_LIBADD := query/libevlquery.la \
		   template/libevltemplate.la \
		   util/libevlutil.la

# LTLIBRARIES is for shared+static libs
lib_LTLIBRARIES = libevl.la

libevl_la_SOURCES := $(evl_sources)
libevl_la_LIBADD := query/libevlquery.la \
		    template/libevltemplate.la \
		    util/libevlutil.la

libevl_la_LDFLAGS := 

# Extra files
libevlextradir = @EVLOG_CONF_DIR@
libevlextra_DATA = $(LIBEVLEXTRAFILES)

# bins or src products may be required to generate stuff in etc and or
# doc. Stuff in bin, intl and lib may be required to generate stuff in
# the rest (so, this is a possible dependency order).
SUBDIRS = query template util . test


