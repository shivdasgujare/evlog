#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_template.h"

/*
 * usage: $0 recid facility filename_format
 * Example: $0 4567 ipr /tmp/ela%d
 * Reads event record #4567, which is an opaque event.  Matches the record's
 * msg attribute to an entry in ipr_table.to, and appends the ELA info from
 * that entry to the event record.  Invents a new recid for the record (i.e.,
 * the negative of the original recid), and sets the facility to ipr.  Writes
 * the adjusted record to /tmp/ela-4567.  Prints -4567 to stdout.
 */

#define min(a,b) ((a) < (b) ? (a) : (b))

char *progName;
posix_log_recid_t recid;
char *facName;
posix_log_facility_t facility;
char *filenameFormat;

struct ela_entry {
	char	*msg;
	char	*file;
	char	*elaClass;
	char	*elaType;
	int	nProbableCauses;
	char	**probableCauses;
	int	nActions;
	char	**actions;
	int	threshold;
	char	*interval;
	posix_log_recid_t alt_event_type;
};

int
usage()
{
	fprintf(stderr, "usage: %s recid facility filename_format\n", progName);
	exit(1);
}

void
processArgs(int argc, char **argv)
{
	int status;
	char *c;

	progName = argv[0];
	if (argc != 4) {
		usage();
	}

	recid = (posix_log_recid_t) strtol(argv[1], &c, 0);
	if (*c != '\0') {
		usage();
	}

	facName = argv[2];
	status = posix_log_strtofac(facName, &facility);
	if (status != 0) {
		fprintf(stderr, "%s: Unknown facility: %s\n",
			progName, facName);
		exit(2);
	}

	filenameFormat = argv[3];
}

static void
readEvent(posix_log_recid_t recid, struct posix_log_entry *entry,
	char *buf, size_t buflen)
{
	int status;
	posix_logd_t logd;
	posix_log_query_t query;
	char query_str[40];

	status = posix_log_open(&logd, NULL);
	if (status != 0) {
		errno = status;
		perror("posix_log_open");
		exit(2);
	}
	sprintf(query_str, "recid=%d", recid);
	status = posix_log_query_create(query_str, POSIX_LOG_PRPS_SEEK,
		&query, NULL, 0);
	if (status != 0) {
		errno = status;
		perror("posix_log_query_create");
		exit(2);
	}
	status = posix_log_seek(logd, &query, POSIX_LOG_SEEK_LAST);
	if (status == ENOENT) {
		fprintf(stderr, "%s: event record %d has disappeared!\n",
			progName, recid);
		exit(2);
	} else if (status != 0) {
		errno = status;
		perror("posix_log_seek");
		exit(2);
	}
	status = posix_log_read(logd, entry, buf, buflen);
	if (status != 0) {
		errno = status;
		perror("posix_log_read");
		exit(2);
	}
	(void) posix_log_close(logd);
}

/* Cut and pasted from libevl's formatrec.c */
static template_t *
getPopulatedTemplate(const struct posix_log_entry *entry, const char *buf)
{
	int status;
	template_t *tmpl;
	posix_log_facility_t fac = entry->log_facility;
	int evType = entry->log_event_type;

	status = evl_readtemplate(fac, evType, &tmpl, 1);
	if (status == ENOENT) {
		/* No such template. */
		return NULL;
	}
	assert(status == 0);

	status = evl_populatetemplate(tmpl, entry, buf);
	assert(status == 0);
	return tmpl;
}

static evlattribute_t *
getAtt(template_t *t, const char *attName, int vital)
{
	evlattribute_t *att;
	int status = evltemplate_getatt(t, attName, &att);
	if (status != 0) {
		fprintf(stderr,
			"%s: Can't find attribute %s for recid %d\n",
			progName, attName, t->tm_entry->log_recid);
		if (vital) {
			exit(2);
		}
		return NULL;
	}
	return att;
}

static template_t *
findElaEntry(template_t *tmpl, const char *msg)
{
	evlattribute_t *att, *msgAtt;
	evl_listnode_t *head, *p, *end;
	char *msgStr;

	att = getAtt(tmpl, "ipr_table.e", 1);
	assert(isArray(att) && isStruct(att));

	head = att->ta_value.val_list;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		template_t *st = (template_t*) p->li_data;
		msgAtt = getAtt(st, "msg", 0);
		if (!msgAtt) {
			continue;
		}
		msgStr = evl_getStringAttVal(msgAtt);
		if (!strcmp(msgStr, msg)) {
			return st;
		}
	}
	return NULL;
}

static char **
collectStringArray(evlattribute_t *att, int *pnel)
{
	int nel, i;
	char **strings;
	char *s;

	if (!att) {
		*pnel = 0;
		return NULL;
	}

	assert(isArray(att) && baseType(att) == TY_STRING);
	*pnel = nel = att->ta_dimension->td_dimension;
	strings = (char**) malloc(nel * sizeof(char*));
	assert(strings != NULL);

	s = evl_getArrayAttVal(att);
	for (i = 0; i < nel; i++) {
		strings[i] = s;
		s += strlen(s) + 1;
	}
	return strings;
}

static void
extractElaInfo(template_t *tmpl, struct ela_entry *ee)
{
	evlattribute_t *att;
	template_t *eeTmpl;

	memset(ee, 0, sizeof(struct ela_entry));
	
	att = getAtt(tmpl, "alt_event_type", 1);
	ee->alt_event_type = (int) evl_getLongAttVal(att);

	/*
	 * Find the message in the ELA table that matches the message
	 * in the original event.
	 */
	att = getAtt(tmpl, "msg", 1);
	ee->msg = evl_getStringAttVal(att);

	eeTmpl = findElaEntry(tmpl, ee->msg);
	if (!eeTmpl) {
		fprintf(stderr, "%s: Can't find ELA info for message: %s\n",
			progName, ee->msg);
		exit(2);
	}

	att = getAtt(eeTmpl, "file", 0);
	if (att) {
		ee->file = evl_getStringAttVal(att);
	}
	att = getAtt(eeTmpl, "elaClass", 0);
	if (att) {
		ee->elaClass = evl_getStringAttVal(att);
	}
	att = getAtt(eeTmpl, "elaType", 0);
	if (att) {
		ee->elaType = evl_getStringAttVal(att);
	}

	att = getAtt(eeTmpl, "probableCauses", 0);
	ee->probableCauses = collectStringArray(att, &ee->nProbableCauses);

	att = getAtt(eeTmpl, "actions", 0);
	ee->actions = collectStringArray(att, &ee->nActions);

	att = getAtt(eeTmpl, "threshold", 0);
	if (att) {
		ee->threshold = (int) evl_getLongAttVal(att);
	}
	att = getAtt(eeTmpl, "interval", 0);
	if (att) {
		ee->interval = evl_getStringAttVal(att);
	}
}

/* Cut and pasted from kernel/evlapi.c */
struct evl_recbuf {
	char *b_buf;	/* start of buffer */
	char *b_tail;	/* add next data here */
	char *b_end;	/* b_buf + buffer size */
};

static void
evl_init_recbuf(struct evl_recbuf *b, char *buf, size_t size)
{
	b->b_buf = buf;
	b->b_tail = buf;
	b->b_end = buf + size;
}

static void
evl_put(struct evl_recbuf *b, const void *data, size_t datasz)
{
	ptrdiff_t room = b->b_end - b->b_tail;
	if (room > 0) {
		(void) memcpy(b->b_tail, data, min(datasz, (size_t)room));
	}
	b->b_tail += datasz;
}

static void
evl_puts(struct evl_recbuf *b, const char *s, int null)
{
	char *old_tail = b->b_tail;
	if (s) {
		evl_put(b, s, strlen(s) + null);
	} else {
		evl_put(b, "", null);
	}
	if (b->b_tail > b->b_end && old_tail < b->b_end) {
		*(b->b_end - 1) = '\0';
	}
}

static void
adjustRecord(struct posix_log_entry *entry, char *buf, size_t bufsz,
	struct ela_entry *ee)
{
	int i;
	struct evl_recbuf b;

	/* entry->log_recid *= -1; */
	entry->log_event_type = ee->alt_event_type;
	entry->log_facility = facility;

	evl_init_recbuf(&b, buf, bufsz);
	b.b_tail = buf + entry->log_size;
	evl_puts(&b, ee->file, 1);
	evl_puts(&b, ee->elaClass, 1);
	evl_puts(&b, ee->elaType, 1);
	evl_put(&b, &ee->nProbableCauses, sizeof(ee->nProbableCauses));
	for (i = 0; i < ee->nProbableCauses; i++) {
		evl_puts(&b, ee->probableCauses[i], 1);
	}
	evl_put(&b, &ee->nActions, sizeof(ee->nActions));
	for (i = 0; i < ee->nActions; i++) {
		evl_puts(&b, ee->actions[i], 1);
	}
	evl_put(&b, &ee->threshold, sizeof(ee->threshold));
	evl_puts(&b, ee->interval, 1);

	entry->log_size = (size_t) (b.b_tail - buf);
	if (entry->log_size > bufsz) {
		entry->log_size = bufsz;
	}
}

static void
writeFakeEvent(struct posix_log_entry *entry, char *buf)
{
	char path[PATH_MAX];
	int fd;

	snprintf(path, PATH_MAX, filenameFormat, entry->log_recid);
	fd = open(path, O_WRONLY|O_CREAT, 0666);
	if (fd < 0) {
		perror(path);
		exit(2);
	}
	if (_evlWriteLogHeader(fd) < 0) {
		fprintf(stderr, "%s: Cannot write log header to %s\n",
			progName, path);
		perror(path);
		exit(2);
	}
	if (_evlFdWrite(fd, entry, buf) != 0) {
		fprintf(stderr, "%s: Cannot write event record to %s\n",
			progName, path);
		perror(path);
		exit(2);
	}
	(void) close(fd);
}

main(int argc, char **argv)
{
	struct posix_log_entry entry;
	char buf[POSIX_LOG_ENTRY_MAXLEN];
	template_t *tmpl;
	struct ela_entry ee;

	processArgs(argc, argv);

	/* Read the original event. */
	readEvent(recid, &entry, buf, POSIX_LOG_ENTRY_MAXLEN);

	/* Extract the ELA info from the template. */
	tmpl = getPopulatedTemplate(&entry, buf);
	if (tmpl == NULL) {
		fprintf(stderr, "%s: No template for recid %d\n",
			progName, recid);
		exit(2);
	}
	extractElaInfo(tmpl, &ee);
	
	/* Augment the event with the ELA info. */
	adjustRecord(&entry, buf, POSIX_LOG_ENTRY_MAXLEN, &ee);

	/* Fake 'evlview -o temp' of this record. */
	writeFakeEvent(&entry, buf);

	printf("%d\n", entry.log_recid);
	exit(0);
}
