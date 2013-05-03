#!/bin/sh
#
# This test is testing for the %recid% is being passed to the 
# action script

/sbin/evlnotify --add "echo %recid% > test2.out" -p
sleep 1
/sbin/evlsend -f LOCAL0 -t 100 -m "test2-1"

sleep 1

if [ -f /tmp/test2.out ]
then
	RECID1=`cat /tmp/test2.out`
else
	echo "test2	:FAILED"
	exit 1
fi

/sbin/evlsend -f LOCAL0 -t 100 -m "test2-2"

sleep 1

if [ -f /tmp/test2.out ]
then
	RECID2=`cat /tmp/test2.out`
	if [ "$RECID2" -gt "$RECID1" ]
	then
 		echo "test2	:PASSED"
	else
 		echo "test2	:FAILED"
	fi
else
	echo "test2	:FAILED"
fi
echo "-----------------"
