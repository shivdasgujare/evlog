KERN_VERSION=v2.4.4
KERN_INC=../../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

UROOT=../../..
INCLUDEDIRS=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
LIBDIRS = -L/usr/lib -L$(UROOT)/lib
DEBUG=-g
LIBS=$(UROOT)/lib/libevl.a -lpthread
CFLAGS=-DDEBUG1  $(INCLUDEDIRS) $(LIBDIRS) -O $(DEBUG)
PRODUCTS = testclient test  testnull

all: $(PRODUCTS)

testclient: testclient.c 
	$(CC) $(CFLAGS) $(LIBDIRS) -o testclient testclient.c $(LIBS)


test: test.c 
	$(CC) $(CFLAGS) $(LIBDIRS) -o test test.c $(LIBS)
	
testnull: testnull.c 
	$(CC) $(CFLAGS) $(LIBDIRS) -o testnull testnull.c $(LIBS)

			
clean:
	-rm -f *.o *~ \#*\#
	-rm -f $(PRODUCTS)

clobber: clean

nothing:
	
