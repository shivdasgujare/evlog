#!/bin/bash

FACILITY=$1
ret=`/sbin/evlfacility -l | awk '{print $2"#"}' | grep -i $FACILITY#`

if [ -z $ret ]; then
	# Try to add facility
	/sbin/evlfacility -a $FACILITY
	ret=$?
	if [ $ret -ne "0" ]; then 
		echo "ERROR: Failed to add facility $FACILITY."
		echo "You must be root to add  a new facility"
		exit 1
	fi
fi

exit 0 

