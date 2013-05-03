#!/bin/sh
# 
# This test testing the security feature (access denied) of the evlnotify -
#

user="nobody"

touch /etc/evlog.d/action.deny
echo $user > /etc/evlog.d/action.deny
echo "This test is testing for access denied"
/sbin/evlnotify --add "echo test3" --persistent --uid $user
ret=$?

sleep 1
if [ $ret -eq  "1" ]
then
	echo "test3	:PASSED"
else
	echo "test3	:FAILED"
fi
echo "-----------------"

rm /etc/evlog.d/action.deny

