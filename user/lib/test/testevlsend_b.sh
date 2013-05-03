#!/bin/bash
# ./evlsend_b.sh should log the same set of events as ./evl_log_write_test2.
#
TEST_TEMPLATE=evl_log_write2.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc $EVLTMPLPATH/user/evl_log_write_test2.t
else
echo >&2 "testevlwrite2.sh: Not running as root.  Assuming template source file"
echo >&2 "$EVLTMPLPATH/user/evl_log_write_test2.t has already been compiled."
fi

lastRecid=`/sbin/evlview -t 1 -S %recid%`
./evlsend_b.sh
sleep 1
/sbin/evlview -f "(recid>$lastRecid && event_type >= 1001 && event_type <= 1100)" -S 'event type %event_type:d%:\n%data%' > testevlsend_b.sh.out

diff testevlsend_b.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo evlsend --binary with templates	:PASSED
else
	echo evlsend --binary with templates	:FAILED
fi
