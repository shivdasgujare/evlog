#!/bin/sh

# This test test the restricted filter
# Only the message with severity=INFO is logged

TESTNAME=testfac1.sh

/sbin/evlfacility -a TESTFAC1 -f 'uid="root" && severity=INFO'
sleep 6
#
#
logtime=`date +%s`
/sbin/evlsend -f TESTFAC1 -t 101 -s INFO -m "ROOT guy logs this info message"
/sbin/evlsend -f TESTFAC1 -t 101 -s WARNING -m "ROOT guy logs this warning message"
sleep 1
/sbin/evlview -f "(time>=${logtime} && event_type=101)" -B -S 'size=%size%\nseverity=%severity%\n%data%' > $TESTNAME.out

diff $TESTNAME.out testfac1.tpl.out

if [ $? -eq "0" ]
then 
	echo "testfac1	:PASSED"
	/sbin/evlfacility -d TESTFAC1
else
	echo "testfac1	:FAILED"
fi

