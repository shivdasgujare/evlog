#!/bin/bash
#
TEST_TEMPLATE=bftest.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc -c $EVLTMPLPATH/user/bftest.t
else
echo >&2 "bftest.sh: Not running as root.  Assuming template source file"
echo >&2 "$EVLTMPLPATH/user/bftest.t has already been compiled."
fi

logtime=`date +%s`
sleep 1
./bftest
sleep 1
/sbin/evlview -f "(time>${logtime} && event_type>3000 && event_type<3200)" -S '%event_type:d%: %data%' > bftest.sh.out

diff bftest.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo templates bitfield test	:PASSED
else
	echo templates bitfield test	:FAILED
fi
