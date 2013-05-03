UROOT=../../..
KERN_VERSION=v2.4.4
KERN_INC=$(UROOT)/../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h

DEBUG=-g

# Link with share lib
#LIBEVL=-levl
# Link with static lib
LIBEVL=$(UROOT)/lib/libevl.a

LIBS=$(LIBEVL) -lpthread
INC=-I$(UROOT)/include -I$(KERN_INC)
CFLAGS=$(DEBUG) $(INC)

# mktest.sh takes a while to run.  So by default, we build only those tests
# that we include with the package.
ALLTESTS=bftest bftestN bftestU bftestUN bftestS bftestSN
TESTS=bftest bftestSN

all: $(TESTS)

bftest: bftest.c $(LIBEVL)
	$(CC) $(CFLAGS) -o bftest bftest.c $(LIBS)

bftestN: bftestN.c $(LIBEVL)
	$(CC) $(CFLAGS) -o bftestN bftestN.c $(LIBS)

bftestU: bftestU.c $(LIBEVL)
	$(CC) $(CFLAGS) -o bftestU bftestU.c $(LIBS)

bftestUN: bftestUN.c $(LIBEVL)
	$(CC) $(CFLAGS) -o bftestUN bftestUN.c $(LIBS)

bftestS: bftestS.c $(LIBEVL)
	$(CC) $(CFLAGS) -o bftestS bftestS.c $(LIBS)

bftestSN: bftestSN.c $(LIBEVL)
	$(CC) $(CFLAGS) -o bftestSN bftestSN.c $(LIBS)

# These rules also create the corresponding .t files (bftest.t or bftestU.t).
bftest.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh

bftestN.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -n

bftestU.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -u

bftestUN.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -u -n

bftestS.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -s

bftestSN.c: mktest.sh bftest.cpre bftest.cpost bftest.tpre bftest.tpost mkc.awk mkt.awk
	./mktest.sh -s -n

clean:
	rm -f *.o *.sh.out

clobber: clean
	rm -f $(ALLTESTS) \
		bftest.c bftestN.c bftestU.c bftestUN.c bftestS.c bftestSN.c \
		bftest.t bftestU.t bftestS.t
