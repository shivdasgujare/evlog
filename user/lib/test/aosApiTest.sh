#!/bin/sh
#
TEST_TEMPLATE=aosApiTest.out

EVLTMPLPATH=$EVL_TESTDIR/templates
export EVLTMPLPATH

# Note: We use templates that have already been compiled, and records that
# have already been logged, by aostest.sh.

./aosApiTest user 4001 season > aosApiTest.sh.out
./aosApiTest user 4002 dates >> aosApiTest.sh.out
./aosApiTest user 4003 causes >> aosApiTest.sh.out

diff aosApiTest.sh.out $TEST_TEMPLATE

if [ $? -eq "0" ]
then
	echo templates array-of-structs API test	:PASSED
else
	echo templates array-of-structs	API test	:FAILED
fi
