#!/bin/sh
# evl_log_write_test2.c recoded using evlsend

set -e

larray="30 31 32 33 34"
iarray="1 2 3 4 5"
llarray="10 11 12 13 14"
ullarray="20 21 22 23 24"
sarray="1 2 3 4 5"
carray="0x6c 0x6f 0x76 0x65"
ucarray="0xff 0x10 0xfe 0x20"
ldarray="2.33333 3.333333"
ld1=1.7676764
ld2=5.57575757
addr1=0xfeedf00d
addr2=0xfacef00d
addrarray="0 1 2 $addr1 $addr2"
plal="peace love and linux"
c=0x63
wpeace=peace
wlove=love
wand=and
wlinux=linux
wplal="$wpeace $wlove $wand $wlinux"
wc=W
	
facility=8
severity=3

type=1001
evlsend -f $facility -s $severity -t $type -b \
	"uint" 10 \
	"int" 10 \
	"int[]" 5 $iarray \
	"2*short" 10 11 \
	"string" love \
	"int" 20 \
	"int[]" 5 $iarray \
	"ulong" 0xf0f0f0f0 \
	"char" $c

type=1002
evlsend -f $facility -s $severity -t $type -b \
	"short" 9 \
	"longlong" 56 \
	"ulonglong" 56 \
	"int" 10 \
	"longlong[]" 3 10 11 12 \
	"string" peace \
	"int" 20 \
	"int[]" 5 $iarray \
	"ulong" 0xf0f0f0f0 \
	"char" $c
	
type=1003
evlsend -f $facility -s $severity -t $type -b \
	"longlong[]" 5 $llarray \
	"ulonglong[]" 5 $ullarray \
	string linux
	
type=1004
evlsend -f $facility -s $severity -t $type -b \
	"int[]" 5 $iarray

type=1005
evlsend -f $facility -s $severity -t $type -b \
	"short" 1 \
	"ushort" 2 \
	"long" 1111 \
	"ulong" 2222 \
	"ulonglong" 10 \
	"longlong" 10 \
	"float" 0.123 \
	"double" 9.999 \
	"ldouble" $ld1
	
type=1006
evlsend -f $facility -s $severity -t $type -b \
	"char" A \
	"uchar" B \
	"ldouble" $ld2

type=1007
evlsend -f $facility -s $severity -t $type -b \
	"longlong[]" 5 $llarray \
	"ulonglong[]" 5 $ullarray \
	"int" 10

type=1008
evlsend -f $facility -s $severity -t $type -b \
	"int[]" 5 $iarray \
	"uint[]" 5 $iarray \
	"short[]" 5 $sarray \
	"ushort[]" 5 $sarray \
	"char[]" 4 $carray \
	"uchar[]" 4 $ucarray \
	"string" "peace" \
	"long[]" 5 $larray \
	"string" "linux"

type=1009
evlsend -f $facility -s $severity -t $type -b \
	"6*int" 10 11 12 13 14 15 \
	"2*longlong" 10 11 \
	"ldouble[]" 2 $ldarray \
	"2*ldouble" $ld1 $ld2

type=1010
evlsend -f $facility -s $severity -t $type -b \
	"2*address" $addr1 $addr2 \
	"address[]" 5 $addrarray \
	"address" 0xfeedface

type=1011
evlsend -f $facility -s $severity -t $type -b \
	"string[]" 4 $plal

type=1012
evlsend -f $facility -s $severity -t $type -b \
	"wchar" $wc \
	"wstring" $wpeace \
	"2*wstring" $wlove $wlinux \
	"wstring[]" 4 $wplal

exit 0
