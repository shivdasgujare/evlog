KERN_VERSION=v2.4.20
KERN_INC=../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

MAKE=make -f Makefile.mk
DEBUG=-g 
# 
# To enable posix_log_write directly writes to log daemon
#
EVL_WRITE_DIRECT=-D_EVL_WRITE_DIRECT
#EVL_WRITE_DIRECT=
DOTOS := $(patsubst %.c, %.o,$(wildcard *.c))
QDOTOS=query/q.tab.o query/lex.qq.o query/normalize.o query/evaluate.o \
	query/qopt.o
TDOTOS=template/bvfmt.o template/lex.tt.o template/serial.o \
	template/template.o template/tmplfmt.o template/tmplmgmt.o \
	template/tt.tab.o template/convert.o
UDOTOS := $(patsubst %.c, %.o,$(wildcard util/*.c))
LIBDOTOS=$(DOTOS) $(QDOTOS) $(TDOTOS) $(UDOTOS)

DOTSOS := $(patsubst %.c,%.os,$(wildcard *.c))
QDOTSOS=query/q.tab.os query/lex.qq.os query/normalize.os query/evaluate.os \
	query/qopt.os
TDOTSOS=template/bvfmt.os template/lex.tt.os template/serial.os \
	template/template.os template/tmplfmt.os template/tmplmgmt.os \
	template/tt.tab.os template/convert.os
UDOTSOS := $(patsubst %.c, %.os,$(wildcard util/*.c))
LIB_SO=$(DOTSOS) $(QDOTSOS) $(TDOTSOS) $(UDOTSOS)
LINUX_INCDIR=../include/linux
INCDIR=../include
INC=-I$(INCDIR) -I$(LINUX_INCDIR) -I$(KERN_INC)
HEADERS=$(INCDIR)/posix_evlog.h \
		$(INCDIR)/posix_evlsup.h \
		$(INCDIR)/evlog.h \
		$(INCDIR)/evl_common.h \
		$(LINUX_INCDIR)/evl_log.h \
		$(EVL_LOG_H)
CFLAGS=$(DEBUG) $(EVL_WRITE_DIRECT) $(INC)
CFLAGS_SO=-fPIC -O -D_SHARED_LIB $(EVL_WRITE_DIRECT) $(INC) 

all: libevl.a libevl.so lib_test

libevl.a: $(DOTOS) queryDir templateDir utilDir  
	rm -f libevl.a
	ar qc libevl.a $(LIBDOTOS)

libevl.so: $(DOTSOS) query/made template/made util/made
	rm -f libevl.so
	ld -share -soname libevl.so.1 -o libevl.so $(LIB_SO) -lc

queryDir:
	(cd query; $(MAKE) KERN_VERSION=$(KERN_VERSION))

templateDir:
	(cd template; $(MAKE) KERN_VERSION=$(KERN_VERSION))

utilDir:
	(cd util; $(MAKE) KERN_VERSION=$(KERN_VERSION))

lib_test: libevl.a libevl.so
	(cd test; $(MAKE) KERN_VERSION=$(KERN_VERSION))

posix1.o: $(HEADERS)

posix1.os: posix1.c $(HEADERS)
	$(CC) -c $(CFLAGS_SO) $< -o $@ 

posix2.o: $(HEADERS)

posix2.os: posix2.c $(HEADERS)
	$(CC) -c $(CFLAGS_SO) $< -o $@ 
	
posix_evlsup.o: $(HEADERS)

posix_evlsup.os: posix_evlsup.c $(HEADERS)
	$(CC) -c $(CFLAGS_SO) $< -o $@ 

formatrec.o: $(HEADERS)

formatrec.os: formatrec.c $(HEADERS)
	$(CC) -c $(CFLAGS_SO) $< -o $@ 

facreg.o: $(HEADERS)

facreg.os: facreg.c $(HEADERS)
	$(CC) -c $(CFLAGS_SO) $< -o $@ 

evl_log_write.o: $(HEADERS)

evl_log_write.os: evl_log_write.c $(HEADERS)
	$(CC) -c $(CFLAGS_SO) $< -o $@ 

syslogat.o: $(HEADERS)

syslogat.os: syslogat.c $(HEADERS)
	$(CC) -c $(CFLAGS_SO) $< -o $@ 

clean:
	rm -f $(DOTOS) $(DOTSOS)
	(cd query; $(MAKE) clean)
	(cd template; $(MAKE) clean)
	(cd util; $(MAKE) clean)
	(cd test; $(MAKE) clean)

clobber: clean
	rm -f libevl.a libevl.so
	(cd query; $(MAKE) clobber)
	(cd template; $(MAKE) clobber)
	(cd util; $(MAKE) clobber)
	(cd test; $(MAKE) clobber)
