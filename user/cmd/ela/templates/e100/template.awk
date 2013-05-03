BEGIN {
	indent = 0
	doing_atts = 0
	defaultFile = ""

	print "#include \"ela.h\""
	print "#define DPRINTK_PFX_FMT \"e100: %s: %s: \""
	print "#define DPRINTK_PFX_ATTS string dev_name; string function;"
	print ""
}

$1 == "defaultFile" {
	defaultFile = $3
	next
}

$1 == "DPRINTK" {
	print "facility \"" FACILITY "\";"
	$1 = "event_type DPRINTK_PFX_FMT"
	print $0
	file = defaultFile
	next
}

/^eventName[ 	]/ {
	print "const {"
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
	file = $3;
	next
}

/^Class[ 	]/ {
	print "\tstring file = " file
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
		ival = $3
		sub(/;/, "", ival);
		$3 = "\"" ival "\";"
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
	print "\tDPRINTK_PFX_ATTS"
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
	for (i = 1; i <= indent; i++) { printf "\t" }
	print $0
}
