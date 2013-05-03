## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

tcp_distdir = @RPM_TCP_RMTLOG_BE_NAME@-@RPM_VERSION@
udp_distdir = @RPM_UDP_RMTLOG_BE_NAME@-@RPM_VERSION@
DISTCLEANFILES += config.h spec stamp-h 
MAINTAINERCLEANFILES += aclocal.m4 config.h.in configure stamp-h.in \
			config.h stamp-h \
			evlog-@VERSION@-@RPM_RELEASE@.@ARCH@.rpm \
			evlog-@VERSION@-@RPM_RELEASE@.src.rpm \
			evlog-@VERSION@.tar.gz \
			$(tcp_distdir)-@RPM_RELEASE@.@ARCH@.rpm \
			$(tcp_distdir)-@RPM_RELEASE@.src.rpm \
			$(tcp_distdir).tar.gz \
			$(udp_distdir)-@RPM_RELEASE@.@ARCH@.rpm \
			$(udp_distdir)-@RPM_RELEASE@.src.rpm \
			$(udp_distdir).tar.gz \
			-r autom4te-*.cache # Workaround an automake bug
MOSTLYCLEANFILES += 

EVL_LOG_H = $(top_srcdir)/user/include/linux/evl_log.h
kernel_header_includedir = @KERN_HEADER_INCLUDEDIR@
kernel_header_include_HEADERS = $(EVL_LOG_H)

EXTRA_DIST += Makefile.vars Makefile.rules COPYING evlog.spec \
		autogen.sh bld_evlgentmpls.sh bld_tcp_rmtlog_be.sh \
		bld_udp_rmtlog_be.sh CHANGES README.autoconf \
		setup README.ela

# bins or src products may be required to generate stuff in etc and or
# doc. Stuff in bin, intl and lib may be required to generate stuff in
# the rest (so, this is a possible dependency order).
SUBDIRS = user 

distcleancheck:
	@:
RPM_TOP_DIR=${HOME}/rpmbuild
rpm: dist
	if [ -e ${HOME}/.rpmrc ]; \
	then \
		mv ${HOME}/.rpmrc ${HOME}/rpmrc.ORG; \
	fi; \
	echo "macrofiles: /usr/lib/rpm/macros:~/.rpmmacros" > ${HOME}/.rpmrc; \
	if [ -e ${HOME}/.rpmmacros ]; \
	then \
		mv ${HOME}/.rpmmacros ${HOME}/rpmmacros.ORG; \
	fi; \
	echo "%_topdir ${RPM_TOP_DIR}" > ${HOME}/.rpmmacros; \
	if [ -e ${RPM_TOP_DIR} ]; \
	then \
		rm -rf ${RPM_TOP_DIR}; \
	fi; \
	mkdir -p ${RPM_TOP_DIR}/BUILD; \
	mkdir ${RPM_TOP_DIR}/SOURCES; \
	mkdir ${RPM_TOP_DIR}/SPECS; \
	mkdir ${RPM_TOP_DIR}/RPMS; \
	mkdir ${RPM_TOP_DIR}/SRPMS
	cp evlog-@VERSION@.tar.gz ${RPM_TOP_DIR}/SOURCES
	@RPM_BUILD@ -ba evlog.spec
	
	cp ${HOME}/rpmbuild/SRPMS/@RPM_NAME@-@VERSION@-1.src.rpm .
	cp ${HOME}/rpmbuild/RPMS/@ARCH@/@RPM_NAME@-@VERSION@-1.@ARCH@.rpm .

links startall stopall:
	./setup $@

rpm-tcp: distdir
	rm -rf $(tcp_distdir)
	mv $(distdir) $(tcp_distdir)
	tar cfvz $(tcp_distdir).tar.gz $(tcp_distdir)
	cp  $(tcp_distdir).tar.gz ${RPM_TOP_DIR}/SOURCES	
	@RPM_BUILD@ -ba user/cmd/evlogd/tcp_rmtlog_be/tcp_rmtlog_be.spec
#	@RPM_BUILD@ -ta $(tcp_distdir).tar.gz
	rm -rf $(tcp_distdir)
	cp ${HOME}/rpmbuild/RPMS/@ARCH@/tcp_rmtlog_be-@VERSION@-1.@ARCH@.rpm .

rpm-udp: distdir
	rm -rf $(udp_distdir)
	mv $(distdir) $(udp_distdir)
	tar cfvz $(udp_distdir).tar.gz $(udp_distdir)
	cp  $(udp_distdir).tar.gz ${RPM_TOP_DIR}/SOURCES	
	@RPM_BUILD@ -ba user/cmd/evlogd/udp_rmtlog_be/udp_rmtlog_be.spec
#	@RPM_BUILD@ -ta $(udp_distdir).tar.gz
	rm -rf $(udp_distdir)
	cp ${HOME}/rpmbuild/RPMS/@ARCH@/udp_rmtlog_be-@VERSION@-1.@ARCH@.rpm .
