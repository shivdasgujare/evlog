#!/bin/bash
#
TEST_TEMPLATE=aligntest.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc $EVLTMPLPATH/user/aligntest.t
else
echo >&2 "testevlwrite2.sh: Not running as root.  Assuming template source file"
echo >&2 "$EVLTMPLPATH/user/aligntest.t has already been compiled."
fi

logtime=`date +%s`
sleep 1
./aligntest
sleep 1
/sbin/evlview -f "(time>${logtime} && event_type >= 2001 && event_type <= 2100)" -S 'event type %event_type:d%:\n%data%' > aligntest.sh.out

diff aligntest.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo templates aligntest	:PASSED
else
	echo templates aligntest	:FAILED
fi
