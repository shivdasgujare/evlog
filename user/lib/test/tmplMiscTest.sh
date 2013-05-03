#!/bin/bash

export LANG=en_US
TEST_TEMPLATE=tmplMiscTest.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc $EVLTMPLPATH/hr/hr.t
	/sbin/evltc $EVLTMPLPATH/user/tmplMiscTest.t
else
echo >&2 "tmplMiscTest.sh: Not running as root.  Assuming the following"
echo >&2 "template source files jave already been compiled:"
echo >&2 $EVLTMPLPATH/hr/hr.t
echo >&2 $EVLTMPLPATH/user/tmplMiscTest.t
fi

logtime=`date +%s`
sleep 1
./tmplMiscTest
sleep 1
/sbin/evlview -f "time>${logtime}" -S 'event type %event_type:d%:\n%data%' > tmplMiscTest.sh.out

diff tmplMiscTest.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo templates misc test	:PASSED
else
	echo templates misc test	:FAILED
fi
