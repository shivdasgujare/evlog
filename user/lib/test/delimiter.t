facility "user"; event_type 6000;
const {
        int i[3] = {10,20,30};
}
format string "%i%"
END

facility "user"; event_type 6001;
const {
        int i[3] = {10,20,30}   delimiter=", ";
}
format string "%i%"
END

facility "user"; event_type 6002;
const {
        int i[3] = {10,20,30}   "%x";
}
format string "%i%"
END

facility "user"; event_type 6003;
const {
        int i[3] = {10,20,30}   "%x" delimiter=", ";
}
format string "%i%"
END

facility "user"; event_type 6012;
const {
        int i[3] = {10,20,30}   "(%x)";
}
format string "%i%"
END

facility "user"; event_type 6013;
const {
        int i[3] = {10,20,30}   "(%x)" delimiter=", ";
}
format string "%i%"
END

facility "user"; event_type 6015;
const {
	char c[6] = {'h', 'e', 'l', 'l', 'o', '\0'}	"%s";
}
format string "%c%"
END
facility "user"; event_type 6016;
const {
	char c[6] = {'h', 'e', 'l', 'l', 'o', '\0'}	"%c";
}
format string "%c%"
END
facility "user"; event_type 6017;
const {
	char c[6] = {'h', 'e', 'l', 'l', 'o', '\0'}	"%d" delimiter = ", ";
}
format string "%c%"
END

struct struct_t;
aligned attributes {
	int one;
	int two;
}
format string "{%one%, %two%}"
END

facility "user"; event_type 6021;
attributes {
	struct struct_t str[2];
}
format
"%str%"
END
facility "user"; event_type 6022;
attributes {
	struct struct_t str[2]		delimiter=", ";
}
format
"%str%"
END
facility "user"; event_type 6023;
attributes {
	struct struct_t str[2]		"(%Z)";
}
format
"%str%"
END
facility "user"; event_type 6024;
attributes {
	struct struct_t str[2]		"(%Z)" delimiter=", ";
}
format
"%str%"
END
facility "user"; event_type 6025;
attributes {
	struct struct_t str[2]		"%Z" delimiter=", ";
}
format
"%str%"
END
facility "user"; event_type 6026;
attributes {
	struct struct_t str[2]		"%Z";
}
format
"%str%"
END

/* Tests of delimiter=attname */
struct params;
attributes {
	string delim;
}
format string "%delim%"
END
facility "user"; event_type 6031;
const {
	struct params p = { "\n" };
	int i[3] = {10,20,30}	delimiter=p.delim;
}
format string "%i%"
END
facility "user"; event_type 6032;
const {
	string delim = ":";
	int i[3] = {10,20,30}	delimiter=delim;
}
format string "%i%"
END
facility "user"; event_type 6033;
attributes {
	struct params p;
	int i[3] delimiter=p.delim;
}
format string "%i%"
END
facility "user"; event_type 6034;
attributes {
	string delim;
	int i[3] delimiter=delim;
}
format string "%i%"
END
