struct srcInfo;
attributes {
	string srcFile;
	int srcLine;
}
format
%srcFile%:%srcLine%
END

facility "local7"; event_type default;
attributes {
	struct srcInfo si;
	string msg;
	char remainder[_R_]	"%t";
}
format
%si%: %msg%
%remainder%
END

facility "local7"; event_type 1003;
attributes {
	struct srcInfo si;
	string msg;
	int month;
	int day;
	int year;
}
format
Hey!  Something happened at line %si.srcLine% of %si.srcFile%:
%msg%
%month%/%day%/%year%
