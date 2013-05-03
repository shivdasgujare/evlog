KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

UROOT=../..
INCLUDEDIRS = -I$(UROOT)/include -I$(KERN_INC)
LIBDIRS = -L/usr/lib -L$(UROOT)/lib
DEBUG= -g
#DEBUG= -g -DDEBUG2
LIBS = $(UROOT)/lib/libevl.a -lz -lpthread
CFLAGS =  $(INCLUDEDIRS) $(LIBDIRS) -O $(DEBUG) $(CDEST)
DOTOS=evlogmgr.o evl_logmgmt.o
PRODUCTS = evlogmgr 

all: $(PRODUCTS) 

evlogmgr: $(DOTOS) 
	$(CC) $(MFLAGS) $(CFLAGS) $(DOTOS) $(LIBDIRS) $(LIBS) -o evlogmgr

clean:
	-rm -f *.o *~ \#*\#

clobber: clean
	-rm -f $(PRODUCTS) 
	
nothing:
	
