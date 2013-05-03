#!/bin/sh

# This test test the restricted filter
# Only the message with severity=INFO is logged

TESTNAME=testfac3.sh

/sbin/evlfacility -a "my testfac200" -f 'uid="root" && severity=INFO'
sleep 6
#
#
logtime=`date +%s`
/sbin/evlsend -f "my testfac200" -t 101 -s INFO -m "testfac3:ROOT guy logs this info message"
/sbin/evlsend -f "my testfac200" -t 101 -s WARNING -m "testfac3:ROOT guy logs this warning message"
sleep 1
/sbin/evlview -f "(time>=${logtime} && event_type=101)" -B -S 'size=%size%\nseverity=%severity%\n%data%' > $TESTNAME.out

diff $TESTNAME.out testfac3.tpl.out

if [ $? -eq "0" ]
then 
	echo "testfac3	:PASSED"
	/sbin/evlfacility -d "my testfac200"
else
	echo "testfac3	:FAILED"
fi

