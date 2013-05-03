KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

UROOT=../..
INCLUDEDIRS = -I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
LIBDIRS = -L/usr/lib -L$(UROOT)/lib
MAKE=make -f Makefile.mk
#DEBUG= -g -DDEBUG2
DEBUG= -g

ifeq ($(PPC_K64_U32), yes)
CFLAGS =  $(INCLUDEDIRS) $(LIBDIRS) -O $(DEBUG) $(EVL_WRITE_DIRECT) $(CDEST) -D_PPC_64KERN_32USER_
else
CFLAGS =  $(INCLUDEDIRS) $(LIBDIRS) -O $(DEBUG) $(EVL_WRITE_DIRECT) $(CDEST)
endif
LIBS = $(UROOT)/lib/libevl.a -ldl -lpthread
DOTOS = evlogd.o backendmgr.o ksym.o ksym_mod.o
PRODUCTS = evlogd evlogrmtd

all: $(PRODUCTS) evlogd/test

evlogd: $(DOTOS)
	$(CC) $(CFLAGS) $(DOTOS) $(LIBDIRS) $(LIBS) -o evlogd

evlogrmtd: evlogrmtd.o shared/rmt_common.o
	$(CC) $(CFLAGS) evlogrmtd.o shared/rmt_common.o $(LIBDIRS) $(LIBS) -o evlogrmtd

#evlogrmtd: evlogrmtd.o 
#	$(MAKE) -f evlogrmtd.mk	

evlogd/test:
	cd test; $(MAKE) KERN_VERSION=$(KERN_VERSION)


clean:
	-rm -f *.o *~ \#*\#
	-rm -f shared/*.o shared/*~
	(cd test; $(MAKE) clean)

clobber: clean
	-rm -f $(PRODUCTS)  
	(cd test; $(MAKE) clobber)
	
nothing:
	
