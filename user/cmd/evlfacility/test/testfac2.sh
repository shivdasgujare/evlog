#!/bin/bash

# This test test the restricted filter
# evlsend should not be able to log this message since the restricted query 
# does not meet.

TESTNAME=testfac2.sh

/sbin/evlfacility -a TESTFAC2 -f 'uid="root" && severity=INFO'
#
# wait for the facility to get to memory
#
sleep 6
logtime=`date +%s`
/sbin/evlsend -f TESTFAC2 -t 102 -s WARNING -m "ROOT guy try to log this warning message"
sleep 1
/sbin/evlview -f "(time>=${logtime} && event_type=102)" -B -S 'size=%size%\nseverity=%severity%\n%data%' > $TESTNAME.out

diff $TESTNAME.out testfac2.tpl.out

if [ $? -eq "0" ]
then 
	echo "testfac2	:PASSED"
	/sbin/evlfacility -d TESTFAC2
else
	echo "testfac2	:FAILED"
fi

