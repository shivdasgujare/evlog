#!/bin/bash
#
TEST_TEMPLATE=aostest.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc $EVLTMPLPATH/user/aostest.t
else
echo >&2 "aostest.sh: Not running as root.  Assuming template source file"
echo >&2 "$EVLTMPLPATH/user/aostest.t has already been compiled."
fi

logtime=`date +%s`
sleep 1
./aostest
sleep 1
/sbin/evlview -f "(time>${logtime} && event_type > 4000 && event_type < 4100)" -S 'event type %event_type:d%:\n%data%' > aostest.sh.out

diff aostest.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo templates array-of-structs test	:PASSED
else
	echo templates array-of-structs	test	:FAILED
fi
