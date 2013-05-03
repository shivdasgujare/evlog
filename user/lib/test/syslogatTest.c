#include <stdlib.h>
#include <syslog.h>
#include <assert.h>
#include "evlog.h"

#define EVL_FACILITY_NAME LOCAL7

void
game(const char *opponent, const char *where, int our_score, int their_score,
	const char *commentary)
{
	assert(our_score != their_score);	/* No ties allowed */
	if (our_score > their_score) {
		syslogat(LOG_INFO,
"OSU beat {opponent}%s in {where}%s, {us}%d-{them}%d\n{{commentary}%s",
			opponent, where, our_score, their_score, commentary);
	} else {
		syslogat(LOG_ERR,
"{opponent}%s beat OSU in {where}%s, {them}%d-{us}%d\n{{commentary}%s",
			opponent, where, their_score, our_score, commentary);
	}
}

main()
{
	/* syslog messages to LOCAL1, evlog to LOCAL7 */
	openlog("syslogatTest", 0, LOG_LOCAL1);
	game("Eastern Kentucky", "Corvallis", 49, 10, "");
	game("Temple", "Philadelphia", 35, 3, "");
	game("UNLV", "Corvallis", 47, 17, "");
	game("Fresno St", "Corvallis", 59, 19, "Wow!  How good is this team?");
	game("USC", "LA", 0, 22, "Not great, apparently.");
	game("UCLA", "Corvallis", 35, 43, "");
	game("Arizona St", "Tempe", 9, 13, "PAC-10 looking good this year");
	game("California", "Corvallis", 24, 13, "");
	game("Arizona", "Corvallis", 38, 3, "");
	game("Washington", "Seattle", 29, 41, "No mud in Husky Stadium");
	game("Stanford", "Palo Alto", 31, 21, "Looked grim at halftime");
	game("Oregon", "Corvallis", 45, 24, "Ducks' nightmare continues");
	exit(0);
}
