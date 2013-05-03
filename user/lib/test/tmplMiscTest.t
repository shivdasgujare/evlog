facility "USER"; event_type 5001;
/* Test of various kinds of constants */
const {
	char	c = 'c' "%c";
	int	i = 1;
	double	d = 1.1;
	double	d2 = 2.2e2;
	string	s = "string s";

}
attributes {
	string	msg;
}
format
c=%c%
i=%i%
d=%d:.2f%
d2=%d2:.2f%
s=%s%
msg=%msg%
END

/* Test of %b and %v formats. */
struct _Organism;
attributes {
	string		name;

	int btype	"%b/b1/human/b0/inhuman/"
			"b1x/adult/b0x/juvenile/"
			"b1xx/female/b0xx/male/";

	int xtype	"%b/x7/Woman/"
			"x5/Girl/"
			"x1/Human/x2/Adult/x4/Female/";

	int vtype	"%v/1/boy/"
			"3/man/"
			"5/girl/"
			"7/woman/";
}
format
name=%name%, type=%btype% = %xtype% = %vtype%
END

typedef struct _Organism Organism;

facility "USER"; event_type 5012;
attributes {
	string		msg;
	Organism	folks[_R_]	"(%Z\n)";
}
format
%folks%
%msg%
END

/* Test of import dir.* */
import hr.*;

facility "USER"; event_type 5003;
description "This template uses the hr.employee template.";
attributes {
	string		msg;
	struct employee	employee;
}
format
%employee%
%msg%
END

facility "USER"; event_type 5004;
/* Test of the use of one attribute as another attribute's dimension */
attributes {
	string	msg;
	char	nParents;
	string	parents[nParents]	"(%s )";
	char	nSons;
	string	sons[nSons]		"(%s )";
	char	nDaughters;
	string	daughters[nDaughters]	"(%s )";
	char	nCats;
	string	cats[nCats]		"(%s )";
	char	nDogs;
	string	dogs[nDogs]		"(%s )";
	char	nComputers;
	string	computers[nComputers]	"(%s )";
	char	nOthers;
	string	others[nOthers]		"(%s )";
}
format
%msg%
parents: %parents%
sons: %sons%
daughters: %daughters%
cats: %cats%
dogs: %dogs%
computers: %computers%
others: %others%
END

struct familyFlags;
aligned attributes {
		uint	fatherFlag:1;
		uint	motherFlag:1;
		uint	sonFlag:1;
		uint	daughterFlag:1;
		uint	catFlag:1;
		uint	dogFlag:1;
		uint	computerFlag:1;
		uint	otherFlag:1;
}
format
fatherFlag: %fatherFlag%
motherFlag: %motherFlag%
sonFlag: %sonFlag%
daughterFlag: %daughterFlag%
catFlag: %catFlag%
dogFlag: %dogFlag%
computerFlag: %computerFlag%
otherFlag: %otherFlag%
END

facility "USER"; event_type 5005;
/*
 * Test of the use of one attribute as another attribute's dimension.
 * In this case, the dimension attribute is a member of a struct attribute
 * (and also a bit-field).
 */
attributes {
	string	msg;
	struct familyFlags f;
	string father[f.fatherFlag]	"(%s )";
	string mother[f.motherFlag]	"(%s )";
	string son[f.sonFlag]		"(%s )";
	string daughter[f.daughterFlag]	"(%s )";
	string cat[f.catFlag]		"(%s )";
	string dog[f.dogFlag]		"(%s )";
	string computer[f.computerFlag]	"(%s )";
	string others[f.otherFlag]	"(%s )";
}
format
%msg%
%f%
father: %father%
mother: %mother%
son: %son%
daughter: %daughter%
cat: %cat%
dog: %dog%
computer: %computer%
others: %others%
END

facility "USER"; event_type 5006;
/* Test of [*] as a synonym for [_R_] */
attributes {
	string msg;
	string extraMsgs[*]	"(%s\n)";
}
format
%msg%
%extraMsgs%
END

facility "USER";
event_type "Test of specifying event_type "
	"as string in template.";
attributes {
	string msg;
}
format
%msg%
If you see this line, we probably passed.
END
