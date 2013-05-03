KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

DEBUG=-g
LIBDOTOS=q.tab.o lex.qq.o normalize.o evaluate.o qopt.o
LIBDOTSOS=q.tab.os lex.qq.os normalize.os evaluate.os qopt.os


UROOT=../..
INC=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
CFLAGS=$(DEBUG) $(INC)
CFLAGS_SO=-fPIC -O -D_SHARED_LIB $(DEBUG) $(INC)

all: $(LIBDOTOS) $(LIBDOTSOS)
	touch made

q.tab.o: q.tab.c evl_parse.h q.tab.h
	$(CC) $(CFLAGS) -c $< -o $@

q.tab.os: q.tab.c evl_parse.h q.tab.h
	$(CC) $(CFLAGS_SO) -c $< -o $@

lex.qq.c: lex
	flex -Pqq lex

lex.qq.o: lex.qq.c q.tab.h
	$(CC) $(CFLAGS) -c $< -o $@

lex.qq.os: lex.qq.c q.tab.h
	$(CC) $(CFLAGS_SO) -c $< -o $@

q.tab.c: q.tab.h

q.tab.h: query.y
	yacc -dl -b q -p qq query.y

normalize.o: normalize.c evl_parse.h q.tab.h
	$(CC) $(CFLAGS) -c $< -o $@

normalize.os: normalize.c evl_parse.h q.tab.h
	$(CC) $(CFLAGS_SO) -c $< -o $@

evaluate.o: evaluate.c evl_parse.h q.tab.h
	$(CC) $(CFLAGS) -c $< -o $@

evaluate.os: evaluate.c evl_parse.h q.tab.h
	$(CC) $(CFLAGS_SO) -c $< -o $@

qopt.o: qopt.c evl_parse.h q.tab.h
	$(CC) $(CFLAGS) -c $< -o $@

qopt.os: qopt.c evl_parse.h q.tab.h
	$(CC) $(CFLAGS_SO) -c $< -o $@

clean:
	rm -f q.tab.h q.tab.c lex.qq.c *.o *.os made

clobber: clean
