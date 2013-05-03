#!/bin/sh
#
# This test will test the once_only flag 
#
/sbin/evlnotify --add "echo %recid% > test5.out" --filter 'data = "test5"' --persistent --once-only
sleep 1
/sbin/evlsend -f LOCAL0 -t 100 -m "test5"
sleep 1

if [ -f /tmp/test5.out ]
then
	RECID1=`cat /tmp/test5.out`
else
	echo "test5	:FAILED"
	exit 1
fi

/sbin/evlsend -f LOCAL0 -t 100 -m "test5"

sleep 1

if [ -f /tmp/test5.out ]
then
	RECID2=`cat /tmp/test5.out`
	if [ "$RECID2" -eq "$RECID1" ]
	then
 		echo "test5	:PASSED"
	else
 		echo "test5	:FAILED"
	fi
else
	echo "test5	:FAILED"
fi
echo "-----------------"
