KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include

DEBUG=-g
UROOT=../..
INC=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
CFLAGS=$(DEBUG) $(INC)
DOTOS=evltc.o funcs.o
LIBS=$(UROOT)/lib/libevl.a -lfl -lpthread

evltc: $(DOTOS) $(UROOT)/lib/libevl.a
	$(CC) $(DOTOS) $(LIBS) -o evltc

clean:
	rm -f $(DOTOS)

clobber: clean
	rm -f evltc
