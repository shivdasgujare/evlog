BEGIN {
	facilityStmt = "[facility statement missing]"
	indent = 0
	doing_atts = 0
	eventNamed = 0
	nUnnamedEvents = 0
	defaultFile = ""

	print "#include \"ela.h\""
	print ""
}

$1 == "defaultFile" {
	defaultFile = $3
	next
}

/^facility[ 	]/ {
	facilityStmt = $0
	next
}

$1=="event_type" {
	print facilityStmt
	file = defaultFile
	if (netdev_printk == 1) {
		$1 = "event_type NPFX_FMT"
		pkfunc="netdev_printk"
	} else if (DPRINTK == 1) {
		$1 = "event_type DPRINTK_PFX_FMT"
		pkfunc="DPRINTK"
	} else if (dev_printk == 1) {
		$1 = "event_type DPFX_FMT"
		pkfunc="dev_printk"
	} else {
		pkfunc="printk"
	}
	print $0
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

$1=="printk" {
	if (printk == 1) {
		print facilityStmt
		file = defaultFile
		$1 = "event_type"
		pkfunc="printk"
		print $0
	}
	next
}

$1=="netdev_printk" {
	if (netdev_printk == 1) {
		print facilityStmt
		file = defaultFile
		$1 = "event_type NPFX_FMT"
		pkfunc="netdev_printk"
		print $0
	}
	next
}

$1=="dev_printk" {
	if (dev_printk == 1) {
		print facilityStmt
		file = defaultFile
		$1 = "event_type DPFX_FMT"
		pkfunc="dev_printk"
		print $0
	}
	next
}

$1=="DPRINTK" {
	if (DPRINTK == 1) {
		print facilityStmt
		file = defaultFile
		$1 = "event_type DPRINTK_PFX_FMT"
		pkfunc="DPRINTK"
		print $0
	}
	next
}

/^eventName[ 	]/ {
	eventNamed = 1
	print "const {"
	print "\tstring " $0
	next
}

/^File[ 	]/ {
	file = $3
	next
}
/^Class[ 	]/ {
	if (! eventNamed) {
		nUnnamedEvents++
		print "const {"
	}
	eventNamed = 0
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
	if (pkfunc == "dev_printk") {
		print "\tDPFX_ATTS"
	} else if (pkfunc == "netdev_printk") {
		print "\tNPFX_ATTS"
	} else if (pkfunc == "DPRINTK") {
		print "\tDPRINTK_PFX_ATTS"
	}
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
	if (doing_atts && pkfunc != "printk" && $0 ~ "PRINTK_ONLY") {
		next
	}
	for (i = 1; i <= indent; i++) { printf "\t" }
	print $0
}

END {
	if (nUnnamedEvents > 0) {
		print "Warning: " nUnnamedEvents " unnamed events" > "/dev/stderr"
	}
}
