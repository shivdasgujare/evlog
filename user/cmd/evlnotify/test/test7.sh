#!/bin/bash

./rmactions.sh
/sbin/evlnotify -F actions7.nfy
sleep 1
/sbin/evlsend -f LOCAL0 -t 100 -m "start test1"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test2"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test3"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test4"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test5"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test6"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test7"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test8"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test9"
/sbin/evlsend -f LOCAL0 -t 100 -m "start test0"

#
# Sleep for a while so that all notification can be delivered
#
sleep 3
let lines=0
for recid in `cat /tmp/test7.out | awk '{print $4}'`
do
	echo "Action was executed with recid=$recid as an argument." 
	let lines=$lines+1
done

#lines=`wc -l /tmp/test7.out | awk '{print $1}'`

if [ "$lines" -eq "10" ]
then
	echo "test7	:PASSED"
	rm -f /tmp/test7.out
else
	echo "test7	:FAILED"
	mv  /tmp/test7.out /tmp/test7.output
fi
echo "-----------------"
./rmactions.sh
		
