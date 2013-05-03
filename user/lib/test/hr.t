/* Just a bunch of structs to be included in templates/hr */
struct date;
aligned attributes {
	char	month;
	char	day;
	short	year;
}
format string "%month%/%day%/%year%"
END

struct usAddress;
attributes {
	string	street;
	string	city;
	string	state;
	string	zip;
}
format
%street%
%city%, %state% %zip%
USA
END

struct employee;
attributes {
	string			name;
	struct usAddress	homeAddress;
	string			title;
	double			salary		"%.2f";
	struct date		startDate;
}
format
Employee Name:	%name%
Home Address:
%homeAddress%
Job Title:	%title%
Annual Salary:	%salary%
Start Date (m/d/y):	%startDate%
END
