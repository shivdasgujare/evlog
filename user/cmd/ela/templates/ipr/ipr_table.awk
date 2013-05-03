BEGIN {
	comma = 0
	print "#include \"ela.h\""
	print "#include \"ipr_ela.h\""
	print
	print "const struct ipr_table;"
	print "const {"
	print "  struct ela_entry e[] = {"
}

$1=="event_type" {
	if (comma) {
		print "    ,"
	}
	sub(/^event_type[ 	]*/,"")
	sub(/;$/,",")
	print "    {"
	print "\t" $0
	next
}
/^eventName[ 	]/ {
	sub(/^eventName[ ]*=[ 	]*/,"")
	sub(/;$/,"")
	eventName = $0
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
	sub(/^File[ 	]*=[ 	]*/,"")
	sub(/;$/,"")
	file = $0
	next
}
/^Class[ 	]/ {
	sub(/^Class[ 	]*=[ 	]*/,"")
	sub(/;$/,"")
	class = $0
	next
}
/^Type[ 	]/ {
	sub(/^Type[ 	]*=[ 	]*/,"")
	sub(/;$/,"")
	type = $0
	print "\t" eventName ", " file ", " class ", " type ","
	next
}
/^ProbableCauses[ 	]/ {
	print "\t{"
	indent++
	next
}
/^Actions[ 	]/ {
	print "\t{"
	indent++
	next
}
/^};/ {
	print "\t},"
	indent--
	next
}
/^Threshold[ 	]/ {
	sub(/^Threshold[ 	]*=[ 	]*/,"")
	sub(/;$/,"")
	threshold = $0
	next
}
/^Interval[ 	]/ {
	sub(/^Interval[ 	]*=[ 	]*/,"")
	sub(/;$/,"")
	if ($0 ~ /\"/) {
		interval = $0
	} else {
		interval = "\"" $0 "\""
	}
	print "\t" threshold ", " interval
	next
}
/^attributes[ 	]/ {
	print "    }"
	comma = 1
	next
}
/^}$/ {
	next
}

{
	for (i = 1; i <= indent; i++) { printf "\t" }
	print $0
}

END {
	print "  } delimiter=\"\\n\";"
	print "}"
	print "format string \"%e%\""
	print "END"
}
