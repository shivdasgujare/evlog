BEGIN {
	indent = 0
	doing_atts = 0

	print "#include \"ela.h\""
	print "#include \"ipr_ela.h\""
	print ""
}

$1=="dev_printk" {
	pkfunc = $1
	print "facility \"" facility "\";"
	$1 = "event_type DPFX_FMT"
	print $0
	next
}

$1=="ipr_sdev_printk" {
	pkfunc = $1
	print "facility \"" facility "\";"
	$1 = "event_type ISDEV_PFX_FMT"
	print $0
	next
}

$1=="ipr_printk" {
	pkfunc = $1
	print "facility \"" facility "\";"
	$1 = "event_type IPR_PFX_FMT"
	print $0
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

/^servCode[    ]/ {
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
	if ($0 ~ /\"/) {
		print "\tstring " $0
	} else {
		print "\tint " $0
	}
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
	} else if (pkfunc == "ipr_sdev_printk") {
		print "\tISDEV_PFX_ATTS"
	} else if (pkfunc == "ipr_printk") {
		print "\tIPR_PFX_ATTS"
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
