struct ela_entry;
attributes {
	string msg;
	string eventName;
	string file;
	ElaClass elaClass;
	ElaType elaType;
	ElaStringList probableCauses;
	ElaStringList actions;
	int threshold;
	string interval;
}
format string
"Message = %msg%\n"
"EventName = %eventName%\n"
"File = %file%\n"
"Class = %elaClass%\n"
"Type = %elaType%\n"
"ProbableCauses = %probableCauses%\n"
"Actions = %actions%\n"
"Threshold = %threshold%\n"
"Interval = %interval%"
END
