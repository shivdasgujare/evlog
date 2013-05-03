struct _date;
aligned attributes {
	char	month;
	char	day;
	short	year;
}
format string "%month:02d%/%day:02d%/%year%"
END

typedef struct _date date_t;

struct _event;
aligned attributes {
	date_t	date;
	char	desc[32]	"%s";
}
format string "%date%: %desc%"
END

typedef struct _event event_t;

facility "USER";
event_type 4001;
attributes {
	event_t season[_R_]	"(%Z\n)";
}
format
OSU Football 2001
%season%
END

facility "USER";
event_type 4002;
attributes {
	string title;
	int ndates;
	date_t dates[ndates]	"(date[%I] = %Z\n)";
}
format
%title%
%dates%
END

struct prob_cause;
attributes {
	string cause;
	string suspect;
	double probability	"%.2f";
}
format
cause=%cause%; suspect=%suspect%; probability=%probability%
END

facility "USER";
event_type 4003;
const {
	struct prob_cause causes[] = {
		{ "Operator fell asleep", "operator", 0.3 },
		{ "Backhoe severed cable(s)", "utility company", 0.35 },
		{ "Coffee spilled on keyboard", "engineer", 0.15 },
		{ "Network down", "building maintenance", 0.2 }
	}	"(Probable cause[%I] = %Z\n)";
}
attributes {
	string problem;
}
format
problem = %problem%
likely causes:
%causes%
END
