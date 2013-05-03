#!/bin/sh
EVLOG_STATE_DIR=/var/evlog
TMPLDIR=$EVLOG_STATE_DIR/templates
ELA_RPTDIR=$EVLOG_STATE_DIR/ela_report
ELA_BINDIR=/sbin

facility=e1000s
evt_type=0xd591e94e
test_templ=elatest1.t

mkdir $TMPLDIR/$facility
cp $test_templ $TMPLDIR/$facility
cp ela.h $TMPLDIR/$facility
#
# Register facility e1000 for our test
/sbin/evlfacility -a $facility 
#
# Compile the template
#
startdir=`pwd`
cd $TMPLDIR/$facility

/sbin/evltc -c $test_templ

if [ "$?" -ne "0" ]; then
	echo "Failed to compile the template"
	exit 1
fi

#
# Generate the ela rules 
#
$ELA_BINDIR/ela_get_atts -f $facility 2> /dev/null > /tmp/ela.rules

#
# Remove all registered ela rules from the system
#
$ELA_BINDIR/ela_remove_all

#
# register new rules from /tmp/ela.rules
#
$ELA_BINDIR/ela_add -f /tmp/ela.rules

if [ "$?" -ne "0" ]; then
	echo "Failed to register new rules"
	exit 1
fi

logtime=`date +%s`
sleep 1
#
# log an event
cd $startdir
$startdir/log_evt_test1.sh
sleep 3
#
# verify that a new report is created.
#
recid=`/sbin/evlview -f "(time>${logtime} && event_type=${evt_type})" -S '%recid%'`

if [ -z "$recid" ]; then
	echo "Can't find matching event record"
	echo "ela test1:                FAILED"
	exit 1
fi	
rpt="${ELA_RPTDIR}/ela_report${recid}*"
ls $rpt 
if [ "$?" = "0" ]; then
	echo "ela_report created."
	#cat  $rpt
	echo
	echo "ela test1:		PASSED"
	/sbin/evlfacility -d $facility
	rm -rf $TMPLDIR/$facility
	exit 0
fi
echo "ela test1:		FAILED"
