KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

DEBUG=-g
UROOT=../..
INC=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
CFLAGS=$(DEBUG) $(INC)
DOTOS=evlview.o specfmt.o
LIBS=$(UROOT)/lib/libevl.a -lfl -lpthread

evlview: $(DOTOS) $(UROOT)/lib/libevl.a
	$(CC) $(DOTOS) $(LIBS) -o evlview

clean:
	rm -f $(DOTOS)

clobber: clean
	rm -f evlview
