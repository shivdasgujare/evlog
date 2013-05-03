KERN_VERSION=v2.4.4
KERN_INC=../../../kernel/$(KERN_VERSION)/include
EVL_LOG_H=$(KERN_INC)/linux/evl_log.h
MAKE=make -f Makefile.mk

DEBUG=-g

# Link with share lib
#LIBEVL=-levl
# Link with static lib
LIBEVL=../libevl.a

LIBS=-lpthread $(LIBEVL)
TESTS=fmtTest dumpTest fussLog attttest evl_log_write_test seektest \
	evl_log_write_test2 aligntest stattest aostest aosApiTest defaultTest \
	threadTest tmplMiscTest tweakLogGen degenerate delimiter printfTest \
	syslogatTest threadTest2 printfArray syslogatArray
UROOT=../..
INC=-I$(UROOT)/include -I$(UROOT)/include/linux -I$(KERN_INC)
CFLAGS=$(DEBUG) $(INC)

all: $(TESTS) bftests

bftests:
	(cd bitfields; $(MAKE) KERN_VERSION=$(KERN_VERSION))

fmtTest: fmtTest.o $(LIBS)
	$(CC) $(CFLAGS) -o fmtTest fmtTest.o $(LIBS)

dumpTest: dumpTest.o $(LIBS)
	$(CC) $(CFLAGS) -o dumpTest dumpTest.o $(LIBS)

fussLog: fussLog.o $(LIBS)
	$(CC) $(CFLAGS) -o fussLog fussLog.o $(LIBS)

attttest: attttest.o $(LIBS)
	$(CC) $(CFLAGS) -o attttest attttest.o $(LIBS)

seektest: seektest.o $(LIBS)
	$(CC) $(CFLAGS) -o seektest seektest.o $(LIBS)

evl_log_write_test: evl_log_write_test.o $(LIBS)
	$(CC) $(CFLAGS) -o evl_log_write_test evl_log_write_test.o $(LIBS)

evl_log_write_test2: evl_log_write_test2.o $(LIBS)
	$(CC) $(CFLAGS) -o evl_log_write_test2 evl_log_write_test2.o $(LIBS)

aligntest: aligntest.o $(LIBS)
	$(CC) $(CFLAGS) -o aligntest aligntest.o $(LIBS)

stattest: stattest.o $(LIBS)
	$(CC) $(CFLAGS) -o stattest stattest.o $(LIBS)

aostest: aostest.o $(LIBS)
	$(CC) $(CFLAGS) -o aostest aostest.o $(LIBS)

aosApiTest: aosApiTest.o $(LIBS)
	$(CC) $(CFLAGS) -o aosApiTest aosApiTest.o $(LIBS)

defaultTest: defaultTest.o $(LIBS)
	$(CC) $(CFLAGS) -o defaultTest defaultTest.o $(LIBS)

threadTest: threadTest.o $(LIBS)
	$(CC) $(CFLAGS) -o threadTest threadTest.o $(LIBS)

threadTest2: threadTest2.o $(LIBS)
	$(CC) $(CFLAGS) -o threadTest2 threadTest2.o $(LIBS)

tmplMiscTest: tmplMiscTest.o $(LIBS)
	$(CC) $(CFLAGS) -o tmplMiscTest tmplMiscTest.o $(LIBS)

tweakLogGen: tweakLogGen.o $(LIBS)
	$(CC) $(CFLAGS) -o tweakLogGen tweakLogGen.o $(LIBS)

degenerate: degenerate.o $(LIBS)
	$(CC) $(CFLAGS) -o degenerate degenerate.o $(LIBS)

delimiter: delimiter.o $(LIBS)
	$(CC) $(CFLAGS) -o delimiter delimiter.o $(LIBS)

printfTest: printfTest.o $(LIBS)
	$(CC) $(CFLAGS) -o printfTest printfTest.o $(LIBS)

printfArray: printfArray.o $(LIBS)
	$(CC) $(CFLAGS) -o printfArray printfArray.o $(LIBS)

syslogatTest: syslogatTest.o $(LIBS)
	$(CC) $(CFLAGS) -o syslogatTest syslogatTest.o $(LIBS)

syslogatArray: syslogatArray.o $(LIBS)
	$(CC) $(CFLAGS) -o syslogatArray syslogatArray.o $(LIBS)

clean:
	rm -f *.o
	(cd bitfields; $(MAKE) clean)

clobber: clean
	(cd bitfields; $(MAKE) clobber)
	rm -f $(TESTS)
