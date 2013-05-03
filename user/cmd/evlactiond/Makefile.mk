KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

UROOT=../..
INCLUDEDIRS=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
LIBDIRS = -L/usr/lib -L../../lib
LIBS=$(UROOT)/lib/libevl.a -lpthread

DEBUG=
CFLAGS=$(DEBUG) $(INCLUDEDIRS) $(LIBDIRS) -O
DOTOS=evlactiond.o 
PRODUCTS = evlactiond

all: $(PRODUCTS)

evlactiond: $(DOTOS) $(UROOT)/lib/libevl.a
	$(CC) $(CFLAGS) $(DOTOS) $(LIBDIRS)  $(LIBS) -o evlactiond


clean:
	-rm -f *.o *~ \#*\#

clobber: clean
	-rm -f $(PRODUCTS) 

nothing:
	
