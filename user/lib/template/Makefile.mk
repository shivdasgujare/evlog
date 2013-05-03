KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

DEBUG=-g 
DOTOS := tt.tab.o lex.tt.o convert.o template.o tmplmgmt.o tmplfmt.o serial.o bvfmt.o
DOTSOS := tt.tab.os lex.tt.os convert.os template.os tmplmgmt.os tmplfmt.os serial.os bvfmt.os 

LINUX_INCDIR=../../include/linux
INCDIR=../../include
INC=-I$(INCDIR) -I$(LINUX_INCDIR) -I$(KERN_INC)
HEADERS=$(INCDIR)/posix_evlog.h \
		$(INCDIR)/posix_evlsup.h \
		$(INCDIR)/evlog.h \
		$(INCDIR)/evl_list.h \
		$(INCDIR)/evl_template.h \
		$(LINUX_INCDIR)/evl_log.h \
		$(EVL_LOG_H)
CFLAGS=$(DEBUG) $(INC)
#CFLAGS_SO=-fPIC -O -D_SHARED_LIB $(INC)
CFLAGS_SO=-fPIC $(DEBUG) -D_SHARED_LIB $(INC)

all: $(DOTOS) $(DOTSOS)
	touch made

tt.tab.o: tt.tab.c tt.tab.h
	$(CC) $(CFLAGS) -c tt.tab.c

tt.tab.os: tt.tab.c tt.tab.h
	$(CC) $(CFLAGS_SO) -c $< -o $@

lex.tt.c: tmpllex.l
	flex -Ptt tmpllex.l

lex.tt.o: lex.tt.c tt.tab.h
	$(CC) $(CFLAGS) -c lex.tt.c

lex.tt.os: lex.tt.c tt.tab.h
	$(CC) $(CFLAGS_SO) -c $< -o $@

tt.tab.c: tt.tab.h

tt.tab.h: tmplgram.y
	yacc -dl -b tt -p tt tmplgram.y

serial.o: serial.c serialio.inc

serial.os: serial.c serialio.inc
	$(CC) $(CFLAGS_SO) -c $< -o $@

bvfmt.os: bvfmt.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

convert.os: convert.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

template.os: template.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

tmplfmt.os: tmplfmt.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

tmplmgmt.os: tmplmgmt.c
	$(CC) $(CFLAGS_SO) -c $< -o $@

$(DOTOS): $(HEADERS)

$(DOTSOS): $(HEADERS)

clean:
	rm -f *.o *.os tt.tab.c tt.tab.h lex.tt.c tt.output made

clobber: clean
