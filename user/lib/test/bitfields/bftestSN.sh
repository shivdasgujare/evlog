#!/bin/bash
#
TEST_TEMPLATE=bftestSN.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc -c $EVLTMPLPATH/user/bftestS.t
else
echo >&2 "bftestSN.sh: Not running as root.  Assuming template source file"
echo >&2 "$EVLTMPLPATH/user/bftestS.t has already been compiled."
fi

logtime=`date +%s`
sleep 1
./bftestSN
sleep 1
/sbin/evlview -f "(time>${logtime} && event_type>53000 && event_type<53200)" -S '%event_type:d%: %data%' > bftestSN.sh.out

diff bftestSN.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo templates bitfield test signed,negative	:PASSED
else
	echo templates bitfield test signed,negative	:FAILED
fi
