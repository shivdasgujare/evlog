KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include

SBINDIR=$(DESTDIR)/sbin
STATEDIR=$(DESTDIR)/var/evlog

DEBUG=-g
UROOT=../..
INC=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
CFLAGS=$(DEBUG) $(INC)
DOTOS=evlgentmpls.o
LIBS=$(UROOT)/lib/libevl.a -lpthread -lbfd -liberty

evlgentmpls: $(DOTOS) $(UROOT)/lib/libevl.a
	$(CC) $(DOTOS) $(LIBS) -o evlgentmpls

install:
	mkdir -p $(SBINDIR)
#	mkdir -p $(STATEDIR)/test

	install -s -m 755 evlgentmpls $(SBINDIR) 
#	install -m 755 $(UROOT)/lib/test/syslogatTest $(STATEDIR)/test
#	install -m 755 $(UROOT)/lib/test/syslogatTest.sh $(STATEDIR)/test
#	install -m 644 $(UROOT)/lib/test/syslogatTest.out $(STATEDIR)/test

clean:
	rm -f $(DOTOS)

clobber: clean
	rm -f evlgentmpls
