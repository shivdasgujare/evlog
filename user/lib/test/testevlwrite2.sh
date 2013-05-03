#!/bin/sh
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

logtime=`date +%s`
sleep 1
./evl_log_write_test2
sleep 1
/sbin/evlview -f "(time>${logtime} && event_type >= 1001 && event_type <= 1100)" -S 'event type %event_type:d%:\n%data%' > testevlwrite2.sh.out

diff testevlwrite2.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo evl_log_write with templates	:PASSED
else
	echo evl_log_write with templates	:FAILED
fi
