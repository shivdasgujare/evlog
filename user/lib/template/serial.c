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

/*
 * This file, along with serialio.inc, implements template serialization.
 * _evlWriteTemplate() writes a depopulated template out to a binary file,
 * and _evlReadTemplate() reads it back in.
 *
 * This file contains three distinct levels of functions.  First come the
 * low-level functions, which are called by the mid- and high-level
 * functions.  The mid-level functions are defined in serialio.inc, which
 * is included twice by this file (once to define read functions and once
 * to define write functions -- both use the same source).  At the end come
 * the high-level functions.  Try to move as much code as possible to the
 * mid-level functions, to minimize code bloat and minimize the possibility
 * of read and write operations getting out of sync.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <assert.h>

#include "evl_util.h"
#include "evl_template.h"

#define TMPL_MAGIC 0xfeedf00d

/*
 * This object defines a buffer that contains an image of a binary template
 * file.  tf_next starts at tf_buf, and advances through the buffer as values
 * are read or written.  When writing, we don't know in advance how big the
 * buffer needs to be, so the buffer is extensible.
 */
typedef struct tfile {
	char	*tf_buf;	/* start of buffer */
	size_t	tf_bufsz;	/* buffer size in bytes */
	char	*tf_next;	/* next value is/goes here */
	char	*tf_end;	/* tf_buf + tf_bufsz */
	const char *tf_path;	/* file we're reading/writing */
	char	*tf_dir;	/* primary directory for importing structs */
	int	tf_errors;	/* counts errors while reading */
} tfile_t;

/* These are defined in serialio.inc, but called earlier in this file. */
static void writeConstStruct(template_t *t, tfile_t *tf);
static void readConstStruct(template_t *t, tfile_t *tf);
static int writeConstAttDimension(tmpl_attribute_t *att, tfile_t *tf);
static int readConstAttDimension(tmpl_attribute_t *att, tfile_t *tf);

/* Advance tf_next past data we've read. */
static void
tfAdvance(tfile_t *tf, size_t nBytes)
{
	tf->tf_next += nBytes;
	assert(tf->tf_next <= tf->tf_end);
}

/* Append val (size bytes) to the buffer, */
static void
tfWrite(tfile_t *tf, const void *val, size_t size)
{
	char *newNext = tf->tf_next + size;
	if (newNext > tf->tf_end) {
		/*
		 * Buffer would overflow.  Make it twice as big as we'll need
		 * for the data known about so far.
		 */
		size_t curBytes = tf->tf_next - tf->tf_buf;
		tf->tf_bufsz = 2 * (newNext - tf->tf_buf);
		tf->tf_buf = realloc(tf->tf_buf, tf->tf_bufsz);
		assert(tf->tf_buf != NULL);
		tf->tf_next = tf->tf_buf + curBytes;
		tf->tf_end = tf->tf_buf + tf->tf_bufsz;
		/* Note: newNext is no longer valid since buf has moved. */
	}

	memcpy(tf->tf_next, val, size);
	tf->tf_next += size;
}

/* Copy size bytes of data from the buffer to val, and advance tf_next. */
static void
tfRead(tfile_t *tf, void *val, size_t size)
{
	memcpy(val, tf->tf_next, size);
	tfAdvance(tf, size);
}

#define readScalar(val, tf) tfRead((tf), &(val), sizeof(val))
#define writeScalar(val, tf) tfWrite((tf), &(val), sizeof(val))

static void
writeString(const char *s, tfile_t *tf)
{
	tfWrite(tf, s, strlen(s)+1);
}

static char *
readStringF(tfile_t *tf)
{
	char *s = strdup(tf->tf_next);
	assert(s != NULL);
	tfAdvance(tf, strlen(s)+1);
	return s;
}

#define readString(val, tf) val = readStringF(tf)

static void
writeStringOrNull(const char *s, tfile_t *tf)
{
	char exists = (s != NULL);
	writeScalar(exists, tf);
	if (exists) {
		writeString(s, tf);
	}
}

static char *
readStringOrNullF(tfile_t *tf)
{
	char exists;
	readScalar(exists, tf);
	if (exists) {
		return readStringF(tf);
	} else {
		return NULL;
	}
}

#define readStringOrNull(val, tf) val = readStringOrNullF(tf)

static void
writeWstring(const wchar_t *s, tfile_t *tf)
{
	size_t nwc = wcslen(s);
	writeScalar(nwc, tf);
	tfWrite(tf, s, (nwc+1)*sizeof(wchar_t));
}

static wchar_t *
readWstringF(tfile_t *tf)
{
	size_t nwc, nbytes;
	wchar_t *s;
	readScalar(nwc, tf);
	nbytes = (nwc+1) * sizeof(wchar_t);
	s = (wchar_t*) malloc(nbytes);
	assert(s != NULL);
	tfRead(tf, s, nbytes);
	return s;
}

#define readWstring(val, tf) val = readWstringF(tf)

/*
 * Allocate a buffer and copy the next size bytes from the template file
 * into it.  Return a pointer to the buffer.
 */
static void *
readBufferF(tfile_t *tf, size_t size)
{
	char *buf = (char*) malloc(size);
	assert(buf != NULL);
	assert(tf != NULL && tf->tf_next != NULL);
	assert(tf->tf_next + size <= tf->tf_end);
	memcpy(buf, tf->tf_next, size);
	tfAdvance(tf, size);
	return (void*) buf;
}
#define readBuffer(tf, pval, type, size) pval = (type) readBufferF(tf, size)

static void
writeAttRef(tmpl_attribute_t *att, template_t *t, tfile_t *tf)
{
	writeScalar(att->ta_index, tf);
}

static tmpl_attribute_t *
readAttRefF(template_t *t, tfile_t *tf)
{
	int attIndex;
	readScalar(attIndex, tf);
	return _evlTmplGetNthAttribute(t, attIndex);
}
#define readAttRef(att, t, tf) att = readAttRefF(t, tf);

/*
 * ref is a pointer to a struct template that is referenced by one of the
 * client template's attributes.  Find that struct template in the client's
 * import list, and write out the index of that struct ref.
 */
static void
writeTmplRef(template_t *ref, template_t *client, tfile_t *tf)
{
	evl_listnode_t *head = client->tm_imports, *p, *end;
	tmpl_struct_ref_t *sref;
	int i;

	for (i=0, end=NULL, p=head; p!=end; end=head, p=p->li_next, i++) {
		sref = (tmpl_struct_ref_t*) p->li_data;
		if (sref->sr_template == ref) {
			writeScalar(i, tf);
			return;
		}
	}
	fprintf(stderr, "Internal error writing %s: No struct ref for %s\n",
		tf->tf_path, ref->tm_name);
	tf->tf_errors++;
}

/*
 * Read in an integer and use that as an index into the client template's
 * import list.  Return a pointer to the referenced struct template.
 */
static template_t *
readTmplRefF(template_t *client, tfile_t *tf)
{
	int n;
	tmpl_struct_ref_t *sref;

	readScalar(n, tf);
	sref = (tmpl_struct_ref_t*) _evlGetNthValue(client->tm_imports, n);
	assert(sref != NULL);
	_evlTmplIncRef(sref->sr_template);
	return sref->sr_template;
}

#define readTmplRef(ref, client, tf) ref=readTmplRefF(client, tf)

static void
writeConstArrayOfStructs(tmpl_attribute_t *att, tfile_t *tf)
{
	evl_listnode_t *head, *end, *p;

	(void) writeConstAttDimension(att, tf);
	head = att->ta_value.val_list;
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		template_t *st = (template_t*) p->li_data;
		writeConstStruct(st, tf);
	}
}

static void
readConstArrayOfStructs(tmpl_attribute_t *att, tfile_t *tf)
{
	int nel = readConstAttDimension(att, tf);
	int i;
	evl_list_t *list = NULL;

	for (i = 1; i <= nel; i++) {
		template_t *st = _evlCloneTemplate(att->ta_type->u.st_template);
		readConstStruct(st, tf);
		list = _evlAppendToList(list, st);
	}
	att->ta_value.val_list = list;
}

#define TMPL_READING 1
#include "serialio.inc"
#undef TMPL_READING
#include "serialio.inc"

static void
writeAttributes(template_t *t, tfile_t *tf)
{
	evl_list_t *head = t->tm_attributes;
	evl_listnode_t *p, *end;
	int nAttrs = _evlGetListSize(head);

	writeScalar(nAttrs, tf);
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		writeAttribute((tmpl_attribute_t*) p->li_data, t, tf);
	}
}

static void
readAttributes(template_t *t, tfile_t *tf)
{
	int nAttrs, i;

	readScalar(nAttrs, tf);
	for (i = 0; i < nAttrs; i++) {
		tmpl_attribute_t *att = _evlTmplAllocAttribute();
		att->ta_index = i;
		readAttribute(att, t, tf);
		t->tm_attributes = _evlAppendToList(t->tm_attributes, att);
	}
}

static void
writeTmplFormat(template_t *t, tfile_t *tf)
{
	evl_list_t *head = t->tm_parsed_format;
	evl_listnode_t *p, *end;
	int nSegs = _evlGetListSize(head);

	writeScalar(nSegs, tf);
	for (p=head, end=NULL; p!=end; end=head, p=p->li_next) {
		writeFmtSegment((evl_fmt_segment_t*) p->li_data, t, tf);
	}
}

static void
readTmplFormat(template_t *t, tfile_t *tf)
{
	int nSegs, i;

	readScalar(nSegs, tf);
	for (i = 0; i < nSegs; i++) {
		evl_fmt_segment_t *seg = _evlAllocFormatSegment();
		readFmtSegment(seg, t, tf);
		t->tm_parsed_format = _evlAppendToList(t->tm_parsed_format, seg);
	}
}

static void
writeRedirectedAtt(struct redirectedAttribute *ra, int att, tfile_t *tf)
{
	if (ra) {
		writeScalar(ra->type, tf);
		switch (ra->type) {
		case TMPL_RD_NONE:
			break;
		case TMPL_RD_CONST:
			if (att == POSIX_LOG_ENTRY_FACILITY) {
				writeScalar(ra->u.faccode, tf);
			} else {
				writeScalar(ra->u.evtype, tf);
			}
			break;
		case TMPL_RD_ATTNAME:
			writeString(ra->u.attname, tf);
			break;
		}
	} else {
		enum redirection_type rdty = TMPL_RD_NONE;
		writeScalar(rdty, tf);
	}
}

static struct redirectedAttribute *
readRedirectedAtt(int att, tfile_t *tf)
{
	struct redirectedAttribute *ra;
	enum redirection_type rdty;

	readScalar(rdty, tf);
	if (rdty == TMPL_RD_NONE) {
		return NULL;
	}
	ra = (struct redirectedAttribute *)
		malloc(sizeof(struct redirectedAttribute));
	assert(ra != NULL);
	ra->type = rdty;
	switch (rdty) {
	case TMPL_RD_CONST:
		if (att == POSIX_LOG_ENTRY_FACILITY) {
			readScalar(ra->u.faccode, tf);
		} else {
			readScalar(ra->u.evtype, tf);
		}
		break;
	case TMPL_RD_ATTNAME:
		readString(ra->u.attname, tf);
		break;
  default: /* keep gcc happy */;
	}
	return ra;
}

static void
writeTmplRedirection(template_t *t, tfile_t *tf)
{
	tmpl_redirection_t *rd = t->tm_redirection;
	writeRedirectedAtt(rd->rd_fac, POSIX_LOG_ENTRY_FACILITY, tf);
	writeRedirectedAtt(rd->rd_evtype, POSIX_LOG_ENTRY_EVENT_TYPE, tf);

}

static void
readTmplRedirection(template_t *t, tfile_t *tf)
{
	tmpl_redirection_t *rd = (tmpl_redirection_t*)
		malloc(sizeof(tmpl_redirection_t));
	assert(rd != NULL);
	rd->rd_fac = readRedirectedAtt(POSIX_LOG_ENTRY_FACILITY, tf);
	rd->rd_evtype = readRedirectedAtt(POSIX_LOG_ENTRY_EVENT_TYPE, tf);
	t->tm_redirection = rd;
}

/*
 * Write out a string (e.g., "structName" or "dir/subdir/structName") for
 * each struct directly referenced by this template.
 */
static void
writeTemplateRefs(template_t *t, tfile_t *tf)
{
	evl_listnode_t *head = t->tm_imports, *p, *end;
	int nImports = _evlGetListSize(head);

	writeScalar(nImports, tf);
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		tmpl_struct_ref_t *sref = (tmpl_struct_ref_t*) p->li_data;
		writeString(sref->sr_path, tf);
	}
}

/*
 * Read in a string (e.g., "structName" or "dir/subdir/structName") for
 * each struct directly referenced by this template.  Ensure that each
 * indicated struct is imported.  Add corresponding struct refs to this
 * template's import list.
 *
 * Order is important here, because each struct-template ref later in the
 * file is encoded simply as an index into this list.
 *
 * Note that a per-template import list created in this way does NOT share
 * memory with the global import list that is created when a template source
 * file is compiled.  So we have to check the template's TMPL_TF_IMPORTED
 * flag when freeing the template.
 */
static void
readTemplateRefs(template_t *t, tfile_t *tf)
{
	int i, nImports;
	char *structPath;
	template_t *st;
	tmpl_struct_ref_t *sref;
	
	readScalar(nImports, tf);
	for (i = 0; i < nImports; i++) {
		readString(structPath, tf);
		st = _evlImportTemplate(tf->tf_dir, structPath, TMPL_IMPORT_BINARY);
		if (!st) {
			fprintf(stderr, "%s: Could not import %s\n",
				tf->tf_path, structPath);
			tf->tf_errors++;
			free(structPath);
			continue;
		}
		sref = _evlTmplMakeStructRef(st, structPath);
		t->tm_imports = _evlAppendToList(t->tm_imports, sref);
	}
}

static void
writeTemplate(const template_t *tc, tfile_t *tf)
{
	int magic = TMPL_MAGIC;

	/*
	 * We discard 'const' because the various I/O functions don't
	 * promise const (because they're 'I' as well as 'O').
	 */
	template_t *t = (template_t*) tc;

	writeScalar(magic, tf);
	writeHeader(t, tf);
	writeScalar(t->tm_flags, tf);
	writeScalar(t->tm_alignment, tf);
	writeScalar(t->tm_minsize, tf);
	writeTemplateRefs(t, tf);
	writeAttributes(t, tf);
	if (isRedirectTmpl(t)) {
		writeTmplRedirection(t, tf);
	}
	writeTmplFormat(t, tf);
}

static void
readTemplate(template_t *t, tfile_t *tf)
{
	int magic;

	readScalar(magic, tf);
	assert(magic == TMPL_MAGIC);	/* Already checked by caller */
	readHeader(t, tf);
	readScalar(t->tm_flags, tf);
	t->tm_flags |= TMPL_TF_IMPORTED;
	readScalar(t->tm_alignment, tf);
	readScalar(t->tm_minsize, tf);
	readTemplateRefs(t, tf);
	if (tf->tf_errors) {
		/* Failed to import all structs we ref.  Bail out now. */
		return;
	}
	readAttributes(t, tf);
	if (isRedirectTmpl(t)) {
		readTmplRedirection(t, tf);
	}
	readTmplFormat(t, tf);
}

/*
 * Allocate a buffer with the indicated size and a tfile object to manage it.
 */
static tfile_t *
allocTfile(const char *path, size_t nBytes)
{
	tfile_t *tf;
	char *buf;

	buf = (char*) malloc(nBytes);
	assert(buf != NULL);
	tf = (tfile_t*) malloc(sizeof(tfile_t));
	assert(tf != NULL);
	tf->tf_buf = buf;
	tf->tf_bufsz = nBytes;
	tf->tf_next = buf;
	tf->tf_end = buf + nBytes;
	tf->tf_dir = NULL;
	tf->tf_path = path;
	tf->tf_errors = 0;
	return tf;
}

static void
freeTfile(tfile_t *tf)
{
	free(tf->tf_buf);
	free(tf->tf_dir);
	free(tf);
}

/* Signal-safe read. */
static int
safeRead(int fd, void *buf, size_t len)
{
	int nBytes;
	do {
		nBytes = read(fd, buf, len);
	} while (nBytes < 0 && errno == EINTR);
	return nBytes;
}

/* Signal-safe write. */
static int
safeWrite(int fd, const void *buf, size_t len)
{
	int nBytes;
	do {
		nBytes = write(fd, buf, len);
	} while (nBytes < 0 && errno == EINTR);
	return nBytes;
}

int
_evlWriteTemplate(const template_t *t, const char *path)
{
	tfile_t *tf;
	size_t fileSize;
	int fd;
	int nBytes;

	tf = allocTfile(path, 1000);
	writeTemplate(t, tf);
	fileSize = tf->tf_next - tf->tf_buf;

	fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd < 0) {
		perror(path);
		freeTfile(tf);
		return -1;
	}
	nBytes = safeWrite(fd, tf->tf_buf, fileSize);
	assert(nBytes == fileSize);

	freeTfile(tf);
	(void) close(fd);
	return 0;
}

/*
 * Read a binary-encoded template from the indicated file, and return a
 * pointer to the newly created template object.
 */
template_t *
_evlReadTemplate(const char *path)
{
	int magic;
	int bytesRead, fileSize;
	int fd;
	struct stat stat;
	template_t *template;
	tfile_t *tf;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror(path);
		return NULL;
	}

	/*
	 * First read the file's magic number and verify that it's a
	 * template file.
	 */
	bytesRead = safeRead(fd, &magic, sizeof(magic));
	if (bytesRead < 0) {
		perror(path);
		goto failedEarly;
	}
	if (bytesRead < sizeof(magic) || magic != TMPL_MAGIC) {
		fprintf(stderr, "%s is not a template file", path);
		goto failedEarly;
	}
	(void) lseek(fd, 0, SEEK_SET);

	/*
	 * Now get the size of the file, allocate a buffer to hold it all,
	 * and read it into memory.
	 */
	if (fstat(fd, &stat) < 0) {
		perror("stat of template file");
		goto failedEarly;
	}
	fileSize = stat.st_size;
	tf = allocTfile(path, fileSize);
	assert(tf != NULL);

	bytesRead = safeRead(fd, tf->tf_buf, fileSize);
	assert(bytesRead == fileSize);
	(void) close(fd);

	tf->tf_dir = _evlGetParentDir(path);

	template = _evlAllocTemplate();
	readTemplate(template, tf);

	if (tf->tf_errors) {
		_evlFreeTemplate(template);
		freeTfile(tf);
		return NULL;
	}

	freeTfile(tf);
	return template;

failedEarly:
	(void) close(fd);
	return NULL;
}
