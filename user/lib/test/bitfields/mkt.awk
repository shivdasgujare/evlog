BEGIN {
	print "facility 8; event_type " evty ";"
	print "aligned attributes {"
	instruct = 0
	natts = 0
}

/struct bf1 {/ {
	instruct = 1
	next
}

/^};$/ {
	if (instruct) {
		print "}"
		printf("format string \"")
		printf("natts =%2d \\t", natts)
		for (i = 0; i < natts; i++) {
			printf("%%%s%% ", atts[i])
		}
		printf("\"\nEND\n")
		exit
	}
}

{
	if (instruct) {
		if (unSigned && $1 != "struct" && $1 != "enum") {
			sub("\t", "\tu")
		} else if (Signed && $1 == "char") {
			sub("char", "schar")
		}
		print
		if ($1 == "struct" || $1 == "enum") {
			memname = $3
		} else if (NF == 2) {
			memname = $2
		} else {
			print "Don't know how to parse: " $0
			next
		}
		sub(/;/, "", memname)
		split(memname, s, /[\[:]/)
		if (s[1] != "") {
			atts[natts++] = s[1]
		}
	}
}
