#!/bin/sh
# Usage: ./mktest.sh [-n] [-u | -s]
# Creates bftest.c, bftest.t, and variants.
# -n causes all the fields to be initialized to -1, not 1.
# -u causes all the field and member types to be unsigned.
# -s causes all members of type char to be converted to schar.
# You cannot specify both -u and -s.

nflag=
uflag=
sflag=
cSuffix=
tSuffix=
negArg=
unsArg=

baseEvty=3000

for arg
do
	case "$arg" in
	-n)
		nflag=1
		negArg="-v neg=1"
		cSuffix=${cSuffix}N
		baseEvty=`expr $baseEvty + 10000`
		;;
	-u)
		uflag=1
		unsArg="-v unSigned=1"
		cSuffix=${cSuffix}U
		tSuffix=${tSuffix}U
		baseEvty=`expr $baseEvty + 20000`
		;;
	-s)
		sflag=1
		unsArg="-v Signed=1"
		cSuffix=${cSuffix}S
		tSuffix=${tSuffix}S
		baseEvty=`expr $baseEvty + 40000`
		;;
	*)	echo "Usage: $0 [-n] [-u | -s]" >&2
		exit 1
		;;
	esac
done

if [ "$uflag" -a "$sflag" ]
then
	echo "$0: Cannot specify both -u and -s" >&2
	exit 1
fi

cfile=bftest$cSuffix.c
tfile=bftest$tSuffix.t

if [ "$sflag" ]
then
	sed -e 's/^	char/	schar/' < bftest.cpre > $cfile
	sed -e 's/^	char/	schar/' < bftest.tpre > $tfile
else
	cp bftest.cpre $cfile
	cp bftest.tpre $tfile
fi

lastbf=76
i=1
while [ $i -le $lastbf ]
do
	# bf9.c, bf48.c, and bf51.c have intentional errors.
	# bf34.c defines a struct with length 0.
	# bf49.c uses a wchar_t, whose default format is troublesome here.
	# bf50.c uses enums.
	if [ $i = 9 -o $i = 34 -o $i = 48 -o $i = 49 -o $i = 50 -o $i = 51 ]
	then
		i=`expr $i + 1`
		continue
	fi

	# bf29.c already has an unsigned member.
	if [ "$uflag" -a \( $i = 29 \) ]
	then
		i=`expr $i + 1`
		continue
	fi

	f=bf$i.c
	if [ ! -f $f ]
	then
		i=`expr $i + 1`
		continue
	fi
	evty=`expr $baseEvty + $i`
	awk -f mkc.awk -v evty=$evty $negArg $unsArg < $f >> $cfile
	awk -f mkt.awk -v evty=$evty $negArg $unsArg < $f >> $tfile

	f=bf${i}b.c
	if [ ! -f $f ]
	then
		i=`expr $i + 1`
		continue
	fi
	evty=`expr $baseEvty + 100 + $i`
	awk -f mkc.awk -v evty=$evty $negArg $unsArg < $f >> $cfile
	awk -f mkt.awk -v evty=$evty $negArg $unsArg < $f >> $tfile

	i=`expr $i + 1`
done

cat bftest.cpost >> $cfile
cat bftest.tpost >> $tfile
