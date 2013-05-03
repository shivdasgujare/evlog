LIBDIRS = -L/usr/lib
LIBS=../../lib/libevl.a -lpthread
UROOT=../..
INCLUDEDIRS=-I$(UROOT)/include -I$(UROOT)/include/linux
DEBUG=-g 
CFLAGS=$(DEBUG) -O $(INCLUDEDIRS)
PRODUCTS = ela_sig_send ela_get_atts ela_add ela_fake_event

all: $(PRODUCTS)

ela_fake_event: ela_fake_event.c
	$(CC) $(CFLAGS) ela_fake_event.c $(LIBDIRS)  $(LIBS) -o ela_fake_event
ela_sig_send: ela_sig_send.c
	$(CC) $(CFLAGS) ela_sig_send.c $(LIBDIRS) $(LIB) -o ela_sig_send

ela_get_atts: ela_get_atts.c
	$(CC) $(CFLAGS) ela_get_atts.c $(LIBDIRS)  $(LIBS) -o ela_get_atts

ela_add: ela_add.c
	$(CC) $(CFLAGS) ela_add.c $(LIBDIRS) -o ela_add

clean:
	-rm -f *.o *~ \#*\#
	-rm -f $(PRODUCTS)

clobber: clean
	-rm -f $(PRODUCTS)

nothing:
