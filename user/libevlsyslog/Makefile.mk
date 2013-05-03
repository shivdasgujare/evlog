KERN_VERSION=v2.4.4
KERN_INC=../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

DEBUG=-g 
#
# To enable posix_log_write directly writes to log daemon
#
EVL_WRITE_DIRECT=-D_EVL_WRITE_DIRECT
#EVL_WRITE_DIRECT=

DOTSOS := $(patsubst %.c,%.os,$(wildcard *.c))
LIB_SO=$(DOTSOS) -ldl -lc -lpthread
LINUX_INCDIR=../include/linux
INCDIR=../include
INC=-I$(INCDIR) -I$(LINUX_INCDIR) -I$(KERN_INC)
HEADERS=$(INCDIR)/posix_evlog.h \
		$(INCDIR)/posix_evlsup.h \
		$(INCDIR)/evlog.h \
		$(INCDIR)/evl_common.h \
		$(LINUX_INCDIR)/evl_log.h \
		$(EVL_LOG_H)
CFLAGS=$(DEBUG) $(INC)
CFLAGS_SO= -fPIC $(EVL_WRITE_DIRECT) -O $(INC)

all: libevlsyslog.so 


libevlsyslog.so: $(DOTSOS)
	rm -f libevlsyslog.so
	ld -share -soname libevlsyslog.so.1 -o libevlsyslog.so $(LIB_SO)

libevlsyslog.os: libevlsyslog.c $(HEADERS)
	$(CC) -c $(CFLAGS_SO) -o $@ libevlsyslog.c  

install:
	./ldsopreload -r /lib/libevlsyslog.so
	rm -f /lib/libevlsyslog.so.1
	rm -f /lib/libevlsyslog.so
	cp libevlsyslog.so /lib
	/sbin/ldconfig
	./ldsopreload /lib/libevlsyslog.so

uninstall:
	./ldsopreload -r /lib/libevlsyslog.so
	rm -f /lib/libevlsyslog.so.1
	rm -f /lib/libevlsyslog.so
	/sbin/ldconfig

syslog2evlog_on:
	cp libevlsyslog.so /lib
	/sbin/ldconfig
	export LD_PRELOAD=/lib/libevlsyslog.so

syslog2evlog_off:
	export LD_PRELOAD=""

clean:
	rm -f $(DOTSOS)

clobber: clean
	rm -f libevlsyslog.so
