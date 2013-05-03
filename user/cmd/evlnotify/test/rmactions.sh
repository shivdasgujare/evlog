#!/bin/bash
#
# This script will remove all registered actions
# If you run this scriot as root it will remove all
# registered actions. However if you run this script
# as yourself (joeuser) it will only remove joeuser's
# registered actions.
#
verbose=off

if [ "$1" = "-v" ]
then
	verbose=on
fi
/sbin/evlnotify --list > /tmp/actions.list

cat /tmp/actions.list | awk -F ":" '{print $1}' > /tmp/nfy_id.list

for nfy_id in `cat /tmp/nfy_id.list`
do
	if [ "$verbose" = "on" ]
	then
		echo "removing actioniID $nfy_id"
	fi
	/sbin/evlnotify -d $nfy_id
done

rm -f /tmp/actions.list
rm -f /tmp/nfy_id.list



