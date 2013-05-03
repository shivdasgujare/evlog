#!/bin/bash
#
TEST_TEMPLATE=degenerate.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc $EVLTMPLPATH/user/degenerate.t
else
echo >&2 "degenerate.sh: Not running as root.  Assuming template source file"
echo >&2 "$EVLTMPLPATH/user/degenerate.t has already been compiled."
fi

lastRecid=`/sbin/evlview -t 1 -S %recid%`
./degenerate
sleep 1
/sbin/evlview -f "recid>$lastRecid && event_type > 4100 && event_type < 4200" -S '%facility% %event_type:d% %severity%:\n%data%' > degenerate.sh.out
echo '-----' >> degenerate.sh.out
/sbin/evlview -b -f "recid>$lastRecid && category=\"client\"" -S '%facility% %event_type:d% %severity%:\n%data%' >> degenerate.sh.out


diff degenerate.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo test of degenerate templates/records	:PASSED
else
	echo test of degenerate templates/records	:FAILED
	exit 1
fi
rm -f degenerate.sh.out
exit 0
