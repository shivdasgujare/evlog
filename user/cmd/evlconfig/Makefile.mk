KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

UROOT=../..
INCLUDEDIRS=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
LIBDIRS = -L/usr/lib -L../../lib
LIBS=$(UROOT)/lib/libevl.a -lpthread

DEBUG=
EVL_WRITE_DIRECT=-D_EVL_WRITE_DIRECT
CFLAGS=$(DEBUG) $(EVL_WRITE_DIRECT) $(INCLUDEDIRS) $(LIBDIRS) -O
DOTOS=evlconfig.o
PRODUCTS = evlconfig

all: $(PRODUCTS)

evlconfig: $(DOTOS) $(UROOT)/lib/libevl.a
	$(CC) $(CFLAGS) $(DOTOS) $(LIBDIRS)  $(LIBS) -o evlconfig

clean:
	-rm -f *.o *~ \#*\#

clobber: clean
	-rm -f $(PRODUCTS)

nothing:
