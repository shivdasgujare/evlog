KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

DEBUG=-g
LIBDOTOS := $(patsubst %.c, %.o,$(wildcard *.c))
LIBDOTSOS := $(patsubst %.c, %.os,$(wildcard *.c))
UROOT=../..
INC=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
CFLAGS=$(DEBUG) $(INC)
CFLAGS_SO=-fPIC -O -D_SHARED_LIB $(DEBUG) $(INC)

all: $(LIBDOTOS) $(LIBDOTSOS)
	touch made

scanner.os: scanner.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

evl_list.os: evl_list.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

format.os: format.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

fmtbuf.os: fmtbuf.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

evl_common.os: evl_common.c
	$(CC) $(CFLAGS_SO) -c $< -o $@
	
clean:
	rm -f $(LIBDOTOS) $(LIBDOTSOS) made

clobber: clean
