#!/bin/sh
#
TEST_TEMPLATE=delimiter.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc $EVLTMPLPATH/user/delimiter.t
else
echo >&2 "delimiter.sh: Not running as root.  Assuming template source file"
echo >&2 "$EVLTMPLPATH/user/delimiter.t has already been compiled."
fi

lastRecid=`/sbin/evlview -t 1 -S %recid%`
./delimiter
sleep 1
/sbin/evlview -f "recid>$lastRecid && event_type > 5999 && event_type < 6100" -S '%facility% %event_type:d% %severity%:\n%data%' > delimiter.sh.out

diff delimiter.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo test of array delimiter		:PASSED
else
	echo test of array delimiter 		:FAILED
	exit 1
fi
rm -f delimiter.sh.out
exit 0
