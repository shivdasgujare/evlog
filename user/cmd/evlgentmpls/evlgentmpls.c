/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2002
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
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 */

/*
 * Usage: evlgentmpls [-v version] tmpldir file1.o file2.o ...
 * This program extracts data from the .log section of each specified
 * object file, and produces a formatting template in tmpldir for each
 * event-logging call.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <bfd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <regex.h>

#include <posix_evlog.h>
#include <evlog.h>
#include <evl_util.h>
#include <evl_list.h>
#include <evl_template.h>

#define min(x,y) ((x) < (y) ? (x) : (y))

extern int evl_gen_event_type(const char *s1, const char *s2, const char *s3);
extern tmpl_base_type_t _evlGetTypeFromConversion(struct evl_parsed_format *pf,
	int promote, int signedOnly);

static const char *progname = NULL;
static int errors = 0;

struct log_meta_info {
	const struct evlog_info	*info;
	evl_list_t		*dups;
#define MINFO_UNIQUE	0
#define MINFO_1ST_DUP	1
#define MINFO_NTH_DUP	2
	int			dup_status;
	posix_log_facility_t	facility;
	int			event_type;
	char			*unbraced_format;
};

/*
Two printkat/syslogat calls that yield the same event type are
referred to here as "duplicates."  Since we can generate only one
template specification for each event type, we have to be somewhat
creative about generating a template that accounts for all the
calls in a set of duplicates.

In version 1 of our template-generation scheme, an event's type is
computed from the format string, file name, and function name.
Therefore, it is highly unlikely that two events in the same facility
will have the same event type but different file names or different
function names.  (We basically complain and give up if this happens.)
The only duplicates we deal with result from multiple calls of the
same type within the same function.  (Everything is the same except the
line number.)

In version 2, an event's type is computed just from the format string
(and from the unbraced format string, at that).  So it's possible that
duplicate events will come from different files or different functions.
 */
int version = 1;
/* This version of evlgentmpls can handle only versions 1-2. */
#define VERSION_MAX 2

static void
usage()
{
	fprintf(stderr, "Usage: %s [-v version] tmpldir file.o ...\n",
		progname);
	exit(1);
}

static int
compute_event_type(const struct log_meta_info *mif)
{
	switch (version) {
	case 1:
	    {
		const struct evlog_info *info = mif->info;
		return evl_gen_event_type(info->pos.file, info->pos.function,
			info->format);
	    }
	case 2:
		return evl_gen_event_type_v2(mif->unbraced_format);
	default:
		assert(0);
	}
	return EVL_INVALID_EVENT_TYPE;
}

static void
ensure_dir_exists(const char *path)
{
	struct stat st;
	if (stat(path, &st) == 0) {
		if (!S_ISDIR(st.st_mode)) {
			errno = ENOTDIR;
			goto badpath;
		}
	} else {
		if (errno != ENOENT) {
			goto badpath;
		}
		if (mkdir(path, 0755) != 0) {
			goto badpath;
		}
	}
	return;

badpath:
	fprintf(stderr, "%s: directory does not exist and cannot be created:\n",
		progname);
	perror(path);
	exit(1);
}

/*
 * Open the template source file for the facility named facname --
 * rootdir/cfacname/cfacname.t, where cfacname is the canonical version
 * of facname.  If necessary, create the file and the directory it goes in.
 * Return a pointer to the open file, or NULL on failure.
 */
static FILE *
open_tmpl_src_file(const char *rootdir, const char *facname)
{
	char path[PATH_MAX];
	char cfacname[PATH_MAX];
	FILE *f;

	(void) _evlGenCanonicalFacilityName(facname, cfacname);
	if (snprintf(path, PATH_MAX, "%s/%s", rootdir, cfacname) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		goto badpath;
	}
	ensure_dir_exists(path);

	if (snprintf(path, PATH_MAX, "%s/%s/%s.t", rootdir, cfacname, cfacname)
	    >= PATH_MAX) {
		errno = ENAMETOOLONG;
		goto badpath;
	}

	f = fopen(path, "a");
	if (!f) {
		goto badpath;
	}
	if (ftell(f) == 0L) {
		/* First time opening this file */
		posix_log_facility_t faccode;
		fprintf(f, "/*\n");
		fprintf(f, " * Automatically generated by %s\n", progname);
		if (posix_log_strtofac(facname, &faccode) != 0) {
			fprintf(f, " * Register facility with /sbin/evlfacility -a '%s' [-k]\n", facname);
		}
		fprintf(f, " * Compile with /sbin/evltc %s.t\n", cfacname);
		fprintf(f, " */\n\n");
		fprintf(f, "facility \"%s\";\n", facname);
	}

	return f;

badpath:
	fprintf(stderr, "%s: Cannot open template source file %s.t\n",
		progname, cfacname);
	perror(path);
	exit(1);
	/*NOTREACHED*/
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

/*
 * When a name is not explicitly specified for an attribute, we make up
 * a name based on the conversion character.  For example, given the format
 * string
 *	"Interesting values = %s, %ld, %d, %s, %x, and %llu\n"
 * the attributes will be named s1, d1, d2, s2, x1, and u1, respectively.
 */
static short name_index[128];

static void
reset_default_name_generator()
{
	memset(name_index, 0, sizeof(name_index));
}

static const char *
make_default_att_name(const struct evl_parsed_format *pf)
{
	static char name[20];
	char c = pf->fm_conversion;
	if (!isalpha(c)) {
		/* Shouldn't happen */
		c = '_';
	}
	name_index[c]++;
	sprintf(name, "%c%d", c, name_index[c]);
	return name;
}

/*
 * We call this so we get attribute lists like
 *	s1, first_name, last_name, s4, city, s6
 * rather than
 *	s1, first_name, last_name, s2, city, s3
 * when you have a mix of named and unnamed attributes.  (In this example,
 * all the attributes are strings.)
 */
static void
bump_default_att_name(const struct evl_parsed_format *pf)
{
	char c = pf->fm_conversion;
	if (!isalpha(c)) {
		/* Shouldn't happen */
		c = '_';
	}
	name_index[c]++;
}

static char *name_equals_regex_string = "[a-zA-Z_][a-zA-Z0-9_]*[ \t]*=[ \t]*$";
static regex_t name_equals_regex;
static regmatch_t regex_match;

static void
init_regex()
{
	regex_t *preg = &name_equals_regex;
	int status = regcomp(preg, name_equals_regex_string, REG_EXTENDED);
	if (status != 0) {
		char errbuf[100];
		(void) regerror(status, preg, errbuf, 100);
		fprintf(stderr, "%s: internal error: %s\n", progname, errbuf);
		exit(1);
	}
}

/*
 * If string s ends in "id=", where id is a legal C identifier and the '='
 * may be preceded and/or followed by spaces or tabs, insert a null at the
 * end of the identifier and return a pointer to the identifier.  Otherwise,
 * return NULL.
 */
static char *
collect_equals_name(char *s)
{
	char *c;
	if (regexec(&name_equals_regex, s, 1, &regex_match, 0) != 0) {
		return NULL;
	}
	c = s + regex_match.rm_so;
	assert(isalpha(*c) || *c == '_');
	for (c++; isalnum(*c) || *c == '_'; c++)
		;
	*c = '\0';
	return s + regex_match.rm_so;
}

/*
 * If string s ends in "{id}", where id is a legal C identifier, replace
 * the '}' with a null and return a pointer to the identifier.  Otherwise,
 * return NULL.
 */
static char *
collect_curly_name(char *s)
{
	char *lcb, *rcb, *c;
	int slen = strlen(s);

	if (slen < 3) {
		return NULL;
	}
	rcb = s + (slen - 1);
	if (*rcb != '}') {
		return NULL;
	}
	lcb = strrchr(s, '{');
	if (!lcb) {
		return NULL;
	}
	c = lcb + 1;
	if (!isalpha(*c) && *c != '_') {
		return NULL;
	}
	for (c++; c < rcb; c++) {
		if (!isalnum(*c) && *c != '_') {
			return NULL;
		}
	}
	*rcb = '\0';
	return lcb + 1;
}

static char *
collect_att_name(char *s)
{
	char *name = collect_curly_name(s);
	if (!name) {
		name = collect_equals_name(s);
	}
	return name;
}

static void
squeezeOutBracket(const char *fmt, char *squeezedFmt)
{
	const char *f;
	char *s;
	for (f = fmt, s = squeezedFmt; *f; f++) {
		if (*f != '[') {
			*s++ = *f;
		}
	}
	*s = '\0';
}

static void
emit_event_type(FILE *t, const struct log_meta_info *mif)
{
	switch (version) {
	case 1:
		fprintf(t, "event_type 0x%x;\n", mif->event_type);
		break;
	case 2:
		fprintf(t, "event_type \"%s\";\n",
			add_escapes(mif->unbraced_format));
		break;
	default:
		assert(0);
	}
}

/*
 * Emit
 *	string file = "pathname";
 * If there are dups from different files, do
 *	string file = "pathname1, pathname2, pathname3";
 * Can also handle function attribute.
 */
static void
emit_filefunc_att(FILE *t, const struct log_meta_info *mif, const char *att)
{
	const struct evlog_info *info;
	int file_att = (strcmp(att, "file") == 0);

	fprintf(t, "\tstring %s = \"", att);
	if (mif->dups) {
		const char *curr, *prev = NULL;
		evl_listnode_t *head = mif->dups, *end, *p;
		for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
			info = (const struct evlog_info *) p->li_data;
			curr = (file_att ? info->pos.file : info->pos.function);
			if (!prev || strcmp(prev, curr) != 0) {
				if (prev) {
					fprintf(t, ", ");
				}
				fprintf(t, "%s", curr);
				prev = curr;
			}
		}
	} else {
		info = mif->info;
		fprintf(t, "%s",
			(file_att ? info->pos.file : info->pos.function));
	}
	fprintf(t, "\";\n");
}

/*
 * Create the template source for the printkat() call whose meta-information
 * is pointed to by mif.
 */
static void
create_template(const char *rootdir, const struct log_meta_info *mif)
{
	evl_list_t *parsed_fmt = NULL;
	evl_listnode_t *head, *end, *p;
	const struct evlog_info *info = mif->info;
	const char *facname = info->facility;
	char *format_dup = strdup(info->format);	/* scribbleable copy */
	FILE *t;
	const char *att_name = NULL;
	int rec_supplies_line_number;
	int status;

	if (!strcmp(facname, "EVL_FACILITY_NAME")) {
		/*
		 * Programmer called syslogat() without defining
		 * EVL_FACILITY_NAME.
		 */
		facname = "user";
	}
	t = open_tmpl_src_file(rootdir, facname);
	reset_default_name_generator();

	// Just once, at start of file...
	// fprintf(t, "facility \"%s\";\n", facname);

	emit_event_type(t, mif);
	fprintf(t, "const {\n");
	if (mif->unbraced_format) {
		fprintf(t, "\tstring __fmt = \"%s\";\n",
			add_escapes(mif->unbraced_format));
	}
	emit_filefunc_att(t, mif, "file");
	emit_filefunc_att(t, mif, "function");

	rec_supplies_line_number = (strstr(info->format, "{line}%") != NULL);
	if (!rec_supplies_line_number) {
		fprintf(t, "\tint line = %d;\n", info->pos.line);
	}
	fprintf(t, "}\n");
	fprintf(t, "attributes {\n");
	fprintf(t, "\tstring fmt;\n");
	fprintf(t, "\tint __argsz;\n");

	parsed_fmt = _evlParsePrintfFormat(format_dup, 1, &status);
	if (status != 0) {
		fprintf(stderr,
"%s: Cannot parse format string: \"%s\"\n", progname, info->format);
		errors++;
		goto done;
	}
	head = parsed_fmt;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		evl_fmt_segment_t *fs = (evl_fmt_segment_t*) p->li_data;
		if (fs->fs_type == EVL_FS_STRING) {
			att_name = collect_att_name(fs->u.fs_string);
		} else {
			tmpl_base_type_t ty;
			struct evl_parsed_format *pf;
			assert(fs->fs_type == EVL_FS_PRINTFCONV);
			pf = fs->u.fs_conversion;
			if (att_name) {
				bump_default_att_name(pf);
			} else {
				att_name = make_default_att_name(pf);
			}
			if (pf->fm_array) {
				char fmt[20];
				assert(strlen(fs->fs_userfmt) < 20);
				squeezeOutBracket(fs->fs_userfmt, fmt);
				ty = _evlGetTypeFromConversion(pf, 0, 0);
				fprintf(t, "\tint __dim_%s;\n", att_name);
				fprintf(t, "\tstring __delim_%s;\n", att_name);
				fprintf(t, "\t%s %s[__dim_%s] \"%s\" delimiter=__delim_%s;\n",
					_evlTmplTypeInfo[ty].ti_name,
					att_name, att_name, fmt, att_name);
			} else {
				ty = _evlGetTypeFromConversion(pf, 1, 0);
				fprintf(t, "\t%s %s \"%s\";\n",
					_evlTmplTypeInfo[ty].ti_name,
					att_name, fs->fs_userfmt);
			}
			att_name = NULL;
		}
	}
	fprintf(t, "}\n");
	fprintf(t, "format\n");
	fprintf(t, "%%file%%:%%function%%:%%line%%");
	if (mif->dups && !rec_supplies_line_number) {
		/* Tack on the dups' line numbers. */
		evl_listnode_t *head = mif->dups, *p;
		for (p = head->li_next; p != head; p = p->li_next) {
			const struct evlog_info *dup =
				(const struct evlog_info *) p->li_data;
			fprintf(t, ", %d", dup->pos.line);
		}
	}
	fprintf(t, "\n");
	fprintf(t, "%%fmt:printk%%\n");
	fprintf(t, "END\n");

done:
	if (parsed_fmt) {
		_evlFreeParsedFormat(parsed_fmt);
	}
	free(format_dup);
	(void) fclose(t);
}

/*
 * The indicated objects yield the same event type.  Are they really the
 * same event?  yes=1, no=0
 */
static int
same_event(const struct log_meta_info *mif1, const struct log_meta_info *mif2)
{
	switch (version) {
	case 1:
	    {
		const struct evlog_info *info1, *info2;
		info1 = mif1->info;
		info2 = mif2->info;
		return (strcmp(info1->format, info2->format) == 0
		    && strcmp(info1->pos.file, info2->pos.file) == 0
		    && strcmp(info1->pos.function, info2->pos.function) == 0);
	    }
	case 2:
		return (!strcmp(mif1->unbraced_format, mif2->unbraced_format));
	default:
		assert(0);
	}
	/*NOTREACHED*/
}

/* Return 1 for a match, 0 for no match. */
static int
duplicate_info(const struct log_meta_info *mif1,
	const struct log_meta_info *mif2)
{
	if (mif1->event_type == mif2->event_type
	    && mif1->facility == mif2->facility) {
		/* We have a match.  Make sure it's valid. */
		const struct evlog_info *info1, *info2;
		info1 = mif1->info;
		info2 = mif2->info;
		if (! same_event(mif1, mif2)) {
			fprintf(stderr,
"%s: CRC collision: Two different events yield the same event type:\n"
"%s:%s:%d - %s\n"
"%s:%s:%d - %s\n",
				progname,
				info1->pos.file, info1->pos.function,
				info1->pos.line, info1->format,
				info2->pos.file, info2->pos.function,
				info2->pos.line, info2->format);
			errors++;
		}
		return 1;
	}
	return 0;
}

/*
 * Given 2 evlog_info objects, return < 0 if info1 sorts before info2,
 * 0, if they sort the same, or > 0 otherwise.
 */
static int
compare_info(const struct evlog_info *info1, const struct evlog_info *info2)
{
	int ret = 0;
	switch (version) {
	case 1:
		ret = info1->pos.line - info2->pos.line;
		break;
	case 2:
		ret = strcmp(info1->pos.file, info2->pos.file);
		if (ret == 0) {
			ret = strcmp(info1->pos.function, info2->pos.function);
			if (ret == 0) {
				ret = info1->pos.line - info2->pos.line;
			}
		}
		break;
	default:
		assert(0);
	}
	return ret;
}

/*
 * Create a list node for info and insert it into the list.  Nodes are
 * sorted according to compare_info().
 */
static evl_list_t *
add_dup(evl_listnode_t *head, const struct evlog_info *info)
{
	evl_listnode_t *end, *p;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		const struct evlog_info *pinfo =
			(const struct evlog_info*) p->li_data;
		if (compare_info(info, pinfo) < 0) {
			return _evlInsertToList((void*)info, p, head);
		}
	}
	return _evlAppendToList(head, info);
}

/*
 * Copy src to dest, replacing strings of the form "{id}%" with "%".
 * If src contains "{{", strip out that and anything beyond it.
 * dest is a buffer of size bufsz.  Make sure we don't overflow it.
 */
static void
unbrace(char *dest, const char *src, int bufsz)
{
	const char *copy_this = src, *scan_this = src;
	const char *c, *lcb, *rcb, *cut_here;
	int copy_len, dlen = 0;

	cut_here = strstr(src, "{{");
	dest[0] = '\0';
	for (;;) {
		lcb = strchr(scan_this, '{');
		if (!lcb) {
			goto done;
		}
		rcb = strstr(lcb+2, "}%");
		if (!rcb) {
			goto done;
		}
		if (cut_here && cut_here < rcb) {
			goto done;
		}
		/* Is it a valid identifier between the { and } ? */
		c = lcb+1;
		if (*c != '_' && !isalpha(*c)) {
			goto scan_again;
		}
		for (c++; c < rcb; c++) {
			if (*c != '_' && !isalnum(*c)) {
				goto scan_again;
			}
		}
		copy_len = min(lcb - copy_this, bufsz-(dlen+1));
		strncat(dest + dlen, copy_this, copy_len);
		dlen += copy_len;
		copy_this = rcb+1;
scan_again:
		scan_this = rcb+2;
	}
done:
	if (cut_here) {
		copy_len = min(cut_here - copy_this, bufsz-(dlen+1));
	} else {
		copy_len = bufsz-(dlen+1);
	}
	strncat(dest + dlen, copy_this, copy_len);
	return;
}

/* If s ends in a newline, return a pointer to that newline; else NULL. */
static const char *
terminating_newline(const char *s)
{
	const char *last;
	int slen = (int) strlen(s);
	if (slen == 0) {
		return NULL;
	}
	last = s + slen-1;
	return (*last == '\n' ? last : NULL);
}

/*
 * For version 2 and beyond, strip out the stuff that doesn't count
 * in computing event_type.
 */
static char *
unbrace_fmt(const char *fmt)
{
	int has_severity = (fmt[0] == '<' && isdigit(fmt[1]) && fmt[2] == '>');
	if (has_severity || terminating_newline(fmt)
	    || strstr(fmt, "}%") || strstr(fmt, "{{")) {
		char *nl;
		int bufsz = strlen(fmt) + 1;
		char *buf = (char*) malloc(bufsz);
		assert(buf != NULL);
		if (has_severity) {
			fmt += 3;
		}
		unbrace(buf, fmt, bufsz);
		/* Recheck, in case of "...\n{{...\n" or some such */
		nl = (char*) terminating_newline(buf);
		if (nl) {
			*nl = '\0';
		}
		return buf;
	}
	/* Nothing to strip out. */
	return (char*) fmt;
}

/*
 * info is the array of evlog_info objects extracted from the .o file's .log
 * section.  Create and return mif, an array of meta-info objects, such
 * that mif[i].info = &info[i].  If two or more objects are dups of each
 * other (i.e., they yield the same facility and event type) then we
 * annotate their mif elements so that only one template is generated.
 * If info[i] is the "first" in a group of dups (e.g., the one with the
 * lowest line number), then mif[i].dups is a linked list of all the dups,
 * including i.
 */
static struct log_meta_info *
prepare_meta_info(const struct evlog_info *info, int nel)
{
	int i, j, status;
	const struct evlog_info *infoi;
	struct log_meta_info *mi, *mj, *mif = (struct log_meta_info *)
		malloc(nel * sizeof(struct log_meta_info));
	assert(mif != NULL);
	for (i = 0; i < nel; i++) {
		infoi = info + i;
		mi = mif + i;
		mi->info = infoi;
		mi->dups = NULL;
		mi->dup_status = MINFO_UNIQUE;
		if (version == 1) {
			mi->unbraced_format = NULL;
		} else {
			mi->unbraced_format = unbrace_fmt(infoi->format);
		}
		mi->event_type = compute_event_type(mi);
		status = evl_gen_facility_code(infoi->facility,
			&(mi->facility));
		if (status != 0) {
			fprintf(stderr,
"%s: Cannot generate valid facility code for facility \"%s\";\n"
"using LOG_KERN.\n",
				progname, infoi->facility);
			perror(infoi->facility);
			mi->facility = LOG_KERN;
			errors++;
		}
		for (j = 0; j < i; j++) {
			mj = mif + j;
			if (mj->dup_status == MINFO_NTH_DUP) {
				continue;
			}
			if (duplicate_info(mif + i, mif + j)) {
				/* i is a dup of j. */
				evl_list_t *dups = mj->dups;
				const struct evlog_info *first_dup;
				if (!dups) {
					dups = _evlMkListNode(info + j);
				}
				dups = add_dup(dups, infoi);
				first_dup = (const struct evlog_info *)
					dups->li_data;
				if (first_dup == infoi) {
					/* i replaces j as first dup */
					mi->dups = dups;
					mi->dup_status = MINFO_1ST_DUP;

					mj->dups = NULL;
					mj->dup_status = MINFO_NTH_DUP;
				} else {
					mi->dup_status = MINFO_NTH_DUP;

					/* In case j was previously UNIQUE... */
					mj->dups = dups;
					mj->dup_status = MINFO_1ST_DUP;
				}
				/* No need to search for another dup of i */
				break;
			}
		}
	}
	return mif;
}

static void
free_meta_info(struct log_meta_info *mif, int nel)
{
	int i;
	struct log_meta_info *mi;
	for (i = 0; i < nel; i++) {
		mi = mif + i;
		if (mi->dups) {
			_evlFreeList(mi->dups, 0);
		}
		if (mi->unbraced_format
		    && mi->unbraced_format != mi->info->format) {
		 	free(mi->unbraced_format);
		 }
	}
	free(mif);
}

#ifdef DEBUG
static void
dump_details(const struct evlog_info *info, int nel)
{
	const struct evlog_info *in, *end = info + nel;
	for (in = info; in < end; in++) {
		printf("%p %s\t%s\t%s:%s:%d\n",
			in, in->format, in->facility,
			in->pos.file, in->pos.function, in->pos.line);
	}
}
#endif

static void
do_section(bfd *bfd, asection *log, void *tsd)
{
	bfd_size_type size;
	size_t number;
	struct evlog_info *info;
	int i;
	struct log_meta_info *mif;

	/* Not log section? */
	if (strcmp(log->name, ".log") != 0)
		return;

	size = bfd_section_size(bfd, log);
	number = size / sizeof(struct evlog_info);
	info = (struct evlog_info*) malloc(size + sizeof(struct evlog_info));

	if (!bfd_get_section_contents(bfd, log, info, 0, size)) {
		bfd_perror("Getting section contents");
		exit(1);
	}

#ifdef DEBUG
	dump_details(info, number);
#endif

	mif = prepare_meta_info(info, number);

	for (i = 0; i < number; i++) {
		if (mif[i].dup_status == MINFO_NTH_DUP) {
			/* The first in our dup group prints the template. */
			continue;
		}
		create_template(tsd, mif + i);
	}
	free_meta_info(mif, number);
	free(info);
}

int
main(int argc, char *argv[])
{
	int i, c;
	const char *tsd;	/* template source directory */
	extern char *optarg;
	extern int optind;

	progname = argv[0];
	while ((c = getopt(argc, argv, "v:")) != -1) {
		switch (c) {
		case 'v':
			version = atoi(optarg);
			if (version < 1 || version > VERSION_MAX) {
				fprintf(stderr, "%s: unknown version: %d\n",
					progname, version);
				exit(1);
			}
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	if (argc < optind + 2) {
		usage();
	}

	init_regex();
	bfd_init();
	i = optind;
	tsd = argv[i++];
	ensure_dir_exists(tsd);

	for (; i < argc; i++) {
		bfd *obj;

		obj = bfd_openr(argv[i], NULL);
		if (bfd_get_error()) {
			bfd_perror("Opening file");
			exit(1);
		}
		/* You have to check format before using it.  Sigh. */
		if (bfd_check_format(obj, bfd_archive)) {
			bfd *i, *nexti;

			for (i = bfd_openr_next_archived_file(obj, NULL);
			    i;
			    i = nexti) {
				bfd_map_over_sections(i, do_section, (void*) tsd);
				nexti = bfd_openr_next_archived_file(obj, i);
				bfd_close(i);
				bfd_set_error(bfd_error_no_error);
			}
		} else if (bfd_check_format(obj, bfd_object)) {
			bfd_map_over_sections(obj, do_section, (void*) tsd);
		} else {
			bfd_perror("Identifying file");
			exit(1);
		}
		bfd_close(obj);
		/* BFD's error caching is screwed.  Grrr. */
		bfd_set_error(bfd_error_no_error);
	}
	exit (errors ? 1 : 0);
}
