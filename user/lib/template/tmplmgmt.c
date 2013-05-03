/*
 * Linux Event Logging for the Enterprise
 * Copyright (C) International Business Machines Corp., 2001
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>

#include "config.h"
#include "evl_common.h"
#include "evl_template.h"

/*
 * Template management:
 * The code in this file manages the following functions:
 * - Compilation of a template source file
 * - Importing of templates (including the algorithm for finding the
 *	template's binary file)
 * - Storage and retrieval of templates to/from hash tables
 */

#ifdef _POSIX_THREADS
static pthread_mutex_t tmplPathMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t tmplCacheMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

extern void _evlTmplReinitLex(const char *parseMe);
extern void _evlTmplEndLex();

int _evlTmplMgmtFlags = 0;

/* The name of the environment variable that lists paths to template dirs */
#define TMPL_PATH_VAR "EVLTMPLPATH"
/* The default directory for templates */
#define TMPL_PATH_DEFAULT LOG_EVLOG_DIR "/templates"

/* The list of templates we've created from the current source file. */
static evl_list_t *templateList = NULL;

/*
 * The list of struct refs we've created due to explicit or implicit import
 * requests.  This also contains an entry for each struct defines so far in
 * the current source file.  This is a list of tmpl_struct_ref_t.
 */
static evl_list_t *imports = NULL;

static char *sourceDir = NULL;	/* where source file resides */

/* Useful abbreviations */
#define getFacility(t) (t)->tm_header.u.u_evl.evl_facility
#define getEventType(t) (t)->tm_header.u.u_evl.evl_event_type

/* Forward refs */
static void forgetStructRefs();
static void rememberStructRefs(template_t *t);
static template_t *_evlImportTemplateFromPaths(const char *dir,
	const char *structPath, int purpose);
static void registerImport(template_t *t, const char *structPath, int purpose);

/***** Begin template hashing functions *****/

/*
 * ht_table is an array of pointers to lists.  The number of slots (lists) is
 * ht_size.  Each list contains zero or more templates.  (The pointer
 * to a zero-length list is simply NULL.)  Within each list, all templates
 * have the same hash value.
 *
 * The hash value for a struct template is determined by the struct's name.
 * There can be multiple struct templates with the same name.  Within a
 * list, templates are sorted by name, so all templates with the same name
 * will be together.
 *
 * Event-record templates are handled differently.  They are hashed by
 * (facility code + event type).  The object actually hashed is a FET
 * (fet_t), which contains the address of the template.  To handle
 * the common case of an event type that occurs frequently but has
 * no corresponding template (or at best an uninteresting template),
 * we keep a FET object for every event-record template we look up.
 * That way we don't keeping going out to the filesystem looking for the
 * same nonexistent or uninteresting template.
 */
typedef struct tmpl_hashtable {
	evl_list_t	**ht_table;
	size_t		ht_size;
} tmpl_hashtable_t;

/* TODO: Make sizes of template hash tables dynamic. */
#define HASHSZ 101
static evl_list_t *structHashArray[HASHSZ];
static evl_list_t *evrecHashArray[HASHSZ];

static tmpl_hashtable_t structHash = {structHashArray, HASHSZ};
static tmpl_hashtable_t evrecHash = {evrecHashArray, HASHSZ};

/*
 * This is the data structure stored in evrecHash.  The template member
 * may be null if there is no template for the specified facility and
 * event type, or if that template is not interesting (e.g., because it
 * doesn't contain the attributes named in a query or format specification).
 *
 * A facility may have a default template.  For such a facility, the FET
 * for event type EVL_DEFAULT_EVENT_TYPE contains a pointer to the template,
 * and the FET for any event type that doesn't have its own template
 * contains a pointer (fet_default) to the EVL_DEFAULT_EVENT_TYPE FET.
 */
typedef struct fet {
	posix_log_facility_t	fet_facility;
	int			fet_eventType;
	template_t		*fet_template;
	struct fet		*fet_default;
	int			fet_flags;
} fet_t;

#define FET_NOTTHERE 1

static fet_t *findFETInHash(posix_log_facility_t fac, int eventType,
	tmpl_hashtable_t *table);

static fet_t *
makeFET(posix_log_facility_t fac, int eventType)
{
	fet_t *fet = (fet_t*) malloc(sizeof(fet_t));
	assert(fet != NULL);
	fet->fet_facility = fac;
	fet->fet_eventType = eventType;
	fet->fet_template = NULL;
	fet->fet_default = NULL;
	fet->fet_flags = 0;
	return fet;
}

static size_t
computeHash(const char *name, int hashsz)
{
	size_t hash = 0;
	const char *c;
	for (c = name; *c; c++) {
		hash <<= 1;
		hash += *c;
	}
	return (hash % hashsz);
}

/* FET = Facility and Event Type */
static size_t
computeHashByFET(posix_log_facility_t fac, int evtype, int hashsz)
{
#define ABS(n) ((n) < 0 ? -(n) : (n))
	int n = fac + evtype;
	return (ABS(n) % hashsz);
}

/*
 * head is the head of the (possibly NULL) list in the name hash where
 * template t should be inserted.  Insert it.
 */
static evl_list_t *
insertTemplateIntoHashList(evl_list_t *head, template_t *t)
{
	evl_listnode_t *p, *end;

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		template_t *pt = (template_t*) p->li_data;
		if (strcmp(t->tm_name, pt->tm_name) <= 0) {
			return _evlInsertToList(t, p, head);
		}
	}

	return _evlAppendToList(head, t);
}

static evl_list_t *
insertFETIntoHashList(evl_list_t *head, fet_t *fet)
{
	evl_listnode_t *p, *end;
	int et = fet->fet_eventType;

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		fet_t *pfet = (fet_t*) p->li_data;
		if (et <= pfet->fet_eventType) {
			return _evlInsertToList(fet, p, head);
		}
	}

	return _evlAppendToList(head, fet);
}

static void
addTemplateToHash(template_t *t, tmpl_hashtable_t *table)
{
	size_t slot = computeHash(t->tm_name, table->ht_size);
	table->ht_table[slot] = insertTemplateIntoHashList(
		table->ht_table[slot], t);
}

static void
addFETToHash(fet_t *fet, tmpl_hashtable_t *table)
{
	size_t slot = computeHashByFET(fet->fet_facility, fet->fet_eventType,
		table->ht_size);
	table->ht_table[slot] = insertFETIntoHashList(table->ht_table[slot],
		fet);
}

static void
addTemplateToFETHash(template_t *t, tmpl_hashtable_t *table)
{
	fet_t *fet;
	fet = findFETInHash(getFacility(t), getEventType(t), table);
	if (fet) {
		/*
		 * There's already a FET for this facility & event type
		 * in the hash.
		 */
		assert(fet->fet_template == NULL);
		fet->fet_template = t;
		fet->fet_default = NULL;
	} else {
		fet = makeFET(getFacility(t), getEventType(t));
		fet->fet_template = t;
		fet->fet_default = NULL;
		addFETToHash(fet, table);
	}
}

/*
 * Find a template with the indicated name in the indicated hash table.
 * If st is not NULL, then it's the stat structure for a binary template
 * file, and the dev/ino must match as well.
 */
static template_t *
findTemplateInHash(const char *name, const struct stat *st,
	tmpl_hashtable_t *table)
{
	evl_listnode_t *head, *p, *end;
	size_t slot = computeHash(name, table->ht_size);
	head = table->ht_table[slot];
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		template_t *pt = (template_t*) p->li_data;
		int cmp = strcmp(name, pt->tm_name);
		if (cmp > 0) {
			/* We're past where name would show up in list. */
			return NULL;
		} else if (cmp == 0) {
			/*
			 * Name matches.  If necessary, verify that it's also
			 * the same template file.
			 */
			if (!st) {
				return pt;
			} else if (st->st_dev == pt->tm_context.tc_device
			    && st->st_ino == pt->tm_context.tc_inode) {
				return pt;
			}
		}
		/* No match; keep looking. */
	}
	return NULL;
}

/*
 * Find a fet with the indicated facility and event type in the
 * indicated hash table.
 */
static fet_t *
findFETInHash(posix_log_facility_t fac, int eventType, tmpl_hashtable_t *table)
{
	evl_listnode_t *head, *p, *end;
	size_t slot = computeHashByFET(fac, eventType, table->ht_size);
	head = table->ht_table[slot];
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		fet_t *pfet = (fet_t*) p->li_data;
		int cmp = pfet->fet_eventType - eventType;
		if (cmp > 0) {
			/* We're past where eventType would show up in list. */
			return NULL;
		} else if (cmp == 0) {
			/*
			 * Event type matches.  Verify that the facility code
			 * also matches.
			 */
			if (pfet->fet_facility == fac) {
				return pfet;
			}
		}
		/* No match; keep looking. */
	}
	return NULL;
}

/*
 * Fake up a name for an event-record template.  This was originally necessary
 * because the hash-lookup algorithms were based on the tm_name member for
 * event-record templates.
 */
void
_evlTmplMakeEvlogName(posix_log_facility_t fac, int eventType, char *buf, size_t size)
{
	snprintf(buf, size, "%d@%u", eventType, fac);
}

/***** Begin high-level functions for compiling templates *****/

/* Like perror, but write to pc->pc_errfile instead */
static void
pcperror(tmpl_parser_context_t *pc, const char *msg)
{
	fprintf(pc->pc_errfile, "%s: %s\n", msg, strerror(errno));
}

/*
 * Change x/y/z to x.y.z -- e.g., for an error message.  Result must be
 * freed by caller.
 */
static char *
makeDotPathFromSlashPath(const char *slashPath)
{
	char *c;
	char *dotPath = strdup(slashPath);
	assert(dotPath != 0);
	for (c = dotPath; *c; c++) {
		if (*c == '/') {
			*c = '.';
		}
	}
	return dotPath;
}

/*
 * Read the contents of the file into fileImage, then break it up into
 * template images.  Template images are separated by END "statements".
 * Return a list of pointers to the template images, or NULL on error.
 * progName is argv[0], used for error messages.
 */
static evl_list_t *
getTemplateImagesFromFile(tmpl_parser_context_t *pc)
{
	int fd;
	struct stat st;
	int nBytes;
	char *fileImage, *tim, *endLine;
	evl_list_t *list = NULL;
	const char *path = pc->pc_pathname;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(pc->pc_errfile, "%s: Cannot open template file\n",
			pc->pc_progname);
		pcperror(pc, path);
		return NULL;
	}
	if (fstat(fd, &st) != 0) {
		fprintf(pc->pc_errfile, "%s: Cannot stat template file\n",
			pc->pc_progname);
		pcperror(pc, path);
		return NULL;
	}
	fileImage = (char *) malloc((size_t) st.st_size + 1);
	assert(fileImage != NULL);
	nBytes = read(fd, fileImage, st.st_size);
	if (nBytes < 0) {
		fprintf(pc->pc_errfile, "%s: Cannot read template file\n",
			pc->pc_progname);
		pcperror(pc, path);
		return NULL;
	}
	close(fd);

	if (nBytes != st.st_size) {
		fprintf(pc->pc_errfile,
"%s: Expected to read %ld bytes from %s, but read %d\n",
			pc->pc_progname, (long)st.st_size, path, nBytes);
		return NULL;
	}
	fileImage[st.st_size] = '\0';
	if (strlen(fileImage) != st.st_size) {
		fprintf(pc->pc_errfile, "%s: %s is not a text file\n",
			pc->pc_progname, path);
		return NULL;
	}

	/* fileImage contains the file image, terminated with a null. */
	tim = fileImage;
	for (;;) {
		endLine = strstr(tim, "\nEND\n");
		if (!endLine) {
			list = _evlAppendToList(list, tim);
			break;
		}

		/*
		 * Found an END line.  Keep the newline before the END
		 * because that's part of tim's template.  Keep the newline
		 * following END (as part of the next template) to keep
		 * line numbers in sync.
		 */
		endLine[1] = '\0';
		list = _evlAppendToList(list, tim);
		tim = endLine+4;
		if (!strcmp(tim, "\n")) {
			/* File ends in (unnecessary) END line. */
			break;
		}
	}

	return list;
}

/*
 * Does what dirname(1) does.  The returned string is malloced.
 */
char *
_evlGetParentDir(const char *path)
{
	const char *lastSlash = strrchr(path, '/');
	if (!lastSlash) {
		/* E.g., "xyz" */
		return strdup(".");
	} else if (lastSlash == path) {
		/* E.g., "/xyz" */
		return strdup("/");
	} else {
		char *parentDir = strdup(path);
		parentDir[lastSlash-path] = '\0';
		return parentDir;
	}
}

char *
_evlTmplGetSourceDir()
{
	return sourceDir;
}

/*
 * Populate templateList with the templates from the text file whose name
 * is path.  Templates are also stored in their respective hash tables.
 */
static evl_list_t *
getTemplatesFromFile(const char *path, FILE *errfile, const char *progName,
	int *errors)
{
	evl_list_t *timList;	/* template images (source code) */
	evl_listnode_t *head, *p, *end;
	char *tim;
	template_t *template;
	tmpl_parser_context_t *pc = _evlTmplGetParserContext();
	int status;

	pc->pc_errfile = errfile;
	pc->pc_pathname = strdup(path);
	pc->pc_lineno = 1;
	pc->pc_progname = progName;

	timList = getTemplateImagesFromFile(pc);
	if (!timList) {
		return NULL;
	}

	/*
	 * Set the "primary directory" in case we have to search for
	 * binary template files.
	 */
	sourceDir = _evlGetParentDir(path);

	*errors = 0;
	head = timList;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		tim = (char*) p->li_data;
		forgetStructRefs();
		_evlTmplClearParserErrors();
		pc->pc_template = NULL;
		pc->pc_dfltdesc = NULL;
		_evlTmplReinitLex(tim);
		status = ttparse();
		_evlTmplEndLex();
		template = pc->pc_template;

		assert(template != NULL || status != 0);
		if (status != 0 || (template->tm_flags & TMPL_TF_ERROR) != 0) {
			(*errors)++;
			if (template) {
				_evlFreeTemplate(template);
			}
			continue;
		}
		rememberStructRefs(template);

		if (template->tm_header.th_type == TMPL_TH_STRUCT) {
			addTemplateToHash(template, &structHash);
			registerImport(template, template->tm_name,
				TMPL_IMPORT_EXPLICIT);
		}
		templateList = _evlAppendToList(templateList, template);
	}

	return templateList;
}

/*
 * Create the basename for a file that contains a template with the
 * indicated facility and event type.  Just use the hex representation
 * of the event type, ignoring facility.  Exception: event type = -1
 * yields "default".
 */
static void
makeEvlogTmplName(posix_log_facility_t fac, int eventType, char *buf, size_t size)
{
	if (eventType == EVL_DEFAULT_EVENT_TYPE) {
		(void) strcpy(buf, "default");
	} else {
		snprintf(buf, size, "0x%x", eventType);
	}
}

/*
 * Write template t to a binary file whose pathname is dir/tmplName.to,
 * where tmplName is the appropriate name for the template.  Returns 0
 * on success, or one of the following:
 * 	ENAMETOOLONG: dir/tmplName.to would be too long.
 *	EPERM: _evlWriteTemplate() failed.
 */
static int
writeTemplateToFile(const template_t *t, const char *dir)
{
	char path[PATH_MAX];
	size_t dirLen = strlen(dir);
	char *fileName;		/* Points the end of "dir/" */
	const char *tmplName;
	char evlogTmplName[100];

	if (!t) {
		return EINVAL;
	}
	if (dirLen + 2 > PATH_MAX) {
		return ENAMETOOLONG;
	}
	(void) strcpy(path, dir);
	path[dirLen] = '/';
	fileName = path + dirLen + 1;

	if (t->tm_header.th_type == TMPL_TH_STRUCT) {
		tmplName = t->tm_name;
	} else {
		makeEvlogTmplName(getFacility(t), getEventType(t),
			evlogTmplName, sizeof(evlogTmplName));
		tmplName = evlogTmplName;
	}
	if (dirLen + strlen(tmplName) + sizeof("/.to0") > PATH_MAX) {
		return ENAMETOOLONG;
	}
	(void) strcpy(fileName, tmplName);
	(void) strcat(fileName, ".to");

	if (_evlWriteTemplate(t, path) != 0) {
		/* Currently EPERM is the most likely reason for it to fail. */
		return EPERM;
	}
	return 0;
}

/*
 * Write the templates in the indicated list to binary files with the
 * appropriate names.  This is the predecessor to evl_writetemplates(),
 * which is the preferred interface.
 */
static int
writeTemplatesToFiles(evl_list_t *templates, const char *progName)
{
	int retval = 0;
	evl_listnode_t *head = templates, *p, *end;
	char *dir = _evlTmplGetSourceDir();

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		template_t *t = (template_t*) p->li_data;
		int status = writeTemplateToFile(t, dir);
		if (status != 0) {
			fprintf(stderr,
"%s: Cannot write template file for template %s\n", progName, t->tm_name);
			errno = status;
			perror("writeTemplatesToFiles()");
			retval = -1;
		}
	}
	return retval;
}

/***** Find a struct template or an event-record template *****/

/*
 * Try to find a struct template via the indicated structPath (which is
 * typically a simple identifier).  If the template is not in memory, but
 * structPath is indeed a simple identifier, try to find and load the
 * struct's binary file in the primary directory.
 *
 * Note that this function is intended to be called when parsing an
 * attribute statement with type struct.
 */
template_t *
_evlFindStructTemplate(const char *structPath)
{

	evl_listnode_t *head = imports;
	evl_listnode_t *p, *end;
	tmpl_struct_ref_t *sref, *best = NULL;
	template_t *t;
	int simplePath = (strchr(structPath, '/') == NULL);

	/*
	 * First search the list of already-imported templates.
	 */
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		const char *lastSlash;
		sref = (tmpl_struct_ref_t *) p->li_data;
		if (!strcmp(sref->sr_path, structPath)) {
			/* Exact match */
			best = sref;
			break;
		}
		if (simplePath
		    && (lastSlash = strrchr(sref->sr_path, '/')) != NULL
		    && !strcmp(lastSlash+1, structPath)) {
			/*
			 * E.g.,
			 *	import a.b.c;
			 *	...
			 *	struct c var;
			 * Probably a match, unless there's an intervening
			 *	import c;
			 * Remember a/b/c, but keep looking.
			 */
			best = sref;
		}
	}

	if (best) {
		best->sr_used = 1;
		return best->sr_template;
	}

	if (simplePath) {
		/*
		 * No struct by this name has been explicitly imported;
		 * nor does it appear earlier in this source file.
		 * Look for it in the current directory.
		 */
		return _evlImportTemplateFromPaths(_evlTmplGetSourceDir(),
			structPath, TMPL_IMPORT_IMPLICIT);
	}
	return NULL;
}

/*
 * Return a pointer to the template for the specified facility and event type.
 * Returns NULL if no such template exists in memory.  *flags is set to the
 * FET flags associated with the specified facility and event type, or to 0
 * if there is no FET object.
 * If clone is 1, we make sure the template we are returning is a clone.
 */
static template_t *
findEvlogTemplate(posix_log_facility_t facility, int eventType, int *flags,
	int clone)
{
	fet_t *fet = findFETInHash(facility, eventType, &evrecHash);
	if (fet && fet->fet_default) {
		/* Use the default template for this facility. */
		fet = fet->fet_default;
	}
	if (fet) {
		template_t *t = fet->fet_template;
		*flags = fet->fet_flags;
		if (t) {
			if (isMasterTmpl(t)) {
				if (clone) {
					/* Found the master, want a clone. */
					template_t *cl = _evlCloneTemplate(t);
					assert(cl != NULL);
					if ((_evlTmplMgmtFlags &
					    TMPL_REUSE1CLONE) != 0) {
						/* Will reuse this clone in
						 * the future.
						 */
						fet->fet_template = cl;
					}
					t = cl;
				}
			} else {
				if (!clone) {
					/* Found the clone, want the master. */
					t = t->tm_master;
				}
			}
		}
		return t;
	} else {
		*flags = 0;
		return NULL;
	}
}

/***** Import-management functions *****/

/*
 * From {x, y, z}, make "x/y/z".  Returns a pointer to the malloced string.
 */
char *
_evlMakeSlashPathFromList(evl_list_t *list)
{
	evl_listnode_t *head = list, *p, *end;
	size_t nBytes = 0;
	char *s, *path, *pend;

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		s = (char*) p->li_data;
		nBytes += strlen(s) + 1;
	}

	path = (char*) malloc(nBytes);
	assert(path != NULL);

	/* pend fussing avoids progressively longer strcats. */
	pend = path;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		s = (char*) p->li_data;
		(void) strcpy(pend, s);
		pend += strlen(s);
		*pend++ = '/';
	}

	/*
	 * At this point we have "x/y/z/" with no terminating null.
	 * Replace the last slash with a null.
	 */
	pend[-1] = '\0';
	return path;
}

/*
 * From {x, y, z}, make "x.y.z".  Returns a pointer to the malloced string.
 */
char *
_evlMakeDotPathFromList(evl_list_t *list)
{
	char *c, *path = _evlMakeSlashPathFromList(list);
	for (c = path; *c; c++) {
		if (*c == '/') {
			*c = '.';
		}
	}
	return path;
}

static evl_list_t *tmplPath = NULL;

/*
 * No EVLTMPLPATH environment variable exists, so return a malloc-ed string
 * that contains the desired default paths:
 * If the LANG environment variable is defined:
 *	TMPL_PATH_DEFAULT/$LANG:TMPL_PATH_DEFAULT
 * Otherwise:
 *	TMPL_PATH_DEFAULT
 */
static char *
getDefaultTmplPath()
{
	char *path;
	const char *lang = getenv("LANG");
	if (lang && lang[0] != '\0') {
		int tpdLen = strlen(TMPL_PATH_DEFAULT);
		path = (char*) malloc(
			tpdLen + 1 + strlen(lang) + 1
			+ tpdLen + 1);
		assert(path != NULL);
		(void) strcpy(path, TMPL_PATH_DEFAULT);
		(void) strcat(path, "/");
		(void) strcat(path, lang);
		(void) strcat(path, ":");
		(void) strcat(path, TMPL_PATH_DEFAULT);
	} else {
		path = strdup(TMPL_PATH_DEFAULT);
	}
	return path;
}

/*
 * Set tmplPath based on the value of the EVLTMPLPATH (or whatever it is)
 * environment variable.
 */
static void
figureTmplPath()
{
	char *pathVar, *paths, *path, *s1, *s2;

	_evlLockMutex(&tmplPathMutex);
	if (tmplPath != NULL) {
		/* Another thread beat us to it. */
		_evlUnlockMutex(&tmplPathMutex);
		return;
	}

	pathVar = getenv(TMPL_PATH_VAR);
	if (pathVar) {
		paths = strdup(pathVar);
	} else {
		paths = getDefaultTmplPath();
	}

	for (s1 = paths; ; s1 = NULL) {
		struct stat st;
		path = strtok_r(s1, ":", &s2);
		if (!path) {
			break;
		}
		/*
		 * If this path is not a currently existing directory,
		 * don't ever bother searching it.  This optimization
		 * is consistent with the use of the FET_NOTTHERE flag:
		 * to be found, ever, the template needs to be there the
		 * first time we look.
		 */
		if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
			tmplPath = _evlAppendToList(tmplPath, path);
		}
	}
	if (!tmplPath) {
		/*
		 * Sigh.  None of the directories we want to search exist.
		 * If we leave tmplPath null, we'll just keep coming back
		 * to this function, so...
		 */
		tmplPath = _evlAppendToList(tmplPath,
			"/a presumably nonexistent directory");
	}

	/* Don't free paths here.  tmplPath contains pointers to it. */

	_evlUnlockMutex(&tmplPathMutex);
}

/*
 * Template t is a struct template that is newly read from a binary file.
 * st is that file's stat structure.  Add the template to the hash table.
 */
static void
registerStructTemplate(template_t *t, const struct stat *st)
{
	t->tm_context.tc_device = st->st_dev;
	t->tm_context.tc_inode = st->st_ino;
	addTemplateToHash(t, &structHash);
}

/* Allocate and initialize a struct ref. */
tmpl_struct_ref_t *
_evlTmplMakeStructRef(template_t *t, const char *structPath)
{
	tmpl_struct_ref_t *sref = (tmpl_struct_ref_t*) malloc(
		sizeof(tmpl_struct_ref_t));
	assert(sref != NULL);
	sref->sr_path = strdup(structPath);
	sref->sr_template = t;
	sref->sr_used = 0;
	return sref;
}

/*
 * If the imports list contains a struct ref whose path exactly matches
 * structPath, return a pointer to that struct ref; else return NULL.
 */
tmpl_struct_ref_t *
_evlFindStructRef(const char *structPath)
{
	evl_listnode_t *head = imports, *p, *end;
	tmpl_struct_ref_t *sref;

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		sref = (tmpl_struct_ref_t*) p->li_data;
		if (!strcmp(structPath, sref->sr_path)) {
			return sref;
		}
	}
	return NULL;
}

/*
 * Template t is a struct template that is newly imported, either explicity
 * as the result of an "import" statement (purpose = TMPL_IMPORT_EXPLICIT),
 * or implicitly due to a local struct ref in an attribute statement
 * (purpose = TMPL_IMPORT_IMPLICIT).  structPath is the slash-delimited
 * version of the (possibly qualified) struct name.  If there's not already
 * a matching struct ref in the imports list (which may be NULL), create
 * one and add it.
 * NOTE: We always add new nodes at the end, to preserve the order in which
 * templates are imported.
 */
static void
registerImport(template_t *t, const char *structPath, int purpose)
{
	tmpl_struct_ref_t *sref = _evlFindStructRef(structPath);
	if (sref) {
		/* Already have a template ref with that path. */
		if (purpose == TMPL_IMPORT_IMPLICIT) {
			sref->sr_used = 1;
		} else if (purpose == TMPL_IMPORT_EXPLICIT) {
			char *dotPath = makeDotPathFromSlashPath(structPath);
			_evlTmplSemanticError(
"struct %s previously defined or imported", dotPath);
			free(dotPath);
		}
		return;
	}

	/* No matching template ref in list. */
	sref = _evlTmplMakeStructRef(t, structPath);
	imports = _evlAppendToList(imports, sref);
	if (purpose == TMPL_IMPORT_IMPLICIT) {
		sref->sr_used = 1;
	}
}

/*
 * The current template refers to struct template st in some way other than
 * by name -- e.g., via a typedef.  Remember this ref so it'll be written
 * to the .to file with the template's other struct refs.
 */
void
_evlRegisterStructRef(template_t *st)
{
	evl_listnode_t *head, *end, *p;
	tmpl_struct_ref_t *sref;

	for (head=imports, p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		sref = (tmpl_struct_ref_t *) p->li_data;
		if (sref->sr_template == st) {
			sref->sr_used = 1;
			break;
		}
	}
}

/*
 * We're about to parse a new template.  Mark all struct refs (imports) as
 * unused by this template.
 */
static void
forgetStructRefs()
{
	evl_listnode_t *head = imports, *p, *end;
	tmpl_struct_ref_t *sref;

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		sref = (tmpl_struct_ref_t*) p->li_data;
		sref->sr_used = 0;
	}
}

/*
 * We've finished parsing a new template, t.  Populate its imports list with
 * those struct refs (from the global list) that have been used during
 * parsing.  This is useful if we're going to write this template to a
 * binary file.
 */
static void
rememberStructRefs(template_t *t)
{
	evl_listnode_t *head = imports, *p, *end;
	tmpl_struct_ref_t *sref;

	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		sref = (tmpl_struct_ref_t*) p->li_data;
		if (sref->sr_used) {
			t->tm_imports = _evlAppendToList(t->tm_imports, sref);
		}
	}
}

/*
 * dir is the pathname of a directory, and structPath is a relative path
 * that ends in the basename of a template file (e.g., the name of a struct).
 * If there is such a template file, import it, unless it's already been
 * imported.  Return a pointer to the imported template, or NULL on failure.
 */
static template_t *
_evlImportTemplateFromPaths(const char *dir, const char *structPath, int purpose)
{
	char fullPath[PATH_MAX];
	const char *structName, *lastSlash;
	struct stat st;
	template_t *t;

	if (strlen(dir) + strlen(structPath) + sizeof("/.to0") > PATH_MAX) {
		return NULL;
	}
	(void) strcpy(fullPath, dir);
	(void) strcat(fullPath, "/");
	(void) strcat(fullPath, structPath);
	(void) strcat(fullPath, ".to");

	if (stat(fullPath, &st) != 0) {
		/* Can't read specified file. */
		return NULL;
	}

	/* Have we already imported the template from this file? */
	lastSlash = strrchr(structPath, '/');
	if (lastSlash) {
		structName = lastSlash+1;
	} else {
		structName = structPath;
	}
	t = findTemplateInHash(structName, &st, &structHash);

	if (!t) {
		/* Not previously imported.  Read in its binary image. */
		t = _evlReadTemplate(fullPath);
		if (t) {
			registerStructTemplate(t, &st);
		}
	}
	if (t && (purpose == TMPL_IMPORT_EXPLICIT
	    || purpose == TMPL_IMPORT_IMPLICIT)) {
		registerImport(t, structPath, purpose);
	}
	return t;
}

/*
 * We are processing a statement of the form "import subdir.*".
 * dir is one of the directories where we look for templates.  Import
 * all the templates in dir/subdir/*.to.  Return 0 (success) if we find
 * at least one such file and successfully import it.  Return -1 on failure.
 */
static int
importAllTemplatesFromDir(const char *dir, const char *subdir)
{
	char dirPath[PATH_MAX];		/* dir/subdir */
	char structPath[PATH_MAX];	/* subdir/basename */
	char *fileName;	/* points to basename in structPath */
	struct dirent *dent;
	DIR *d;
	size_t splen;	/* length of structPath */
	int success = 0;

	if (strlen(dir) + strlen(subdir) + 3 > PATH_MAX) {
		return -1;
	}
	(void) strcpy(dirPath, dir);
	(void) strcat(dirPath, "/");
	(void) strcat(dirPath, subdir);

	/* Is dir/subdir a directory? */
	d = opendir(dirPath);
	if (!d) {
		/* Nope. */
		return -1;
	}

	/* Yes.  Try to import every .to file it contains. */
	(void) strcpy(structPath, subdir);
	(void) strcat(structPath, "/");
	splen = strlen(structPath);
	fileName = structPath + splen;
	while ((dent = readdir(d)) != NULL) {
		char *fn = dent->d_name;
		char *dot;
		if (!_evlEndsWith(fn, ".to")) {
			continue;
		}

		/* Don't import any non-struct templates. */
		if (*fn != '_' && !isalpha(*fn)) {
			continue;
		}

		/* Append filename, minus .to, to structName. */
		if (splen + strlen(fn) > PATH_MAX) {
			continue;
		}
		(void) strcpy(fileName, fn);
		dot = strrchr(fileName, '.');
		assert(dot != NULL);
		*dot = '\0';

		if (_evlImportTemplateFromPaths(dir, structPath,
		    TMPL_IMPORT_EXPLICIT) != NULL) {
			success = 1;
		}
	}
	closedir(d);
	return (success ? 0 : -1);
}

/*
 * Import the template with the specified structPath, using a search
 * starting at primaryDir.  purpose specifies why we're importing this
 * template.
 */
template_t *
_evlImportTemplate(const char *primaryDir, const char *structPath, int purpose)
{
	evl_listnode_t *head, *p, *end;
	template_t *t;

	t = _evlImportTemplateFromPaths(primaryDir, structPath, purpose);
	if (t) {
		return t;
	}

	if (!tmplPath) {
		figureTmplPath();
	}
	head = tmplPath;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		char *dir = (char*) p->li_data;
		t = _evlImportTemplateFromPaths(dir, structPath, purpose);
		if (t) {
			return t;
		}
	}
	return NULL;
}

/*
 * list is a sequence of one or more identifiers that specify a template
 * (or template directory, if dotStar == 1) to be imported.  Import the
 * template(s).  Return 0 on success or -1 on failure.
 */
int
_evlImportTemplateFromIdList(evl_list_t *list, int dotStar)
{
	char *structPath = _evlMakeSlashPathFromList(list);
	char *dotPath;
	evl_listnode_t *head, *p, *end;
	char *primaryDir = _evlTmplGetSourceDir();

	if (!tmplPath) {
		figureTmplPath();
	}

	if (dotStar) {
		/*
		 * structPath specifies a subdirectory.  Try to find that
		 * subdirectory, and import all .to files from that directory.
		 * We stop when we find a directory that has at least one .to
		 * file that we can import.
		 */
		if (importAllTemplatesFromDir(primaryDir, structPath) == 0) {
			goto success;
		}
		head = tmplPath;
		for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
			char *dir = (char*) p->li_data;
			if (importAllTemplatesFromDir(dir, structPath) == 0) {
				goto success;
			}
		}
	} else {
		/* Just import the specified template. */
		if (_evlImportTemplate(primaryDir, structPath, TMPL_IMPORT_EXPLICIT)) {
			goto success;
		}
	}

	/* Not found.  Report error, but use "x.y.z", not "x/y/z". */
	dotPath = makeDotPathFromSlashPath(structPath);
	if (dotStar) {
		_evlTmplSemanticError("Cannot find any matching templates: %s.*",
			dotPath);
	} else {
		_evlTmplSemanticError("Cannot find template: %s", dotPath);
	}
	free(structPath);
	free(dotPath);
	return -1;

success:
	free(structPath);
	return 0;
}

/***** Publicly documented functions *****/

int
evl_parsetemplates(const char *source_filename, evltemplate_t *template_list[],
	size_t listsize, size_t *ntemplates, FILE *error_file,
	const char *prog_name)
{
	evl_list_t *templates;
	evl_listnode_t *t;
	int errors;
	int fd;
	int i,j;
	size_t nReturned;

	if (!source_filename || !template_list || !ntemplates) {
		return EINVAL;
	}

	/*
	 * Spec says we return EINVAL for an unreadable file, as opposed to
	 * EBADMSG for syntax errors.
	 */
	fd = open(source_filename, O_RDONLY);
	if (fd < 0) {
		return EINVAL;
	} else {
		close(fd);
	}

	templates = getTemplatesFromFile(source_filename, error_file,
		prog_name, &errors);

	*ntemplates = _evlGetListSize(templates);
	if (*ntemplates <= listsize) {
		nReturned = *ntemplates;
	} else {
		nReturned = listsize;
	}

	for (t = templates, i = 0; i < nReturned; i++, t = t->li_next) {
		template_list[i] = (template_t*) t->li_data;
	}

	/* Check for duplicates. */
	for (i = 1; i < nReturned; i++) {
		const template_t *ti = template_list[i];
		const char *tiname = ti->tm_name;
		for (j = 0; j < i; j++) {
			if (!strcmp(tiname, template_list[j]->tm_name)) {
				if (ti->tm_header.th_type ==
				    TMPL_TH_STRUCT) {
fprintf(error_file, "%s: multiple templates for struct %s\n",
					source_filename, tiname);
				} else {
fprintf(error_file, "%s: multiple templates for facility %#x, event_type %#x\n",
					source_filename, getFacility(ti),
					getEventType(ti));
				}
				errors++;
				break;
			}
		}
	}

	if (errors > 0) {
		return EBADMSG;
	} else if (nReturned != *ntemplates) {
		return EMSGSIZE;
	}

	return 0;
}

int
evl_writetemplates(const char *directory, const evltemplate_t *template_list[],
	size_t listsize)
{
	struct stat st;
	int i, status;

	if (!template_list || listsize == 0) {
		return EINVAL;
	}
	if (!directory) {
		return ENOTDIR;
	}

	if (stat(directory, &st) != 0 || !S_ISDIR(st.st_mode)) {
		return ENOTDIR;
	}

	for (i = 0; i < listsize; i++) {
		status = writeTemplateToFile(template_list[i], directory);
		if (status != 0) {
			return status;
		}
	}
	return 0;
}

static int
templateMatchesPath(const char *path, const template_t *t,
	posix_log_facility_t facility, int event_type)
{
	posix_log_facility_t tfac = getFacility(t);
	int tevty = getEventType(t);
	if (tfac != facility || tevty != event_type) {
		char facname[POSIX_LOG_MEMSTR_MAXLEN];
		if (!_evlGetFacilityName(tfac, facname)) {
			(void) snprintf(facname, sizeof(facname), "0x%x", tfac);
		}
		fprintf(stderr,
			"%s contains a template with\n"
			"the wrong facility (%s) and/or event type (0x%x).\n",
			path, facname, tevty);
		return 0;
	}
	return 1;
}

/*
 * Does the work of evl_readtemplate().  Called, perhaps recursively, with
 * tmplCacheMutex locked.
 */
int
readTemplateLocked(posix_log_facility_t facility, int event_type,
	evltemplate_t **template, int clone)
{
	template_t *t;
	char facName[POSIX_LOG_MEMSTR_MAXLEN];	/* e.g., "KERN" */
	char *fileName;				/* e.g., "100.to" */
	char subpath[POSIX_LOG_MEMSTR_MAXLEN+25];  /* e.g., "kern/100.to" */
	char path[PATH_MAX];	/* e.g., "/var/evlog/templates/kern/100.to" */
	int status = 0;
	evl_listnode_t *head, *p, *end;
	int flags = 0;
	fet_t *fet;

	if (_evlTmplMgmtFlags & TMPL_IGNOREALL) {
		return ENOENT;
	}

	if (!template) {
		return EINVAL;
	}

	/* If the template is already in memory, return a pointer to it.  */
	t = findEvlogTemplate(facility, event_type, &flags, clone);
	if (t) {
		*template = t;
		return 0;
	} else if ((flags & FET_NOTTHERE) != 0) {
		/*
		 * We've looked for such a template previously.  It's not there.
		 */
		return ENOENT;
	}

	/*
	 * Not in memory.  Go looking for it in all the directories specified
	 * by the EVLTMPLPATH environment variable.
	 */
	if (!tmplPath) {
		figureTmplPath();
	}

	if (posix_log_factostr(facility, facName, POSIX_LOG_MEMSTR_MAXLEN)
	    != 0) {
		/* No such facility in registry. */
		goto noSuchTemplate;
	}
	
	(void) _evlGenCanonicalFacilityName(facName, subpath);
	(void) strcat(subpath, "/");
	fileName = subpath + strlen(subpath);
	makeEvlogTmplName(facility, event_type, fileName, sizeof(subpath) - strlen(subpath));
	(void) strcat(fileName, ".to");

	head = tmplPath;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		struct stat st;
		char *dir = (char*) p->li_data;
		if (strlen(dir) + 1 + strlen(subpath) + 1 > PATH_MAX) {
			continue;
		}
		(void) strcpy(path, dir);
		(void) strcat(path, "/");
		(void) strcat(path, subpath);
		if (stat(path, &st) != 0) {
			continue;
		}
		t = _evlReadTemplate(path);
		if (t && templateMatchesPath(path, t, facility, event_type)) {
			if (clone) {
				t = _evlCloneTemplate(t);
				assert(t != NULL);
			}
			*template = t;
			addTemplateToFETHash(t, &evrecHash);
			return 0;
		}
		/*
		 * A file exists with this pathname, but we can't load a
		 * template from it.  We'll return EIO unless we find another
		 * path that works.
		 */
		status = EIO;
	}

noSuchTemplate:
	/* Failed to find the template. */
	if (event_type != EVL_DEFAULT_EVENT_TYPE) {
		/*
		 * No template with this event type.  Return the default
		 * template for this facility, if any.
		 */
		fet_t *dfltFet;
		int status2 = readTemplateLocked(facility,
			EVL_DEFAULT_EVENT_TYPE, template, clone);
		if (status2 == 0) {
			/* There is indeed a default template.  */
			dfltFet = findFETInHash(facility,
				EVL_DEFAULT_EVENT_TYPE, &evrecHash);
			assert(dfltFet != NULL);
			fet = makeFET(facility, event_type);
			fet->fet_default = dfltFet;
			addFETToHash(fet, &evrecHash);
			return 0;
		} else if (status == 0) {
			status = status2;
		}
	}

	/*
	 * Could find neither the specific template for this event type
	 * nor the default template.  Remember this failure in the future.
	 */
	fet = makeFET(facility, event_type);
	fet->fet_flags = FET_NOTTHERE;
	addFETToHash(fet, &evrecHash);

	if (status != 0) {
		return status;
	} else {
		return ENOENT;
	}
}

struct redirection_context {
	const struct posix_log_entry	*entry;
	const void			*buf;
	posix_log_facility_t		facility;
	int				event_type;
	evl_list_t			*history;
	evltemplate_t			*tmpl;
};

/*
 * Populate rc->tmpl, if it isn't already, and point *att at the attribute
 * whose name is attName.  Return EINVAL if this attribute is not a scalar
 * integer or string.
 */
static int
getRedirectionAtt(const char *attName, evlattribute_t **att,
	struct redirection_context *rc)
{
	int status;

	if (!isPopulatedTmpl(rc->tmpl)) {
		status = evl_populatetemplate(rc->tmpl, rc->entry, rc->buf);
		if (status != 0) {
			return status;
		}
	}
	if (evltemplate_getatt(rc->tmpl, attName, att) == 0) {
		if (isArray(*att)) {
			return EINVAL;
		} else if (!attExists(*att)) {
			status = ENOENT;
		} else if (isIntegerAtt(*att) || baseType(*att) == TY_STRING) {
			status = 0;
		} else {
			status = EINVAL;
		}
	} else {
		status = ENOENT;
	}
	return status;
}

/*
 * rc->tmpl is a redirection template.  Update rc->facility and
 * rc->event_type accordingly.
 */
static int
redirect2(struct redirection_context *rc)
{
	int status = 0;
	evlattribute_t *att;
	tmpl_redirection_t *rd = rc->tmpl->tm_redirection;

	if (rd->rd_fac) {
		switch (rd->rd_fac->type) {
		case TMPL_RD_NONE:
			break;
		case TMPL_RD_CONST:
			rc->facility = rd->rd_fac->u.faccode;
			break;
		case TMPL_RD_ATTNAME:
			status = getRedirectionAtt(rd->rd_fac->u.attname, &att,
				rc);
			if (status != 0) {
				return status;
			}
			if (baseType(att) == TY_STRING) {
				status = posix_log_strtofac(
					evl_getStringAttVal(att),
					&rc->facility);
			} else {
				rc->facility = (posix_log_facility_t)
					evl_getUlongAttVal(att);
			}
			break;
		default:
			status = EINVAL;
		}
	}

	if (status == 0 && rd->rd_evtype ) {
		switch (rd->rd_evtype->type) {
		case TMPL_RD_NONE:
			break;
		case TMPL_RD_CONST:
			rc->event_type = rd->rd_evtype->u.evtype;
			break;
		case TMPL_RD_ATTNAME:
			status = getRedirectionAtt(rd->rd_evtype->u.attname,
				&att, rc);
			if (status != 0) {
				return status;
			}
			if (baseType(att) == TY_STRING) {
				rc->event_type = evl_gen_event_type_v2(
					evl_getStringAttVal(att));
			} else {
				rc->event_type = (int) evl_getLongAttVal(att);
			}
			break;
		default:
			status = EINVAL;
		}
	}

	return (status == 0 ? 0 : ENOENT);
}

/*
 * rc->tmpl is a redirection template.  Update rc->facility and
 * rc->event_type accordingly.
 * This function just fusses with the history (to detect redirection loops)
 * and calls redirect2.
 * If a template redirects to itself, this won't be caught until the 2nd
 * redirection.  No big deal.
 */
static int
redirect(struct redirection_context *rc)
{
	fet_t *fet;	/* handy container for facility & event_type */
	evl_listnode_t *head = rc->history, *p, *end;

	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		fet = (fet_t*) p->li_data;
		if (fet->fet_facility == rc->facility
		    && fet->fet_eventType == rc->event_type) {
			/* redirection loop */
			return ENOENT;
		}
	}

	/* Add old facility and event_type to history. */
	fet = makeFET(rc->facility, rc->event_type);
	assert(fet != NULL);
	rc->history = _evlAppendToList(rc->history, fet);

	return redirect2(rc);
}

/*
 * Like evl_readtemplate(), but handles template redirection.  For ease
 * of implementation, we don't support the (currently unused) case
 * where the caller wants a master template, not a clone.
 */
int
evl_gettemplate(const struct posix_log_entry *entry, const void *buf,
	evltemplate_t **template)
{
	int status;
	int redirecting;
	struct redirection_context rc;
	evltemplate_t *lastValidTmpl = NULL, *prevTmpl = NULL;

	if (!entry) {
		return EINVAL;
	}

	rc.entry = entry;
	rc.buf = buf;
	rc.facility = entry->log_facility;
	rc.event_type = entry->log_event_type;
	rc.history = NULL;

	do {
		redirecting = 0;
		status = evl_readtemplate(rc.facility, rc.event_type,
			&rc.tmpl, 1);
		if (status == 0) {
			lastValidTmpl = rc.tmpl;
			if (prevTmpl) {
				(void) evl_releasetemplate(prevTmpl);
				prevTmpl = NULL;
			}
			if (rc.tmpl->tm_redirection) {
				status = redirect(&rc);
				if (status == 0) {
					redirecting = 1;
					prevTmpl = lastValidTmpl;
				}
			}
		}
	} while (redirecting);
	if (lastValidTmpl) {
		*template = lastValidTmpl;
		status = 0;
	}
	_evlFreeList(rc.history, 1);
	return status;
}

/*
 * Load the template associated with the specified facility and event_type.
 * NOTE: This function does not do redirection.  To load the template you
 * get after following any redirections, use evl_gettemplate().
 */
int
evl_readtemplate(posix_log_facility_t facility, int event_type,
	evltemplate_t **template, int clone)
{
	/*
	 * For simplicity, at the possible expense of multi-thread performance,
	 * we lock the template cache at a very high level.  We keep it
	 * locked throughout this function, even if we have to go to the
	 * filesystem to load the template.
	 */

	int status;
	_evlLockMutex(&tmplCacheMutex);
	status = readTemplateLocked(facility, event_type, template, clone);
	_evlUnlockMutex(&tmplCacheMutex);
	return status;
}

/*
 * Configure the template manager.  We don't currently allow the process to
 * change the flags once they have been set.
 */
int
evltemplate_initmgr(int flags)
{
	static int alreadyCalled = 0;

	if (alreadyCalled) {
		return EINVAL;
	}
	alreadyCalled = 1;
	_evlTmplMgmtFlags = flags;
	return 0;
}

int
evl_freetemplate(evltemplate_t *t)
{
	fet_t *fet;

	if (!t) {
		return EINVAL;
	}
	if (t->tm_ref_count != 0) {
		return EINVAL;
	}

	_evlLockMutex(&tmplCacheMutex);
	fet = findFETInHash(getFacility(t), getEventType(t), &evrecHash);
	if (fet && fet->fet_template == t) {
		/*
		 * We're discarding the template pointed to by the
		 * hash table.  Update the FET to point to t's master,
		 * if t is a clone, or NULL otherwise.  Even in the latter
		 * case, we leave the FET in the hash.
		 */
		fet->fet_template = t->tm_master;
	}
	_evlUnlockMutex(&tmplCacheMutex);

	_evlFreeTemplate(t);
	return 0;
}

/*
 * This template is no longer in use by the caller.  If there is no longer a
 * use for it, deallocate it.
 *
 * Note: If t is a master template, and another thread tries to clone it
 * (e.g., via evl_readtemplate(fac, ety, &cln, 1)) while this thread is
 * releasing it, disorder can occur.  It's up to the caller to avoid
 * releasing a master template that might still be in use.
 */
int
evl_releasetemplate(evltemplate_t *t)
{
	if (!t || t->tm_ref_count != 0) {
		return EINVAL;
	}

	if ((_evlTmplMgmtFlags & TMPL_LAZYDEPOP) == 0) {
		(void) evl_depopulatetemplate(t);
	}

	if (!isMasterTmpl(t) && (_evlTmplMgmtFlags & TMPL_REUSE1CLONE) != 0) {
		/* We'll probably use this clone again. */
		return 0;
	}

	return evl_freetemplate(t);
}
