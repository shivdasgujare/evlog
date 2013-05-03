import stat;

facility "USER";
event_type 2050;
attributes {
	struct stat st;
}
format
%st%
