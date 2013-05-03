BEGIN {
	indent = 0
	doing_atts = 0

	print "#include \"ela.h\""
	print ""
}

$1=="event_type" {
	print "facility \"" facility "\";"
	print $0
	print "const {"
	sub(/;/, "", $2)
	print "\tstring eventName = \"" facility $2 "\";"
	next
}

$1=="MSG" {
	sub(/MSG/, "msgFmt =")
	print "\tstring " $0
	next
}

/^eventName[ 	]/ {
	print "\tstring " $0
	next
}

/^reportType[    ]/ {
	print "\tstring " $0
	next
}

/^servCode[     ]/ {
	print "\tstring " $0
	next
}

/^FRU_SRC_Priority[    ]/ {
	print "\tstring " $0
	next
}

/^File[ 	]/ {
	sub(/F/, "f")
	print "\tstring " $0
	next
}
/^Class[ 	]/ {
	print "\tElaClass ela" $0
	next
}
/^Type[ 	]/ {
	print "\tElaType ela" $0
	next
}
/^ProbableCauses[ 	]/ {
	sub(/P/, "p")
	print "\tElaStringList " $0
	indent++
	next
}
/^Actions[ 	]/ {
	sub(/A/, "a")
	print "\tElaStringList " $0
	indent++
	next
}
/^};/ {
	print "\t" $0
	indent--
	next
}
/^Threshold[ 	]/ {
	sub(/T/, "t")
	print "\tint " $0
	next
}
/^Interval[ 	]/ {
	sub(/I/, "i")
	if ($0 !~ /\"/) {
		sub(/= /, "= \"")
		sub(/;/, "\";")
	}
	print "\tstring " $0
	next
}
/^attributes[ 	]/ {
	print "}"
	print ""
	print $0
	print "\tstring fmt;"
	print "\tint argsz;"
	print "\tDPFX_ATTS"
	print "\tint board;"
	print "\tint msgNum \"%04d\";"
	print "\tstring msg;"
	doing_atts = 1
	next
}
/^}$/ {
	print $0
	print "format string \"%fmt:printk%\""
	print "END"
	doing_atts = 0
	next
}

{
	if (doing_atts) {
		next
	}
	for (i = 1; i <= indent; i++) { printf "\t" }
	print $0
}
