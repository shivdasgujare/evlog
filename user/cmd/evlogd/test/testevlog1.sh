#!/bin/bash
#
ARCH=i386
TEST_TEMPLATE=testevlog1.out

/sbin/evlconfig -L 3 -d on -i 10 -c 100
/sbin/evlsend -f LOCAL1 -t 100 -m "some junk1"
/sbin/evlsend -f LOCAL1 -t 100 -m "some junk2"
sleep 1
logtime=`date +%s`
sleep 1
./testdup3 3 3 1 0
sleep 1
/sbin/evlsend -f LOCAL1 -t 100 -m "some junk3"
sleep 5
/sbin/evlview -f "(time>${logtime} && event_type=0x7d1)" -B -S 'size=%size%\n%data%' > testevlog1.sh.out
/sbin/evlview -f "(time>${logtime} && event_type=0x7d2)" -B -S 'size=%size%\n%data%' >> testevlog1.sh.out
/sbin/evlview -f "(time>${logtime} && event_type=0x7d3)" -B -S 'size=%size%\n%data%' >> testevlog1.sh.out
/sbin/evlview -f "(time>${logtime} && facility=LOGMGMT && event_type=1)" -B -S 'size=%size%\n%data%' >> testevlog1.sh.out

diff testevlog1.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo testevlog1		:PASSED
else
	echo testevlog1		:FAILED
fi
