KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h
MAKE=make -f Makefile.mk
ARCH=

UROOT=../..
INCLUDEDIRS=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
LIBDIRS = -L/usr/lib -L../../lib
#LIBS=$(UROOT)/lib/libevl.a /usr/lib/libc.a -lnsl -lfl 
LIBS=$(UROOT)/lib/libevl.a -lpthread
STATIC_LIBC=/usr/lib/libc.a

#DEBUG=-g -DDEBUG2
DEBUG=-g
EVL_WRITE_DIRECT=-D_EVL_WRITE_DIRECT
CFLAGS_64=$(DEBUG) $(EVL_WRITE_DIRECT) $(INCLUDEDIRS) $(LIBDIRS) -O -r
CFLAGS=$(DEBUG) $(EVL_WRITE_DIRECT) $(INCLUDEDIRS) $(LIBDIRS) -O
DOTOS=evlnotifyd.o serialize.o
PRODUCTS = evlnotifyd

all: $(PRODUCTS) evlnotifyd_test

evlnotifyd: $(DOTOS) $(UROOT)/lib/libevl.a
ifeq ($(ARCH), ia64)
	$(CC) $(CFLAGS_64) $(DOTOS) $(LIBDIRS)  $(LIBS) -o evlnotifyd.r
	ld $(STATIC_LIBC) evlnotifyd.r -o evlnotifyd
else
	$(CC) $(CFLAGS) $(DOTOS) $(LIBDIRS)  $(LIBS) -o evlnotifyd
endif

evlnotifyd_test:
	cd test; $(MAKE)
	
clean:
	-rm -f *.o *~ \#*\#
	-rm -f evlnotifyd.r
	(cd test; $(MAKE) clean)

clobber: clean
	-rm -f $(PRODUCTS) 

nothing:
	
