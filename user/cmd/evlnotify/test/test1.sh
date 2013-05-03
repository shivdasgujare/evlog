#!/bin/sh
#
# This test s testing for basic functionality (notification and action)
# It loads actions in the actions.nfy file which registers an action that 
# will create /tmp/test1.out file and an action that will remove the file.
#

if [ -f /tmp/test1.out ]
then
	rm /tmp/test1.out
fi

/sbin/evlnotify --add 'echo "test 1 started" > /tmp/test1.out' -f 'data="start test1"' -p
/sbin/evlnotify --add "rm test1.out" -f 'data="remove test1.out"' -p
sleep 1
/sbin/evlsend -f LOCAL0 -t 100 -m "start test1"
sleep 1

if [ -f /tmp/test1.out ]
then
	cat /tmp/test1.out
else
	echo "test1	:FAILED"
	exit 1
fi

/sbin/evlsend -f LOCAL0 -t 100 -m "remove test1.out"

sleep 1

if [ ! -f /tmp/test1.out ]
then
 	echo "test1	:PASSED"
else
	echo "test1	:FAILED"
fi
echo "-----------------"
