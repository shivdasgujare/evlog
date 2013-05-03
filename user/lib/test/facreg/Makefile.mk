DEBUG=-g
LIBS=../../libevl.a -lfl -lpthread
TESTS=frtest2
UROOT=../../..
INC=-I$(UROOT)/include
CFLAGS=$(DEBUG) $(INC)

all: $(TESTS)

frtest2: frtest2.o $(LIBS)
	$(CC) $(CFLAGS) -o frtest2 frtest2.o $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(TESTS)
