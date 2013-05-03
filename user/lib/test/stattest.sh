#!/bin/sh

if [ -z "$1" ]
then
	ARCH=i386
else
	ARCH=$1
fi

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

if [ "$USER" = root ]
then
	/sbin/evltc -D__${ARCH}__ $EVLTMPLPATH/user/stat.t
	/sbin/evltc $EVLTMPLPATH/user/stattest.t
else
echo >&2 "stattest.sh: Not running as root.  Assuming template source files"
echo >&2 "stat.t and stattest.t in $EVLTMPLPATH/user have already been compiled."
fi

./stattest > stattest.out1
/sbin/evlview -f 'event_type=2050' -t 1 -S '%data%' > stattest.out2

diff stattest.out1 stattest.out2

if [ $? -eq "0" ]
then
	echo templates stattest	:PASSED
else
	echo templates stattest	:FAILED
fi
