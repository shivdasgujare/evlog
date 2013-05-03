BEGIN {
	print "\tevent_type = " evty ";"
	printf("    {\n");
	sname = "rec_" evty
	instruct = 0
}

/struct bf1 {/ {
	instruct = 1
	printf("\tstruct rec_%s {\n", evty);
	next
}

/^};$/ {
	if (instruct) {
		printf("\t};\n");
	}
	instruct = 0;
}

/struct bf1 v1/ {
	sub("bf1", sname)
	if (neg) {
		gsub("1,", "-1,")
		gsub("1}", "-1}")
	}
	print "\t" $0
}

/gcc bug.*workaround/ {
	if (neg) {
		sub("1;", "-1;")
	}
	print $0
}

{
	if (instruct) {
		if (unSigned && $1 != "struct" && $1 != "enum") {
			sub("\t", "\tu")
		} else if (Signed && $1 == "char") {
			sub("char", "schar")
		}
		print "\t" $0
	}
}

END {
	print "\tstatus = posix_log_write(facility, event_type, severity,"
	print "\t\t&v1, sizeof(v1), POSIX_LOG_BINARY, 0);"
    	print "\tcheckStatus(status, " evty ");"
	print "    }"
}
