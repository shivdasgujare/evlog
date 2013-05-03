#!/bin/sh

i=0
while [ $i -lt 10 ]
do
let i=$i+1;
evlsend -s DEBUG -f LOCAL0 -t 999 -m "$i debug message"
done

/sbin/evlogmgr -s "severity=DEBUG && facility=LOCAL0" > /tmp/val1.out

val1=`cat /tmp/val1.out | grep "records matching"  | sed 's/\([A-Za-z]\)//g' | sed 's/ //g' | sed 's/\.//'`

if [ $val1 -ne 10 ]
then
	echo "logmgmt1	step 1   :FAILED"
	exit 1
fi
/sbin/evlogmgr -c "severity=DEBUG && facility=LOCAL0"
if [ $? -ne 0 ]
then
	echo "logmgmt1 step 2    :FAILED "
	exit 1
fi

/sbin/evlogmgr -s "severity=DEBUG && facility=LOCAL0" > /tmp/val2.out
val2=`cat /tmp/val2.out | grep "records matching" | sed  's/\([A-Za-z]\)//g' | sed 's/ //g' | sed 's/\.//'` 

if [ $val2 -ne 0 ]
then
	echo "logmgmt1 step 3   :FAILED"
	exit 1
fi

echo "logmgmt1      :PASSED"
rm -f /tmp/val*.out
