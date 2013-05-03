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
 * This file is included twice in serial.c: once with TMPL_READING defined,
 * to produce functions that can be used for reading, and once with it
 * undefined, to produce functions that can be used for writing.
 */

#ifdef TMPL_READING
#define ioScalar	readScalar
#define ioString	readString
#define ioStringOrNull	readStringOrNull
#define ioWstring	readWstring
#define ioHeader	readHeader
#define ioAttType	readAttType
#define ioConstAttDimension readConstAttDimension
#define ioAttDimension	readAttDimension
#define ioAttFormat	readAttFormat
#define ioAttDelimiter	readAttDelimiter
#define ioConstStruct	readConstStruct
#define ioConstArrayOfStructs	readConstArrayOfStructs
#define ioAttValue	readAttValue
#define ioBuffer	readBuffer
#define ioAttribute	readAttribute
#define ioAttRef	readAttRef
#define ioFmtSegment	readFmtSegment
#define ioTmplRef	readTmplRef
#else
#define ioScalar	writeScalar
#define ioString	writeString
#define ioStringOrNull	writeStringOrNull
#define ioWstring	writeWstring
#define ioHeader	writeHeader
#define ioAttType	writeAttType
#define ioConstAttDimension writeConstAttDimension
#define ioAttDimension	writeAttDimension
#define ioAttFormat	writeAttFormat
#define ioAttDelimiter	writeAttDelimiter
#define ioConstStruct	writeConstStruct
#define ioConstArrayOfStructs	writeConstArrayOfStructs
#define ioAttValue	writeAttValue
#define ioBuffer(tf, buf, type, size) tfWrite(tf, buf, size)
#define ioAttribute	writeAttribute
#define ioAttRef	writeAttRef
#define ioFmtSegment	writeFmtSegment
#define ioTmplRef	writeTmplRef
#endif

/* Read or write a template header. */
static void
ioHeader(template_t *t, tfile_t *tf)
{
	tmpl_header_t *th = &t->tm_header;
	ioScalar(th->th_type, tf);
	ioStringOrNull(th->th_description, tf);

	switch (th->th_type) {
	case TMPL_TH_EVLOG:
		ioScalar(th->u.u_evl.evl_facility, tf);
		ioScalar(th->u.u_evl.evl_event_type, tf);
		ioString(t->tm_name, tf);
		break;
	case TMPL_TH_STRUCT:
		ioString(th->u.u_struct.st_name, tf);
#ifdef TMPL_READING
		t->tm_name = th->u.u_struct.st_name;
#endif
		break;
	case TMPL_TH_TRACE:
		ioScalar(th->u.u_trace.tra_major, tf);
		ioScalar(th->u.u_trace.tra_minor, tf);
		break;
	}
}

/*
 * att is an array.  If the EVL_ATTR_IMPLDIM flag is set, the dimension
 * type is TMPL_DIM_IMPLIED in the struct's .to file, but TMPL_DIM_CONST
 * in a template that contains a const version of the struct.  So td_dimension
 * has the actual dimension when we're writing, but not when we're reading.
 * Sample template where we see this:
 *	struct s;
 *	attributes { int a[]; }
 *	format string "%a%"
 *	END
 *	facility "f"; event_type 1;
 *	const { struct s sa = {{1, 2}}; }
 *	...
 */
static int
ioConstAttDimension(tmpl_attribute_t *att, tfile_t *tf)
{
	tmpl_dimension_t *dim = att->ta_dimension;
	assert(dim != NULL);
	if ((att->ta_flags & EVL_ATTR_IMPLDIM) != 0) {
		ioScalar(dim->td_dimension, tf);
#ifdef TMPL_READING
		dim->td_dimension2 = dim->td_dimension;
		dim->td_type = TMPL_DIM_CONST;
#endif
	}
	return dim->td_dimension;
}

/* Read or write an attribute's type. */
static void
ioAttType(tmpl_attribute_t *att, template_t *t, tfile_t *tf)
{
#ifdef TMPL_READING
	att->ta_type = _evlTmplAllocDataType();
#endif
	ioScalar(att->ta_type->tt_base_type, tf);
	if (att->ta_type->tt_base_type == TY_STRUCT) {
		ioTmplRef(att->ta_type->u.st_template, t, tf);
	}
}

/* Read or write an attribute's dimension. */
static void
ioAttDimension(tmpl_attribute_t *att, template_t *t, tfile_t *tf)
{
	tmpl_dimension_type_t dimType = TMPL_DIM_NONE;
	tmpl_dimension_t *dim;
#ifdef TMPL_READING
	readScalar(dimType, tf);
	if (dimType == TMPL_DIM_NONE) {
		att->ta_dimension = NULL;
		return;
	}
	dim = _evlTmplAllocDimension();
	att->ta_dimension = dim;
	dim->td_type = dimType;
#else
	dim = att->ta_dimension;
	dimType = (dim ? dim->td_type : TMPL_DIM_NONE);
	writeScalar(dimType, tf);
	if (!dim) {
		return;
	}
#endif
	switch (dimType) {
	case TMPL_DIM_CONST:
		ioScalar(dim->td_dimension, tf);
#ifdef TMPL_READING
		if (isConstAtt(att)) {
			dim->td_dimension2 = dim->td_dimension;
		}
#endif
		break;
	case TMPL_DIM_REST:
	case TMPL_DIM_IMPLIED:
		break;
	case TMPL_DIM_ATTR:
		ioAttRef(dim->u.u_attribute, t, tf);
		break;
	case TMPL_DIM_ATTNAME:
		ioString(dim->u.u_attname, tf);
#ifdef TMPL_READING
	    {
		tmpl_attribute_t *dimAtt;
		dimAtt = _evlTmplFindAttribute(t, dim->u.u_attname);
		if (!dimAtt || !isIntegerAtt(dimAtt)) {
			fprintf(stderr, "Error reading %s: %s is invalid dimension attribute\n",
				tf->tf_path, dim->u.u_attname);
			tf->tf_errors++;
		}
	    }
#endif
		break;
	case TMPL_DIM_BITFIELD:
		ioScalar(dim->td_dimension, tf);
		ioScalar(dim->td_dimension2, tf);
		break;
	default:
		assert(0);
	}
}

/* Read or write an attribute's format. */
static void
ioAttFormat(tmpl_attribute_t *att, template_t *t, tfile_t *tf)
{
	ioScalar(att->ta_format.af_type, tf);
#ifdef TMPL_READING
	att->ta_format.af_shared = 0;
#endif
	switch(att->ta_format.af_type) {
	case TMPL_AFS_DEFAULT:
	case TMPL_AFS_DUMP:
		break;
	case TMPL_AFS_SCALAR:
	case TMPL_AFS_ARRAY:
	case TMPL_AFS_CHARRSTR:
		ioString(att->ta_format.af_format, tf);
		break;
	case TMPL_AFS_IZARRAY:
	    {
		int i;
		ioString(att->ta_format.af_format, tf);
		for (i = 0; i < 3; i++) {
			ioScalar(att->ta_format.u.u_izorder[i], tf);
		}
		break;
	    }
	case TMPL_AFS_BITMAP:
	case TMPL_AFS_VALNM:
	    {
		int i, nBitmaps = 0;
		tmpl_bitmap_t *bmap, *bm;
#ifdef TMPL_READING
		/* Read the bitmap count and allocate memory for the array. */
		readScalar(nBitmaps, tf);
		bmap = (tmpl_bitmap_t*) malloc((nBitmaps+1)
			* sizeof(tmpl_bitmap_t));
		assert(bmap != NULL);
		att->ta_format.u.u_bitmaps = bmap;
		bmap[nBitmaps].bm_name = NULL;
#else
		bmap = att->ta_format.u.u_bitmaps;
		/*
		 * Count bitmaps and write the count, so reader knows how
		 * much to malloc.
		 */
		for (bm = bmap; bm->bm_name; bm++) {
			nBitmaps++;
		}
		writeScalar(nBitmaps, tf);
#endif
		for (i = 0; i < nBitmaps; i++) {
			ioScalar(bmap[i].bm_1bits, tf);
			ioScalar(bmap[i].bm_0bits, tf);
			ioString(bmap[i].bm_name, tf);
		}
		break;
	    }
        default: /* keep gcc happy */ ;
	}
}

/* Read or write an array attribute's delimiter. */
static void
ioAttDelimiter(tmpl_attribute_t *att, template_t *t, tfile_t *tf)
{
	tmpl_delim_type_t deType = TMPL_DELIM_NONE;
	tmpl_delimiter_t *de;

	if (!isArray(att)) {
		return;
	}
#ifdef TMPL_READING
	readScalar(deType, tf);
	if (deType == TMPL_DELIM_NONE) {
		att->ta_delimiter = NULL;
		return;
	}
	de = _evlTmplAllocDelimiter();
	att->ta_delimiter = de;
	de->de_type = deType;
#else
	de = att->ta_delimiter;
	deType = (de ? de->de_type : TMPL_DELIM_NONE);
	writeScalar(deType, tf);
	if (!de) {
		return;
	}
#endif
	switch (deType) {
	case TMPL_DELIM_CONST:
		ioString(de->de_delimiter, tf);
		break;
	case TMPL_DELIM_ATTR:
		ioAttRef(de->u.u_attribute, t, tf);
		break;
	case TMPL_DELIM_ATTNAME:
		ioString(de->u.u_attname, tf);
		break;
	default: 
		assert(0);
	}
}

static void ioAttValue(tmpl_attribute_t *att, template_t *t, tfile_t *tf);

/* Read or write the values of a const struct, whose template (clone) is t. */
static void
ioConstStruct(template_t *t, tfile_t *tf)
{
	evl_listnode_t *head, *end, *p;

	head = t->tm_attributes;
	for (end=NULL, p=head; p!=end; end=head, p=p->li_next) {
		tmpl_attribute_t *att = (tmpl_attribute_t*) p->li_data;
		att->ta_flags |= (EVL_ATTR_CONST|EVL_ATTR_EXISTS);
		ioAttValue(att, t, tf);
	}
#ifdef TMPL_READING
	t->tm_flags |= TMPL_TF_POPULATED;
#endif
}

/* Read or write a const attribute's value(s). */
static void
ioAttValue(tmpl_attribute_t *att, template_t *t, tfile_t *tf)
{
	if (!isConstAtt(att)) {
		return;
	}

	if (isStruct(att)) {
		template_t *master = att->ta_type->u.st_template;
		if (isArray(att)) {
			ioConstArrayOfStructs(att, tf);
		} else if (isConstStructTmpl(master)) {
#ifdef TMPL_READING
			_evlTmplIncRef(master);
			att->ta_value.val_struct = master;
#endif
		} else {
#ifdef TMPL_READING
			att->ta_value.val_struct = _evlCloneTemplate(master);
#endif
			ioConstStruct(att->ta_value.val_struct, tf);
		}
	} else if (isArray(att)) {
		/* Array of strings or scalars */
		int nel = ioConstAttDimension(att, tf);
#ifdef TMPL_READING
		char *arr = tf->tf_next;
#else
		char *arr = (char*) att->ta_value.val_array;
#endif

		switch(baseType(att)) {
		case TY_STRING:
		    {
			char *s = arr;
			int i;
			for (i = 0; i < nel; i++) {
				s += strlen(s) + 1;
			}
			ioBuffer(tf, att->ta_value.val_array, void*, s - arr);
			break;
		    }
		case TY_WSTRING:
		    {
			wchar_t *s = (wchar_t*) arr;
			int i;
			for (i = 0; i < nel; i++) {
				s += wcslen(s) + 1;
			}
			ioBuffer(tf, att->ta_value.val_array, void*,
				(char*)s - arr);
			break;
		    }
		default:
			ioBuffer(tf, att->ta_value.val_array, void*,
				nel * _evlTmplTypeInfo[baseType(att)].ti_size);
			break;
		}
	} else {
		/* Simple value */
		switch(baseType(att)) {
		case TY_STRING:
			ioString(att->ta_value.val_string, tf);
			break;
		case TY_WSTRING:
			ioWstring(att->ta_value.val_wstring, tf);
			break;
		default:
			ioScalar(att->ta_value, tf);	/* sic */
			break;
		}
	}
}

/* Read or write an individual attribute. */
static void
ioAttribute(tmpl_attribute_t *att, template_t *t, tfile_t *tf)
{
	ioStringOrNull(att->ta_name, tf);
	ioScalar(att->ta_flags, tf);
	ioAttType(att, t, tf);
	ioAttDimension(att, t, tf);
	ioAttDelimiter(att, t, tf);
	ioAttFormat(att, t, tf);
	ioAttDimension(att, t, tf);
	ioAttValue(att, t, tf);
}

/* Read or write a segment of the template's parsed format. */
static void
ioFmtSegment(evl_fmt_segment_t *seg, template_t *t, tfile_t *tf)
{
	ioScalar(seg->fs_type, tf);
	switch (seg->fs_type) {
	case EVL_FS_STRING:
		ioString(seg->u.fs_string, tf);
		break;
	case EVL_FS_MEMBER:
		ioScalar(seg->u2.fs_member, tf);
		ioStringOrNull(seg->fs_userfmt, tf);
		break;
	case EVL_FS_ATTR:
		ioAttRef(seg->u2.fs_attribute, t, tf);
		ioStringOrNull(seg->fs_userfmt, tf);
		break;
	case EVL_FS_ATTNAME:
		ioString(seg->u.fs_attname, tf);
		ioStringOrNull(seg->fs_userfmt, tf);
		break;
	default:
		assert(0);
	}
}

#undef ioScalar
#undef ioString
#undef ioStringOrNull
#undef ioWstring
#undef ioHeader
#undef ioAttType
#undef ioConstAttDimension
#undef ioAttDimension
#undef ioAttFormat
#undef ioAttDelimiter
#undef ioConstStruct
#undef ioConstArrayOfStructs
#undef ioAttValue
#undef ioBuffer
#undef ioAttribute
#undef ioAttRef
#undef ioFmtSegment
#undef ioTmplRef
