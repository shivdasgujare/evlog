#
# Enterprise Event Log Spec
#
Summary:   Enterprise Event Logging
Name:      evlog
Version:   1.6.1 
Release:   1
Copyright: LGPL
Vendor:    IBM <http://www.ibm.com>
Url:       http://evlog.sourceforge.net

Packager:  Hien Nguyen <nguyhien@us.ibm.com>
Group:     System Environment/Base
Source:    evlog-1.6.1.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot

%description
Enterprise Event Logging

%prep
%setup
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" $LOCALFLAGS
./configure 

%build
make -C user

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

# Copy ela* stuff to /usr/bin for now, we will use move later.
mkdir -p $RPM_BUILD_ROOT/usr/sbin
cp $RPM_BUILD_ROOT/sbin/ela_* $RPM_BUILD_ROOT/usr/sbin

cd $RPM_BUILD_ROOT
find . -type d | sed '1,2d;s,^\.,\%attr(-\,root\,root) \%dir ,' |grep -v '/etc/init.d' > $RPM_BUILD_DIR/file.list.evlog-user
find . -type f | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.evlog-user
find . -type l | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.evlog-user

%pre
#echo This is pre arg=$1
if [ "$1" = "1" -o "$1" = "2" ] ; then
	if [ -f /etc/init.d/evlaction ]
	then
		PID3=`ps -ef | grep evlactiond | grep -v grep | awk '{print $2}'`
		test -z $PID3 || /etc/init.d/evlaction stop
		/bin/sleep 1
	fi
	if [ -f /etc/init.d/evlnotify ]
	then
		PID2=`ps -ef | grep evlnotifyd | grep -v grep | awk '{print $2}'`
		test -z $PID2 || /etc/init.d/evlnotify stop
		/bin/sleep 1
	fi
	if [ -f /etc/init.d/evlogrmtd ]
	then
		PID1=`ps -ef | grep evlogrmtd | grep -v grep | awk '{print $2}'`
		test -z $PID1 || /etc/init.d/evlogrmtd stop
		/bin/sleep 1
	fi	
	if [ -f /etc/init.d/evlog ]
	then
		PID=`ps -ef | grep evlogd | grep -v grep | awk '$3==1 {print $2}'`
		test -z $PID || /etc/init.d/evlog stop
		/bin/sleep 1
	fi	

	# We don't use logrotate anymore so remove evlog under logrotate.d 
	if [ -f /etc/logrotate.d/evlog ]
	then
		rm -f /etc/logrotate.d/evlog
	fi	
	
	if [ -d /etc/rc.d/init.d ]
	then 
		# Redhat style, make a link to /etc/init.d
		test -L /etc/init.d || ln -s /etc/rc.d/init.d /etc/init.d
	fi
	
	if [ -d /etc/init.d/rc3.d ]
	then
		# Suse style of rc0-6.d
		test -L /etc/rc.d || ln -s /etc/init.d /etc/rc.d
	fi
fi
	
%post
#echo This is post arg=$1
if [ "$1" = "1" -o "$1" = "2" ] ; then
	/sbin/ldconfig

	(cd /var/evlog/templates; make)
	
	/etc/init.d/evlnotify start
	/bin/sleep 1
	/etc/init.d/evlaction start
	/bin/sleep 1
	/etc/init.d/evlog start
	/bin/sleep 1
	/etc/init.d/evlogrmt start
	
	REL_INITDOTD_DIR=../../init.d
	RCDOTD_BASE=/etc/rc.d

	ln -fs $REL_INITDOTD_DIR/evlog $RCDOTD_BASE/rc3.d/S09evlog
	ln -fs $REL_INITDOTD_DIR/evlog $RCDOTD_BASE/rc5.d/S09evlog
	ln -fs $REL_INITDOTD_DIR/evlog $RCDOTD_BASE/rc0.d/K90evlog
	ln -fs $REL_INITDOTD_DIR/evlog $RCDOTD_BASE/rc6.d/K90evlog

	ln -fs $REL_INITDOTD_DIR/evlogrmt $RCDOTD_BASE/rc3.d/S89evlogrmt
	ln -fs $REL_INITDOTD_DIR/evlogrmt $RCDOTD_BASE/rc5.d/S89evlogrmt
	ln -fs $REL_INITDOTD_DIR/evlogrmt $RCDOTD_BASE/rc0.d/K60evlogrmt
	ln -fs $REL_INITDOTD_DIR/evlogrmt $RCDOTD_BASE/rc6.d/K60evlogrmt
	
	# Remove previous version (1.5.3 and earlier) links if exist
	test ! -L $RCDOTD_BASE/rc3.d/S80evlnotify || rm -f $RCDOTD_BASE/rc3.d/S80evlnotify
	test ! -L $RCDOTD_BASE/rc5.d/S80evlnotify || rm -f $RCDOTD_BASE/rc5.d/S80evlnotify
	ln -fs $REL_INITDOTD_DIR/evlnotify $RCDOTD_BASE/rc3.d/S07evlnotify
	ln -fs $REL_INITDOTD_DIR/evlnotify $RCDOTD_BASE/rc5.d/S07evlnotify
	ln -fs $REL_INITDOTD_DIR/evlnotify $RCDOTD_BASE/rc0.d/K50evlnotify
	ln -fs $REL_INITDOTD_DIR/evlnotify $RCDOTD_BASE/rc6.d/K50evlnotify
	
	# Remove previous version (1.5.3 and earlier) links if exist
	test ! -L $RCDOTD_BASE/rc3.d/S90evlaction || rm -f $RCDOTD_BASE/rc3.d/S90evlaction
	test ! -L $RCDOTD_BASE/rc5.d/S90evlaction || rm -f $RCDOTD_BASE/rc5.d/S90evlaction

	ln -fs $REL_INITDOTD_DIR/evlaction $RCDOTD_BASE/rc3.d/S08evlaction
	ln -fs $REL_INITDOTD_DIR/evlaction $RCDOTD_BASE/rc5.d/S08evlaction
	ln -fs $REL_INITDOTD_DIR/evlaction $RCDOTD_BASE/rc0.d/K40evlaction
	ln -fs $REL_INITDOTD_DIR/evlaction $RCDOTD_BASE/rc6.d/K40evlaction
fi


%preun
#echo This is preun arg=$1
if [ "$1" = "0" ] ; then

	if [ -f /etc/init.d/evlaction ]
	then
		PID3=`ps -ef | grep evlactiond | grep -v grep | awk '{print $2}'`
		test -z $PID3 || /etc/init.d/evlaction stop
		/bin/sleep 1
	fi
	if [ -f /etc/init.d/evlnotify ]
	then
		PID2=`ps -ef | grep evlnotifyd | grep -v grep | awk '{print $2}'`
		test -z $PID2 || /etc/init.d/evlnotify stop
		/bin/sleep 1
	fi
	if [ -f /etc/init.d/evlogrmt ]
	then
		PID1=`ps -ef | grep evlogrmtd | grep -v grep | awk '{print $2}'`
		test -z $PID1 || /etc/init.d/evlogrmt stop
		/bin/sleep 1
	fi	
	if [ -f /etc/init.d/evlog ]
	then
		PID=`ps -ef | grep evlogd | grep -v grep | awk '$3==1 {print $2}'`
		test -z $PID || /etc/init.d/evlog stop
		/bin/sleep 1
	fi	

	(cd /var/evlog/templates; make clobber)

	#
	# remove libevlsyslog.so.1 entry in the /etc/ld.so.preload if any
	#
	/sbin/slog_fwd -r
fi
	
%postun
#echo This is postun arg=$1
if [ "$1" = "0" ] ; then
	#
	# remove startup scripts link
	#

	if [ -e /etc/init.d/evlog ]
	then
		rm -f /etc/init.d/evlog
	fi
	if [ -e /etc/init.d/evlogrmt ]
	then
		rm -f /etc/init.d/evlogrmt
	fi
	if [ -e /etc/init.d/evlnotify ]
	then
		rm -f  /etc/init.d/evlnotify
	fi
	if [ -e /etc/init.d/evlaction ]
	then
		rm -f /etc/init.d/evlaction
	fi 
	
	RCDOTD_BASE=/etc/rc.d

	rm -f $RCDOTD_BASE/rc3.d/S09evlog
	rm -f $RCDOTD_BASE/rc5.d/S09evlog
	rm -f $RCDOTD_BASE/rc0.d/K90evlog
	rm -f $RCDOTD_BASE/rc6.d/K90evlog

	rm -f $RCDOTD_BASE/rc3.d/S89evlogrmt
	rm -f $RCDOTD_BASE/rc5.d/S89evlogrmt
	rm -f $RCDOTD_BASE/rc0.d/K60evlogrmt
	rm -f $RCDOTD_BASE/rc6.d/K60evlogrmt

	rm -f $RCDOTD_BASE/rc3.d/S07evlnotify
	rm -f $RCDOTD_BASE/rc5.d/S07evlnotify
	rm -f $RCDOTD_BASE/rc0.d/K50evlnotify
	rm -f $RCDOTD_BASE/rc6.d/K50evlnotify

	rm -f $RCDOTD_BASE/rc3.d/S08evlaction
	rm -f $RCDOTD_BASE/rc5.d/S08evlaction
	rm -f $RCDOTD_BASE/rc0.d/K40evlaction
	rm -f $RCDOTD_BASE/rc6.d/K40evlaction

	/sbin/ldconfig	
fi
	

%clean
rm -rf $RPM_BUILD_ROOT/*
rm -rf $RPM_BUILD_DIR/evlog
rm -rf ../file.list.evlog


%files -f ../file.list.evlog-user

%changelog
* Tue Apr 27 2004 Hien Nguyen <nguyhien@us.ibm.com>
- Release version of v1.6.0. Added ELA feature.
* Thu Dec 18 2003 Hien Nguyen <nguyhien@us.ibm.com>
- Added ELA feature - and bugs fixed
* Mon Jun 16 2003 Hien Nguyen <nguyhien@us.ibm.com>
- Add multi-arch support
* Thu Mar 7 2002 Jim Keniston <kenistoj@us.ibm.com>
- Added kernel module tests kfacreg and kfacreg2 (test3, test4)
* Fri Oct 12 2001 Hien Nguyen <nguyhien@us.ibm.com>
- Bug fixes, add template, evltc, evlfacility, libevlsyslog
  and many small enhancements.
* Fri Sep 28 2001 Hien Nguyen <nguyhien@us.ibm.com>
- Bug fixes, adding s390 and s390x specific tests 
* Tue Aug 26 2001 Hien Nguyen <nguyhien@us.ibm.com>
- First RPM build
