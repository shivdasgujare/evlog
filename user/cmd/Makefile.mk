KERN_VERSION=v2.4.4
PPC_K64_U32=
INCDIR=../../include
HEADERS=$(INCDIR)/posix_evlog.h $(INCDIR)/posix_evlsup.h $(INCDIR)/evlog.h $(INCDIR)/linux/evl_log.h
MAKE=make -f Makefile.mk

all: cmd/evlogd cmd/evlsend cmd/evlview cmd/evlconfig cmd/evlnotifyd \
	 cmd/evlactiond cmd/evlnotify cmd/evlfacility cmd/evltc cmd/evlogmgr \
	 cmd/evlgentmpls cmd/ela
	
cmd/evlogd: 
	cd evlogd; $(MAKE) KERN_VERSION=$(KERN_VERSION) PPC_K64_U32=$(PPC_K64_U32)

cmd/evlsend:
	cd evlsend; $(MAKE) KERN_VERSION=$(KERN_VERSION)

cmd/evlview: 
	cd evlview; $(MAKE) KERN_VERSION=$(KERN_VERSION)

cmd/evlnotifyd: 
	cd evlnotifyd; $(MAKE) KERN_VERSION=$(KERN_VERSION)

cmd/evlactiond: 
	cd evlactiond; $(MAKE) KERN_VERSION=$(KERN_VERSION)

cmd/evlconfig:
	cd evlconfig; $(MAKE) KERN_VERSION=$(KERN_VERSION)

cmd/evlnotify:
	cd evlnotify; $(MAKE) KERN_VERSION=$(KERN_VERSION)
	
cmd/evlfacility:
	cd evlfacility; $(MAKE) KERN_VERSION=$(KERN_VERSION)
	
cmd/evltc:
	cd evltc; $(MAKE) KERN_VERSION=$(KERN_VERSION)

cmd/evlogmgr:
	cd evlogmgr; $(MAKE) KERN_VERSION=$(KERN_VERSION)

cmd/evlgentmpls:
	if [ -z "$(DESTDIR)" ] ; then \
		cd evlgentmpls;	$(MAKE) KERN_VERSION=$(KERN_VERSION); \
	fi

cmd/ela:
	cd ela; $(MAKE) KERN_VERSION=$(KERN_VERSION)

clean:
	(cd evlogd; $(MAKE) clean)
	(cd evlsend; $(MAKE) clean)
	(cd evlview; $(MAKE) clean)
	(cd evlconfig; $(MAKE) clean)
	(cd evlnotifyd; $(MAKE) clean)
	(cd evlactiond; $(MAKE) clean)	
	(cd evlnotify; $(MAKE) clean)
	(cd evlfacility; $(MAKE) clean)
	(cd evltc; $(MAKE) clean)
	(cd evlogmgr; $(MAKE) clean)
	(cd evlgentmpls; $(MAKE) clean)
	(cd ela; $(MAKE) clean)

clobber:
	(cd evlogd; $(MAKE) clobber)
	(cd evlsend; $(MAKE) clobber)
	(cd evlview; $(MAKE) clobber)
	(cd evlconfig; $(MAKE) clobber)
	(cd evlnotifyd; $(MAKE) clobber)
	(cd evlactiond; $(MAKE) clobber)
	(cd evlnotify; $(MAKE) clobber)
	(cd evlfacility; $(MAKE) clobber)
	(cd evltc; $(MAKE) clobber)
	(cd evlogmgr; $(MAKE) clobber)
	(cd evlgentmpls; $(MAKE) clobber)
	(cd ela; $(MAKE) clobber)

