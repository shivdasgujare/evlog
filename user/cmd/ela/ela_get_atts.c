/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2002
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Please send e-mail to nguyhien@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <evl_template.h>

static const char *progName;
static const char *facility;
static const char *drivername=NULL;
static const char *spec_att_name=NULL;
static const char *real_evt_type=NULL;
static int verbose = 0;
static int rule_created_for_real_event=0;

static void report(template_t *t);

static void usage()
{
	fprintf(stderr, "usage: %s -f facility [-d drivername] [-a att_name] [-e real event type]\n", progName);
	exit(1);
}

main(int argc, char **argv)
{
	struct dirent *dirent;
	DIR *dir;
	auto int c;

	progName = argv[0];

	if (argc < 2) {
		usage();
	}
	while ((c = getopt(argc, argv, "vf:a:e:d:")) != EOF) {
		switch (c) {
			case 'f':
				facility = optarg;
				break;
			case 'a':
				spec_att_name = optarg;
				break;
			case 'e':
				real_evt_type = optarg;
				break;
			case 'd':
				drivername = optarg;
				break;
			case 'v':
				verbose=1;
			case 'h':
				usage();
				break;
			default:
				usage();
				break;
		}
	}
	

	dir = opendir(".");
	if (!dir) {
		perror(".");
		exit(1);
	}
	while ((dirent = readdir(dir)) != NULL) {
		template_t *t;
		const char *fname = dirent->d_name;
		if (!_evlStartsWith(fname, "0x")
		    || !_evlEndsWith(fname, ".to")) {
			continue;
		}
		t = _evlReadTemplate(fname);
		if (!t) {
			/* _evlReadTemplate has reported the problem. */
			continue;
		}
		report(t);
	}
	(void) closedir(dir);
	return 0;
}

static evlattribute_t *
getAtt(template_t *t, const char *attName)
{
	evlattribute_t *att;
	int status = evltemplate_getatt(t, attName, &att);
	if (status != 0) {
		if (verbose) {
			if (!strcmp(attName, "forany") || !strcmp(attName, "eventName")) {
	 			return NULL;
	 		}
			fprintf(stderr,
			"%s: Can't find attribute %s for event type %#x\n",
			progName, attName, t->tm_header.u.u_evl.evl_event_type);
		}
		return NULL;
	}
	return att;
}

/*
 * Copy s to a static buffer, replacing special characters with 2-character
 * escapes.  Return the buffer's address.
 */
static char *
add_escapes(const char *s)
{
	static char s_with_escapes[512];
	char *swe;
	for (swe = s_with_escapes; *s; s++, swe++) {
		switch (*s) {
		case '\\':	*swe++ = '\\'; *swe = '\\'; break;
		case '"':	*swe++ = '\\'; *swe = '"'; break;
		case '\a':	*swe++ = '\\'; *swe = 'a'; break;
		case '\b':	*swe++ = '\\'; *swe = 'b'; break;
		case '\f':	*swe++ = '\\'; *swe = 'f'; break;
		case '\n':	*swe++ = '\\'; *swe = 'n'; break;
		case '\r':	*swe++ = '\\'; *swe = 'r'; break;
		case '\t':	*swe++ = '\\'; *swe = 't'; break;
		case '\v':	*swe++ = '\\'; *swe = 'v'; break;
		default:	*swe = *s; break;
		}
	}
	*swe = '\0';
	return s_with_escapes;
}

static void
report(template_t *t)
{
	evlattribute_t *att, att2;
	tmpl_base_type_t att_type;
	char rule_name[256];
	char att_value[80];
	int threshold = 0;
	
	/* Only generate rule if it has reportType attribute */
	att = getAtt(t, "reportType");
	if (!att) {
		return;
	}

	att = getAtt(t, "eventName");
	if (!att) {
		return;
	}
	
	if (drivername) {
		sprintf(rule_name, "%s_%s", drivername, evl_getStringAttVal(att));
	}
	else {	
		sprintf(rule_name, "%s_%s", facility, evl_getStringAttVal(att));
	}
	
	
	att = getAtt(t, "threshold");
	if (att) {
		threshold = (int) evl_getLongAttVal(att);
	} 
	else {
		threshold =1;
	}
		
	if (threshold <= 1 && rule_created_for_real_event == 1) {
		return;
	}
	
	printf("%s {\n", rule_name);	

	if (spec_att_name == NULL) {
		printf("\tfilter = 'facility=\"%s\" && event_type=%#x'\n",
			facility, t->tm_header.u.u_evl.evl_event_type);
	}
	else if (spec_att_name) {
		/*
		 * If the special attribute is not NULL and threshold > 1,
		 * that means we need to use this attribute in the query
		 * statement
		 */
		if (threshold > 1) {
			att = getAtt(t, spec_att_name);
			if (att) {
				att_type = att->ta_type->tt_base_type;
				switch(att_type) {
				case TY_STRING:
					printf("\tfilter = 'facility=\"%s\" && %s=\"%s\"'\n",
						facility, spec_att_name, 
						add_escapes(t->tm_header.th_description));
					break;
				case TY_INT:
				case TY_LONG:
					printf("\tfilter = 'facility=\"%s\" && %s=%#x'\n",
						facility, spec_att_name,
						t->tm_header.u.u_evl.evl_event_type);
			
					break;
				default:
					break;
				}
			} 
		}
		else if (real_evt_type) {
			printf("\tfilter = 'facility=\"%s\" && event_type=%s && !(threshold > 1)'\n",
			facility, real_evt_type);
			rule_created_for_real_event = 1;
		} 
		else {
			printf("\tfilter = 'facility=\"%s\" && %s  && !(threshold > 1)'\n",
			facility, spec_att_name);
			rule_created_for_real_event = 1;
		}
			
	} 

	if (threshold > 0) {
		printf("\tthreshold = %d\n", threshold);
	}

	att = getAtt(t, "interval");
	if (att) {
		if (att->ta_type->tt_base_type == TY_STRING) {
			printf("\tinterval = %s\n", evl_getStringAttVal(att));
		} 
		else {
			printf("\tinterval = %d\n", (int) evl_getLongAttVal(att));
		}
	}
	else {
		printf("\tinterval = -1\n");
	}
	
	att = getAtt(t, "forany");
	if (att) {
		printf("\tforany = \"%s\"\n", evl_getStringAttVal(att));
	}
	printf("}\n");
}
