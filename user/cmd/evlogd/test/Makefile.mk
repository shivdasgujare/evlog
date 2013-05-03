KERN_VERSION=v2.4.4
KERN_INC=../../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

UROOT=../../..
INCLUDEDIRS = -I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC) -I/usr/include
LIBDIRS = -L/usr/lib -L$(UROOT)/lib
DEBUG= -g -DDEBUG1
LIBS = $(UROOT)/lib/libevl.a -lpthread
CFLAGS =  $(INCLUDEDIRS) $(LIBDIRS) -O $(DEBUG) $(CDEST)

PRODUCTS = testdup testdup2 testdup3

all: $(PRODUCTS)

testdup: testdup.c 
	$(CC) $(MFLAGS) $(CFLAGS) $(LIBDIRS) -o testdup testdup.c $(LIBS)

testdup2: testdup2.c 
	$(CC) $(MFLAGS) $(CFLAGS) $(LIBDIRS) -o testdup2 testdup2.c $(LIBS)

testdup3: testdup3.c 
	$(CC) $(MFLAGS) $(CFLAGS) $(LIBDIRS) -o testdup3 testdup3.c $(LIBS)

clean:
	-rm -f *.o *~ \#*\#

clobber: clean
	-rm -f $(PRODUCTS) 

nothing:
	
