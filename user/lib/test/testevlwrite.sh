#!/bin/sh
#
ARCH=i386
TEST_TEMPLATE=evl_log_write.out

if [ ! -z $1 ]
then
	ARCH=$1
fi

if [ "$ARCH" = "i386" ]
then
	TEST_TEMPLATE=evl_log_write.out
else
	TEST_TEMPLATE=evl_log_write.$ARCH.out
fi

logtime=`date +%s`
sleep 1
./evl_log_write_test
sleep 1
/sbin/evlview -f "(time>${logtime} && event_type=1000)" -B -S 'size=%size%\n%data%' | sed "s/[ ]*| .*//g" > testevlwrite.sh.out

diff testevlwrite.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo evl_log_write	:PASSED
else
	echo evl_log_write	:FAILED
fi
