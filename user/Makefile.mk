SHELL = /bin/sh

KERN_VERSION=v2.4.20
PPC_K64_U32=
EVL_LOG_H=../kernel/$(KERN_VERSION)/include/linux/evl_log.h

DISTROS=
ifeq ($(DISTROS), debian)
INITDOTD_DIR	= $(DESTDIR)/etc/init.d
RCDOTD_BASE	= $(DESTDIR)/etc
REL_INITDOTD_DIR = ../init.d
else
ifeq ($(DISTROS), suse)
INITDOTD_DIR   = $(DESTDIR)/etc/init.d
RCDOTD_BASE    = $(DESTDIR)/etc/rc.d
REL_INITDOTD_DIR = ../../init.d
else
INITDOTD_DIR	= $(DESTDIR)/etc/init.d
RCDOTD_BASE	= $(DESTDIR)/etc/rc.d
REL_INITDOTD_DIR = ../../init.d
endif
endif

SBINDIR		= $(DESTDIR)/sbin
INCDIR		= $(DESTDIR)/usr/include
LIBDIR		= $(DESTDIR)/usr/lib
SLIBDIR		= $(DESTDIR)/lib
STATEDIR	= $(DESTDIR)@localstatedir@
EVLOG_CONF_DIR	= $(DESTDIR)/etc/evlog.d
MANDIR		= $(DESTDIR)/usr/share/man
ETCDIR		= $(DESTDIR)/etc
OPT_EVLOG_DIR	= $(DESTDIR)/opt/evlog
USR_SHARE_EVLOG_DIR	= $(DESTDIR)/usr/share/evlog

MAKE=make -f Makefile.mk

# 
PID =`ps -ef | grep evlogd | grep -v grep | awk '$$3==1 {print $$2}'`	
PID1 =`ps -ef | grep evlogrmtd | grep -v grep | awk '{print $$2}'`	
PID2 =`ps -ef | grep evlnotifyd | grep -v grep | awk '{print $$2}'`	
PID3 =`ps -ef | grep evlactiond | grep -v grep | awk '{print $$2}'`	

all: user/lib user/libevlsyslog user/cmd user/man

user/lib:
	cd lib; $(MAKE) KERN_VERSION=$(KERN_VERSION)

user/libevlsyslog:
	cd libevlsyslog; $(MAKE) KERN_VERSION=$(KERN_VERSION)

user/cmd:
	cd cmd; $(MAKE) KERN_VERSION=$(KERN_VERSION) PPC_K64_U32=$(PPC_K64_U32)

user/man:
	cd man; $(MAKE) KERN_VERSION=$(KERN_VERSION)

install: install-package install-links install-local

install-package:
#Creating dirs
	if [ -z "$(DESTDIR)" -a -d /etc/rc.d/init.d ] ; then \
		test -L /etc/init.d || ln -s /etc/rc.d/init.d /etc/init.d; \
	fi
	if [ -z "$(DESTDIR)" -a -d /etc/init.d/rc3.d ] ; then \
		test -L /etc/rc.d || ln -s /etc/init.d /etc/rc.d; \
	fi
	mkdir -p $(SLIBDIR)
	mkdir -p $(INCDIR)/linux
	mkdir -p $(LIBDIR)
	mkdir -p $(SBINDIR)
	mkdir -p $(MANDIR)/man1
	#mkdir -p $(INITDOTD_DIR)
	#mkdir -p $(RCDOTD_BASE)
	mkdir -p $(ETCDIR)/cron.d

	mkdir -p $(EVLOG_CONF_DIR)
	mkdir -p $(OPT_EVLOG_DIR)
	mkdir -p $(USR_SHARE_EVLOG_DIR)
	mkdir -p $(STATEDIR)/templates/kern
	mkdir -p $(STATEDIR)/templates/logmgmt
	mkdir -p $(STATEDIR)/test
	mkdir -p $(STATEDIR)/test/templates/user
	mkdir -p $(STATEDIR)/test/templates/local7
	mkdir -p $(STATEDIR)/test/templates/hr

#Stop EVL daemons
	if [ -z "$(DESTDIR)" ]; then \
		test -z $(PID3) || kill -s SIGTERM $(PID3); \
		test -z $(PID2) || kill -s SIGTERM $(PID2); \
		test -z $(PID1) || kill -s SIGTERM $(PID1); \
		test -z $(PID) || kill -s SIGTERM $(PID); \
		sleep 1; \
	fi

# EVL headers
	install ./include/posix_evlog.h $(INCDIR)
	install ./include/posix_evlsup.h $(INCDIR)
	install ./include/evlog.h $(INCDIR)
	install ./include/evl_template.h $(INCDIR)
	install ./include/evl_list.h $(INCDIR)
	install $(EVL_LOG_H) $(INCDIR)/linux/evl_log.h

# Libraries
	install -m 644 ./lib/libevl.a $(LIBDIR)
	install -m 755 ./lib/libevl.so $(LIBDIR)
	install -m 755 ./libevlsyslog/libevlsyslog.so $(SLIBDIR)

# ld.so.preload manager
	install -m 700 ./libevlsyslog/slog_fwd $(SBINDIR)

#EVL Registries and logs
	install -m 644 ./lib/facility_registry $(EVLOG_CONF_DIR)
	install -m 644 ./cmd/evlconfig/evlog.conf $(EVLOG_CONF_DIR)
	install -m 644 ./cmd/evlactiond/action_registry $(EVLOG_CONF_DIR)
	install -m 644 ./cmd/evlactiond/action_profile $(EVLOG_CONF_DIR)
	install -m 644 ./cmd/evlogd/evlhosts $(EVLOG_CONF_DIR)
	install -m 600 ./cmd/evlogd/evlogrmtd.conf $(EVLOG_CONF_DIR)
	install -m 644 ./libevlsyslog/libevlsyslog.conf $(EVLOG_CONF_DIR)
# EVL daemons	
	install -s -m 744 ./cmd/evlactiond/evlactiond $(SBINDIR)
	install -s -m 744 ./cmd/evlnotifyd/evlnotifyd $(SBINDIR)
	install -s -m 744 ./cmd/evlogd/evlogd $(SBINDIR)
	install -s -m 744 ./cmd/evlogd/evlogrmtd $(SBINDIR)

# EVL commands
	install -s -m 755 ./cmd/evlview/evlview $(SBINDIR)
	install -s -m 755 ./cmd/evlsend/evlsend $(SBINDIR)
	install -s -m 755 ./cmd/evlconfig/evlconfig $(SBINDIR)
	install -s -m 755 ./cmd/evlnotify/evlnotify $(SBINDIR)
	install -s -m 755 ./cmd/evlfacility/evlfacility $(SBINDIR)
	install -s -m 755 ./cmd/evltc/evltc $(SBINDIR)
	install -s -m 755 ./cmd/evlogmgr/evlogmgr $(SBINDIR)
	if [ -z "$(DESTDIR)" ] ; then \
		install -s -m 755 ./cmd/evlgentmpls/evlgentmpls $(SBINDIR); \
	fi

# ELA
	install -m 644 ./cmd/ela/ela_funcs $(USR_SHARE_EVLOG_DIR)
	install -m 744 ./cmd/ela/ela_crt_rpt $(USR_SHARE_EVLOG_DIR)
	install -m 744 ./cmd/ela/ela_sig $(USR_SHARE_EVLOG_DIR)
	install -m 744 ./cmd/ela/ela $(USR_SHARE_EVLOG_DIR)
	install -m 744 ./cmd/ela/ela_console $(USR_SHARE_EVLOG_DIR)
	install -m 744 ./cmd/ela/ela_hw_alert $(USR_SHARE_EVLOG_DIR)
	install -m 744 ./cmd/ela/ipr_opaque_alert $(USR_SHARE_EVLOG_DIR)
	install -m 744 ./cmd/ela/ela_fake_event $(USR_SHARE_EVLOG_DIR)

	install -s -m 744 ./cmd/ela/ela_add $(SBINDIR)
	install -s -m 744 ./cmd/ela/ela_sig_send $(SBINDIR)
	install -s -m 755 ./cmd/ela/ela_get_atts $(SBINDIR)
	install -m 744 ./cmd/ela/ela_show $(SBINDIR)
	install -m 744 ./cmd/ela/ela_show_all $(SBINDIR)
	install -m 744 ./cmd/ela/ela_remove $(SBINDIR)
	install -m 744 ./cmd/ela/ela_remove_all $(SBINDIR)

# EVL start up scripts to /opt/evlog dir evlog.spec will copy them to 
# /etc/init.d
	install -m 755 ./sysfiles/etc/init.d/evlog $(OPT_EVLOG_DIR)/evlog
	install -m 755 ./sysfiles/etc/init.d/evlogrmt $(OPT_EVLOG_DIR)/evlogrmt
	install -m 755 ./sysfiles/etc/init.d/evlnotify $(OPT_EVLOG_DIR)/evlnotify
	install -m 755 ./sysfiles/etc/init.d/evlaction $(OPT_EVLOG_DIR)/evlaction
	if [ -z "$(DESTDIR)" ] ; then \
		install -m 755 ./sysfiles/etc/init.d/evlog $(INITDOTD_DIR)/evlog; \
		install -m 755 ./sysfiles/etc/init.d/evlogrmt $(INITDOTD_DIR)/evlogrmt; \
		install -m 755 ./sysfiles/etc/init.d/evlnotify $(INITDOTD_DIR)/evlnotify; \
		install -m 755 ./sysfiles/etc/init.d/evlaction $(INITDOTD_DIR)/evlaction; \
	fi

#	install -m 755 ./sysfiles/etc/logrotate.d/evlog $(ETCDIR)/logrotate.d
#	rm -f /etc/logrotate.d/evlog

# Default cron jobs for evlogmgr
	install -m 644 ./cmd/evlogmgr/evlogmgr.cron $(ETCDIR)/cron.d

#EVL man page
	install -m 644 ./man/evlview.1.gz $(MANDIR)/man1
	install -m 644 ./man/evlog.1.gz $(MANDIR)/man1
	install -m 644 ./man/evlsend.1.gz $(MANDIR)/man1
	install -m 644 ./man/evlconfig.1.gz $(MANDIR)/man1
	install -m 644 ./man/evlnotify.1.gz $(MANDIR)/man1
	install -m 644 ./man/evlquery.1.gz $(MANDIR)/man1
	install -m 644 ./man/evlfacility.1.gz $(MANDIR)/man1
	install -m 644 ./man/evltc.1.gz $(MANDIR)/man1	
	install -m 644 ./man/evlogmgr.1.gz $(MANDIR)/man1
	install -m 644 ./man/evlremote.1.gz $(MANDIR)/man1
	install -m 644 ./man/evlgentmpls.1.gz $(MANDIR)/man1

#EVL templates
	install -m 644 ./templates/Makefile $(STATEDIR)/templates
	install -m 644 ./templates/logmgmt/Makefile $(STATEDIR)/templates/logmgmt
	install -m 644 ./templates/logmgmt/logmgmt.t $(STATEDIR)/templates/logmgmt
# Note: No templates/kern/Makefile yet.
	install -m 644 ./templates/kern/kern.t $(STATEDIR)/templates/kern

#EVL test scripts, binaries
	install -m 755 ./lib/test/evl_log_write_test $(STATEDIR)/test
	install -m 755 ./lib/test/testevlwrite.sh $(STATEDIR)/test
	install -m 666 ./lib/test/evl_log_write.out $(STATEDIR)/test
	install -m 666 ./lib/test/evl_log_write.ppc.out $(STATEDIR)/test
	install -m 666 ./lib/test/evl_log_write.s390.out $(STATEDIR)/test
	install -m 666 ./lib/test/evl_log_write.s390x.out $(STATEDIR)/test
	install -m 666 ./lib/test/evl_log_write.ia64.out $(STATEDIR)/test
	install -m 755 ./lib/test/evl_log_write_test2 $(STATEDIR)/test
	install -m 755 ./lib/test/testevlwrite2.sh $(STATEDIR)/test
	install -m 644 ./lib/test/evl_log_write2.out $(STATEDIR)/test
	install -m 644 ./lib/test/evl_log_write_test2.t $(STATEDIR)/test/templates/user
	install -m 755 ./lib/test/testevlsend_b.sh $(STATEDIR)/test
	install -m 755 ./lib/test/evlsend_b.sh $(STATEDIR)/test
	install -m 755 ./lib/test/aligntest $(STATEDIR)/test
	install -m 755 ./lib/test/aligntest.sh $(STATEDIR)/test
	install -m 644 ./lib/test/aligntest.out $(STATEDIR)/test
	install -m 644 ./lib/test/aligntest.t $(STATEDIR)/test/templates/user
	install -m 755 ./lib/test/aostest $(STATEDIR)/test
	install -m 755 ./lib/test/aostest.sh $(STATEDIR)/test
	install -m 644 ./lib/test/aostest.out $(STATEDIR)/test
	install -m 644 ./lib/test/aostest.t $(STATEDIR)/test/templates/user
	install -m 755 ./lib/test/aosApiTest $(STATEDIR)/test
	install -m 755 ./lib/test/aosApiTest.sh $(STATEDIR)/test
	install -m 644 ./lib/test/aosApiTest.out $(STATEDIR)/test
	install -m 755 ./lib/test/defaultTest $(STATEDIR)/test
	install -m 755 ./lib/test/defaultTest.sh $(STATEDIR)/test
	install -m 644 ./lib/test/defaultTest.out $(STATEDIR)/test
	install -m 644 ./lib/test/defaultTest.ia64.out $(STATEDIR)/test
	install -m 644 ./lib/test/defaultTest.ppc.out $(STATEDIR)/test
	install -m 644 ./lib/test/defaultTest.s390.out $(STATEDIR)/test
	install -m 644 ./lib/test/defaultTest.s390x.out $(STATEDIR)/test
	install -m 644 ./lib/test/defaultTest.t $(STATEDIR)/test/templates/local7
	install -m 755 ./lib/test/degenerate $(STATEDIR)/test
	install -m 755 ./lib/test/degenerate.sh $(STATEDIR)/test
	install -m 644 ./lib/test/degenerate.out $(STATEDIR)/test
	install -m 644 ./lib/test/degenerate.t $(STATEDIR)/test/templates/user
	install -m 755 ./lib/test/delimiter $(STATEDIR)/test
	install -m 755 ./lib/test/delimiter.sh $(STATEDIR)/test
	install -m 644 ./lib/test/delimiter.out $(STATEDIR)/test
	install -m 644 ./lib/test/delimiter.t $(STATEDIR)/test/templates/user
	install -m 755 ./lib/test/stattest $(STATEDIR)/test
	install -m 755 ./lib/test/stattest.sh $(STATEDIR)/test
	install -m 644 ./lib/test/stattest.t  $(STATEDIR)/test/templates/user
	install -m 644 ./lib/test/stat.t  $(STATEDIR)/test/templates/user
	install -m 644 ./lib/test/statdefs.h  $(STATEDIR)/test/templates/user

	install -m 755 ./lib/test/tmplMiscTest $(STATEDIR)/test
	install -m 755 ./lib/test/tmplMiscTest.sh $(STATEDIR)/test
	install -m 644 ./lib/test/tmplMiscTest.out $(STATEDIR)/test
	install -m 644 ./lib/test/tmplMiscTest.t $(STATEDIR)/test/templates/user
	install -m 644 ./lib/test/hr.t $(STATEDIR)/test/templates/hr

	install -m 755 ./lib/test/bitfields/bftest $(STATEDIR)/test
	install -m 755 ./lib/test/bitfields/bftest.sh $(STATEDIR)/test
	install -m 644 ./lib/test/bitfields/bftest.out $(STATEDIR)/test
	install -m 644 ./lib/test/bitfields/bftest.t $(STATEDIR)/test/templates/user
	install -m 755 ./lib/test/bitfields/bftestSN $(STATEDIR)/test
	install -m 755 ./lib/test/bitfields/bftestSN.sh $(STATEDIR)/test
	install -m 644 ./lib/test/bitfields/bftestSN.out $(STATEDIR)/test
	install -m 644 ./lib/test/bitfields/bftestS.t $(STATEDIR)/test/templates/user
	install -m 755 ./lib/test/syslogatTest $(STATEDIR)/test
	install -m 755 ./lib/test/syslogatTest.sh $(STATEDIR)/test
	install -m 644 ./lib/test/syslogatTest.out $(STATEDIR)/test
	install -m 755 ./lib/test/runtests.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/runacttests.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/test1.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/test2.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/test3.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/test4.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/test5.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/test6.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/test7.sh $(STATEDIR)/test
	install -m 666 ./cmd/evlnotify/test/actions7.nfy $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/rmactions.sh $(STATEDIR)/test
	install -m 666 ./cmd/evlnotify/test/actions.nfy $(STATEDIR)/test
	install -m 755 ./cmd/evlnotify/test/loadactions.sh $(STATEDIR)/test

	install -m 755 ./cmd/evlfacility/test/testfac1.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlfacility/test/testfac2.sh $(STATEDIR)/test
	install -m 755 ./cmd/evlfacility/test/testfac3.sh $(STATEDIR)/test
	install -m 666 ./cmd/evlfacility/test/testfac1.tpl.out $(STATEDIR)/test
	install -m 666 ./cmd/evlfacility/test/testfac2.tpl.out $(STATEDIR)/test
	install -m 666 ./cmd/evlfacility/test/testfac3.tpl.out $(STATEDIR)/test

	install -m 755 ./cmd/evlogd/test/testdup $(STATEDIR)/test
	install -m 755 ./cmd/evlogd/test/testdup3 $(STATEDIR)/test
	install -m 755 ./cmd/evlogd/test/testevlog1.sh $(STATEDIR)/test
	install -m 666 ./cmd/evlogd/test/testevlog1.out $(STATEDIR)/test

	install -m 744 ./cmd/evlogmgr/test/logmgmt1.sh $(STATEDIR)/test

	install -m 644 ./cmd/ela/test/ela.h $(STATEDIR)/test
	install -m 744  ./cmd/ela/test/log_evt_test1.sh $(STATEDIR)/test
	install -m 744 ./cmd/ela/test/ela_test1.sh $(STATEDIR)/test
	install -m 744 ./cmd/ela/test/ela_test2.sh $(STATEDIR)/test
	install -m 744 ./cmd/ela/test/run_ela_tests.sh $(STATEDIR)/test
	install -m 644 ./cmd/ela/test/elatest1.t $(STATEDIR)/test
	install -m 644 ./cmd/ela/test/elatest2.t $(STATEDIR)/test

install-links:
#Creating links			

	-ln -fs $(REL_INITDOTD_DIR)/evlog $(RCDOTD_BASE)/rc3.d/S09evlog
	-ln -fs $(REL_INITDOTD_DIR)/evlog $(RCDOTD_BASE)/rc5.d/S09evlog
	-ln -fs $(REL_INITDOTD_DIR)/evlog $(RCDOTD_BASE)/rc0.d/K90evlog
	-ln -fs $(REL_INITDOTD_DIR)/evlog $(RCDOTD_BASE)/rc6.d/K90evlog

	-ln -fs $(REL_INITDOTD_DIR)/evlogrmt $(RCDOTD_BASE)/rc3.d/S89evlogrmt
	-ln -fs $(REL_INITDOTD_DIR)/evlogrmt $(RCDOTD_BASE)/rc5.d/S89evlogrmt
	-ln -fs $(REL_INITDOTD_DIR)/evlogrmt $(RCDOTD_BASE)/rc0.d/K60evlogrmt
	-ln -fs $(REL_INITDOTD_DIR)/evlogrmt $(RCDOTD_BASE)/rc6.d/K60evlogrmt
	# Remove older links
	test ! -L $(RCDOTD_BASE)/rc3.d/S80evlnotify || rm -f $(RCDOTD_BASE)/rc3.d/S80evlnotify
	test ! -L $(RCDOTD_BASE)/rc5.d/S80evlnotify || rm -f $(RCDOTD_BASE)/rc5.d/S80evlnotify 
	-ln -fs $(REL_INITDOTD_DIR)/evlnotify $(RCDOTD_BASE)/rc3.d/S07evlnotify
	-ln -fs $(REL_INITDOTD_DIR)/evlnotify $(RCDOTD_BASE)/rc5.d/S07evlnotify
	-ln -fs $(REL_INITDOTD_DIR)/evlnotify $(RCDOTD_BASE)/rc0.d/K50evlnotify
	-ln -fs $(REL_INITDOTD_DIR)/evlnotify $(RCDOTD_BASE)/rc6.d/K50evlnotify
	# Remove older links
	test ! -L $(RCDOTD_BASE)/rc3.d/S90evlaction ||  rm -f $(RCDOTD_BASE)/rc3.d/S90evlaction
	test ! -L $(RCDOTD_BASE)/rc5.d/S90evlaction || rm -f $(RCDOTD_BASE)/rc5.d/S90evlaction
	-ln -fs $(REL_INITDOTD_DIR)/evlaction $(RCDOTD_BASE)/rc3.d/S08evlaction
	-ln -fs $(REL_INITDOTD_DIR)/evlaction $(RCDOTD_BASE)/rc5.d/S08evlaction
	-ln -fs $(REL_INITDOTD_DIR)/evlaction $(RCDOTD_BASE)/rc0.d/K40evlaction
	-ln -fs $(REL_INITDOTD_DIR)/evlaction $(RCDOTD_BASE)/rc6.d/K40evlaction

install-local:	
	-@/sbin/ldconfig

	test -n "$(DESTDIR)" || make -C $(STATEDIR)/templates

startall:
	$(INITDOTD_DIR)/evlog start
	sleep 1
	$(INITDOTD_DIR)/evlogrmt start
	sleep 1
	$(INITDOTD_DIR)/evlnotify start
	sleep 1
	$(INITDOTD_DIR)/evlaction start

stopall:
	test -z $(PID3) || kill -s SIGTERM $(PID3)
	sleep 1
	test -z $(PID2) || kill -s SIGTERM $(PID2)
	sleep 1
	test -z $(PID1) || kill -s SIGTERM $(PID1)
	sleep 1
	test -z $(PID) || kill -s SIGTERM $(PID)
	sleep 1

clean:
	(cd lib; $(MAKE) clean)
	(cd libevlsyslog; $(MAKE) clean)
	(cd cmd; $(MAKE) clean)
	(cd man; $(MAKE) clean)
	(cd templates; $(MAKE) clean)

clobber: clean
	(cd lib; $(MAKE) clobber)
	(cd libevlsyslog; $(MAKE) clobber)
	(cd cmd; $(MAKE) clobber)
	(cd man; $(MAKE) clobber)
	(cd templates; $(MAKE) clobber)

