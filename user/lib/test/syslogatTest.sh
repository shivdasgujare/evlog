#!/bin/sh
#
fail() {
	echo syslogat test	: FAILED
	exit 1
}

if [ ! -x /sbin/evlgentmpls ]
then
	echo "Skipping syslogat test: /sbin/evlgentmpls not installed."
	exit 0
fi

TEST_TEMPLATE=syslogatTest.out

export EVLTMPLPATH=$EVL_TESTDIR/templates

if [ "$USER" != root ]
then
	echo >&2 "syslogatTest.sh must be run as root, since it creates files"
	echo >&2 "in $EVLTMPLPATH."
	fail
fi

rm -f $EVLTMPLPATH/local7/local7.t

/sbin/evlgentmpls $EVLTMPLPATH syslogatTest
if [ $? -ne 0 ]
then
	fail
fi

/sbin/evltc $EVLTMPLPATH/local7/local7.t
if [ $? -ne 0 ]
then
	fail
fi

lastRecid=`/sbin/evlview -t 1 -S %recid%`
./syslogatTest
if [ $? -ne 0 ]
then
	fail
fi
sleep 1

/sbin/evlview -b -f "recid>$lastRecid && where" -S %data% | sed -e 's/game:.*/game:x/' > syslogatTest.sh.out
echo '-----' >> syslogatTest.sh.out
/sbin/evlview -b -f "recid>$lastRecid && where" -S '%file%:%opponent%:%where%:%us%:%them%:%commentary%' >> syslogatTest.sh.out

diff syslogatTest.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo syslogat test	:PASSED
else
	fail
fi
rm -f syslogatTest.sh.out
exit 0
