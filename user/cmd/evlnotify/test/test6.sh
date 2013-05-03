#!/bin/sh

./rmactions.sh
sleep 1
/sbin/evlnotify --list > /tmp/actions.list

lines=`wc -l /tmp/actions.list | awk '{print $1}'`

if [ "$lines" -eq "0" ]
then
	echo "test6	:PASSED"
else
	echo "test6	:FAILED"
fi
echo "-----------------"
		
rm -f /tmp/actions.list
