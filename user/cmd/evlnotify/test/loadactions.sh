#!/bin/bash
let i=10
while [ "$i" -gt "0" ]
do
	let i=$i-1
	echo "Loading actions..."
	/sbin/evlnotify -F actions.nfy
done
