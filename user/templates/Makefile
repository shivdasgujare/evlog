# NOTE: We do not currently compile kern/kern.t.  It's up to the user.
SUBDIRS=logmgmt

all:
	@for d in $(SUBDIRS); do (cd $$d; $(MAKE)); done

clean:
	@for d in $(SUBDIRS); do (cd $$d; $(MAKE) clean); done

clobber: clean
