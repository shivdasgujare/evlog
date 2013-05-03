KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

DEBUG=-g
UROOT=../..
INC=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
CFLAGS=$(DEBUG) $(INC)
LIBS=$(UROOT)/lib/libevl.a -lfl -lpthread

evlsend: evlsend.o $(UROOT)/lib/libevl.a
	$(CC) evlsend.o $(LIBS) -o evlsend

clean:
	rm -f evlsend.o

clobber: clean
	rm -f evlsend
