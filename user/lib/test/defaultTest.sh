#!/bin/bash

if [ -z "$1" ]
then
	ARCH=i386
else
	ARCH=$1
fi

if [ "$ARCH" = "i386" ]
then
	TEST_TEMPLATE=defaultTest.out
else
	TEST_TEMPLATE=defaultTest.$ARCH.out
fi

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH
export LANG=C

if [ "$USER" = root ]
then
	/sbin/evltc $EVLTMPLPATH/local7/defaultTest.t
else
echo >&2 "defaultTest.sh: Not running as root.  Assuming template source file"
echo >&2 "defaultTest.t in $EVLTMPLPATH/local7 has already been compiled."
fi

logtime=`date +%s`
sleep 1
./defaultTest
sleep 1
/sbin/evlview -f "time>${logtime} && event_type > 1000 && event_type < 2000" -S 'event type %event_type:d%:\n%data%' | sed "s/at line .* of/at line xx of/g" | sed "s/:.*:/::/g" | sed "s/[ ]*| .*//g" > defaultTest.sh.out

diff defaultTest.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo templates defaultTest	:PASSED
else
	echo templates defaultTest	:FAILED
fi
