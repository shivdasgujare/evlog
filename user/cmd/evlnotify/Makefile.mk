KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

UROOT=../..
INCLUDEDIRS=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
LIBDIRS = -L/usr/lib -L../../lib
LIBS=$(UROOT)/lib/libevl.a -lpthread

DEBUG=-g -DDEBUG1
CFLAGS=$(DEBUG) $(INCLUDEDIRS) $(LIBDIRS) -O
DOTOS=evlnotify.o
PRODUCTS = evlnotify

all: $(PRODUCTS)

evlnotify: $(DOTOS) $(UROOT)/lib/libevl.a
	$(CC) $(CFLAGS) $(DOTOS) $(LIBDIRS)  $(LIBS) -o evlnotify

clean:
	-rm -f *.o *~ \#*\#

clobber: clean
	-rm -f $(PRODUCTS)

nothing:
