/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2001
 *
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
 *
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
// #include <wchar.h>
// #include <endian.h>
#include <byteswap.h>
#include <wchar.h>
#include "config.h"
#include <evlog.h>

#include "evl_util.h"
#include "evl_template.h"

/*
 * There are few assumptions:
 * 1. For all 32-bit architectures supported, all pointers
 *    and (unsigned) long are 32 bits. For all 64-bit
 *    architectures, they are 64 bits.
 * 2. For all other integral data types, sizes are constant
 *    across all known architectures.
 * 3. long doubles are special 
 * 4. All big endian architectures use the same format for 
 *    long double.
 */

typedef enum conv_{
	NO_CONV,
	BS_32_32,		/* byteswap 32->32 	*/
	BS_32_64,
	BS_64_32,
	BS_64_64,
	LE_32_64,		/* Intel 32->64 	*/
	LE_64_32,
	BE_32_64,		/* IBM 32->64 		*/
	BE_64_32
} conv_t;

extern tmpl_arch_type_info_t _evlTmplArchTypeInfo[8][24];
/*
 * The first element of _evlTmplArchTypeInfo[0]... is always compiled
 * with local architecture, so it should contains local architecture
 * type information.
 */
#define	COMPILED_ARCH 	0


#if defined(__ppc__) || defined(__powerpc__)
	#define LOCAL_ARCH 		LOGREC_ARCH_PPC
#elif defined(__i386__)
	#define LOCAL_ARCH		LOGREC_ARCH_I386
#elif defined(__ia64__)
	#define LOCAL_ARCH		LOGREC_ARCH_IA64
#elif defined(__s390__) 
	#define LOCAL_ARCH 		LOGREC_ARCH_S390
#elif defined(__s390x__)
	#define LOCAL_ARCH		LOGREC_ARCH_S390X
#elif defined(__x86_64__)
	#define LOCAL_ARCH		LOGREC_ARCH_X86_64
#elif defined(__arm__) && defined(__ARMEB__)
	#define LOCAL_ARCH		LOGREC_ARCH_ARM_BE
#elif defined(__arm__) && !defined(__ARMEB__)
	#define LOCAL_ARCH		LOGREC_ARCH_ARM_LE
#endif

/* 
 * Two dimmension array to hold conversion type
 * The 1st subscript represents the log record archicture (remote event)
 * The 2nd subscript represents the local architecture (local host) 
 */
conv_t conv_info[][10] = {
	   /*   I386      IA64      S390      S390X     PPC       PPC64	    X86_64    ARM_LE    ARM_BE  */
/* NOARCH*/{ 0, 0,        0,        0,        0,        0,        0,        0,	      0,        0 },
/* I386  */{ 0, NO_CONV,  LE_32_64, BS_32_32, BS_32_64, BS_32_32, BS_32_64, LE_32_64, NO_CONV,  BS_32_32 },
/* IA64  */{ 0, LE_64_32, NO_CONV,  BS_64_32, BS_64_64, BS_64_32, BS_64_64, NO_CONV,  LE_64_32, BS_64_32 },
/* S390  */{ 0, BS_32_32, BS_32_64, NO_CONV,  BE_32_64, NO_CONV,  BE_32_64, BS_32_64, BS_32_32, NO_CONV },
/* S390X */{ 0, BS_64_32, BS_64_64, BE_64_32, NO_CONV,  BE_64_32, NO_CONV,  BS_64_64, BS_64_32, BE_64_32 },
/* PPC   */{ 0, BS_32_32, BS_32_64, NO_CONV,  BE_32_64, NO_CONV,  BE_32_64, BS_32_64, BS_32_32, NO_CONV },
/* PPC64 */{ 0, BS_64_32, BS_64_64, BE_64_32, NO_CONV,  BE_64_32, NO_CONV,  BS_64_64, BS_64_32, BE_64_32 },
/* X86_64*/{ 0, LE_64_32, NO_CONV,  BS_64_32, BS_64_64, BS_64_32, BS_64_64, NO_CONV,  LE_64_32, BE_64_32 },
/* ARM_BE*/{ 0, BS_32_32, BS_32_64, NO_CONV,  BE_32_64, NO_CONV,  BE_32_64, BS_32_64, BS_32_32, NO_CONV },
/* ARM_LE*/{ 0, NO_CONV,  LE_32_64, BS_32_32, BS_32_64, BS_32_32, BS_32_64, LE_32_64, NO_CONV,  BS_32_32 },
};

static void
byteswap(void *data, int nbytes)
{
	int i, j;
	char tmp, *d = (char*) data;
	for (i=0, j=nbytes-1; i<j; i++, j--) {
		tmp = d[i];
		d[i] = d[j];
		d[j] = tmp;
	}
}
/*
 * This function converts 10-byte (12-byte, or 16-btye on IA64)
 * long double IEEE754 extended precision (15 bit exponent) from a Intel
 * machine to a 8-byte double (11-bit exponent) for a non-Intel architecture. 
 *  
 */ 
static double 
intel_ldouble2ppc_double(unsigned int arr[])
{
	unsigned int dbl[2];
	unsigned int exp;
	unsigned int sign;
	double d;
	unsigned int array[3];

	array[0] = bswap_32(arr[0]);
	array[1] = bswap_32(arr[1]);
	array[2] = bswap_32(arr[2]);

	exp = array[2] & 0x7fff; /* 15-bit exp */
	sign = array[2] & 0x8000;
	if (exp == 0x7fff) { /* NaN on extended */
		dbl[1] = (unsigned int) -1;
		dbl[0] = 0x7ff00000;
	}
	else if ((((int) exp - 16383) + 1023) >= 0x7ff) { /* Infinite on double */
		if (sign) {
			dbl[0] = 0xfff00000;
		}
		else { 	  
			dbl[0] = 0x7ff00000;
		}
		dbl[1] = 0; 
	}
	else {
		exp = (exp -16383) + 1023;
		dbl[0] = exp << 20;
		dbl[0] |= sign ? 0x80000000 : 0;
		dbl[0] |= (array[1] >> 11) & 0x000fffff;
		dbl[1] = (array[1] << 21) | (array[0] >> 11);
	}
	return *(double*) dbl;
}

/*
 * Convert long double intel -> ppc
 */
static void
conv_ia2ppc_ldouble(void * data)
{
	unsigned int array[3];
	/* A long double on Intel is 12 bytes */
	/* only ten bytes is significant      */
	memcpy(array, data, 3 * sizeof(int));
	/* Intel record data to IBM local */
	*(long double *) data  = (long double) intel_ldouble2ppc_double(array);
}

/*
 * Convert long double ppc -> intel
 */
static void
conv_ppc2ia_ldouble(void *data)
{
	/* long double on PPC is only 8 bytes  */
	double d = *(double  *) data;
	/* IBM record data to Intel local */
	byteswap(&d, sizeof(d));
	*(long double *) data = (long double) d;
}

/*
 * This function performs the long double conversion from Intel->IBM or 
 * IBM->Intel.
 */
static void
conv_swap_ldouble(int log_arch, void *data)
{
	if (log_arch == LOGREC_ARCH_I386 ||
	    log_arch == LOGREC_ARCH_IA64 ||
	    log_arch == LOGREC_ARCH_IA64 ||
	    log_arch == LOGREC_ARCH_ARM_LE) {
		conv_ia2ppc_ldouble(data);
	} else if (log_arch == LOGREC_ARCH_PPC ||
		   log_arch == LOGREC_ARCH_PPC64 ||
		   log_arch == LOGREC_ARCH_S390 ||
		   log_arch == LOGREC_ARCH_S390X ||
		   log_arch == LOGREC_ARCH_ARM_BE) {
		conv_ppc2ia_ldouble(data);
	}

}


static void
conv_swap_long(int log_arch, tmpl_base_type_t type, void *data, 
		conv_t conv_type)
{
	size_t src_size = _evlTmplArchTypeInfo[log_arch][type].ti_size;
	size_t dest_size = _evlTmplArchTypeInfo[COMPILED_ARCH][type].ti_size;
	unsigned long tmp[2];

	switch(conv_type) {
		case BS_32_32:
		case BS_64_64:
			byteswap(data, dest_size);
			break;
		case BS_32_64:
		case BS_64_32:
			memset(tmp, 0, 2 * sizeof(long));
			memcpy(tmp, data, src_size);
			byteswap(tmp, src_size);
			memset(data, 0, src_size);
			if ((log_arch == LOGREC_ARCH_IA64 ||
			     log_arch == LOGREC_ARCH_X86_64)) { 
				memcpy(data, &(tmp[1]), dest_size);
			} else {
				memcpy(data, tmp, dest_size);
			}
			break;
		default:
			printf("default switch case %s\n", __FUNCTION__);
			break;
	}
}

/**************************************************************
 * Scalar 
 **************************************************************/

static void 
__conv_scalar(int log_arch, tmpl_base_type_t type, void *data, 
		conv_t conv_type)
{
	size_t src_size = _evlTmplArchTypeInfo[log_arch][type].ti_size;
	size_t dest_size = _evlTmplArchTypeInfo[COMPILED_ARCH][type].ti_size;
	
	unsigned long tmp[2];

	if (conv_type == NO_CONV) {
		return;
	}
	if (conv_type == BS_32_32 || conv_type == BS_32_64 
		|| conv_type == BS_64_32 || conv_type == BS_64_64) {
		switch(type)
		{
			case TY_ULONG:
			case TY_LONG:
			case TY_ADDRESS:
				conv_swap_long(log_arch, type, data, conv_type);
				break;
			case TY_LDOUBLE:
				conv_swap_ldouble(log_arch, data);
				break;
			default:
				byteswap(data, src_size);
				break;
		}
	}
	else if (type == TY_ULONG || type == TY_LONG || type == TY_ADDRESS) {
		memset(tmp, 0, 2 * sizeof(long));
		memcpy(tmp, data, src_size);	
		memset(data, 0, src_size);
		if (conv_type == LE_32_64 || conv_type == LE_64_32) {
			memcpy(data, tmp, dest_size);
		}
		else if (conv_type == BE_32_64) {
			memcpy((char *)data + sizeof(long), tmp, dest_size);
		}
		else if (conv_type == BE_64_32) {
			memcpy(data, &(tmp[1]), dest_size);
		}	
	}	
}

/**************************************************************
 * Array of scalar 
 **************************************************************/
static void *
__conv_scalar_array(int log_arch, tmpl_base_type_t type, char * data,
                                        int nel, int el_size, conv_t conv_type)
{
	unsigned int buf[4];
	int i, src_offset=0, dest_offset=0;
	char *dest;
	size_t dest_type_size = (int)_evlTmplArchTypeInfo[COMPILED_ARCH][type].ti_size;	
	dest = calloc(nel, dest_type_size);
	assert(dest);
	for (i=0; i < nel; i++) {
		memcpy(buf, data + src_offset, el_size);
		__conv_scalar(log_arch, type, buf, conv_type);
		memcpy(dest + dest_offset, buf, dest_type_size);
		src_offset += el_size;
		dest_offset += dest_type_size;	
	}	
	return dest;	
}

/***************************************************************
 * APIs 
 ***************************************************************/

void 
_evl_conv_scalar(int arch, tmpl_base_type_t type,  void *data)
{
	switch (arch) {
	/* Supported architectures */
	case LOGREC_ARCH_I386:
	case LOGREC_ARCH_IA64:
	case LOGREC_ARCH_S390:
	case LOGREC_ARCH_S390X:
	case LOGREC_ARCH_PPC:
	case LOGREC_ARCH_PPC64:
	case LOGREC_ARCH_X86_64:
	case LOGREC_ARCH_ARM_BE:
	case LOGREC_ARCH_ARM_LE:
		__conv_scalar(arch, type, data, conv_info[arch][LOCAL_ARCH]);
	default:
		break;
	}
}

void *
_evl_conv_scalar_array(int arch, tmpl_base_type_t type, char * data, int nel, int el_size)
{
	void *ptr;
	switch (arch) {
	/* Supported architectures */
	case LOGREC_ARCH_I386:
	case LOGREC_ARCH_IA64:
	case LOGREC_ARCH_S390:
	case LOGREC_ARCH_S390X:
	case LOGREC_ARCH_PPC:
	case LOGREC_ARCH_PPC64:
	case LOGREC_ARCH_X86_64:
	case LOGREC_ARCH_ARM_BE:
	case LOGREC_ARCH_ARM_LE:
		ptr = __conv_scalar_array(arch, type, data, nel, el_size, 
				conv_info[arch][LOCAL_ARCH]);
		break;
	default:
		ptr = data;
		break;
	}
	return ptr;
}

char  *
_evl_conv_wstring(int arch, char * data)
{
	conv_t conv_type = conv_info[arch][LOCAL_ARCH];
	wchar_t wcsz = sizeof(wchar_t);
	size_t maxlen = div(POSIX_LOG_ENTRY_MAXLEN, sizeof(wchar_t)).quot;
	int len = wcsnlen((wchar_t *) data, maxlen) + 1;
	char * p;
	
	if (conv_type == BS_32_32 || conv_type == BS_32_64 ||
		conv_type == BS_64_32 || conv_type == BS_64_64) {
		p = __conv_scalar_array(arch, TY_WCHAR, data, len, wcsz, conv_type);
		return p;
	}		
			
	return data;
}

char *
_evl_conv_wstring_array(int arch, char *data, int nel)
{
	conv_t conv_type = conv_info[arch][LOCAL_ARCH];
	char *dest, *converted_ws;
	int ns, numchars, offset = 0;
	size_t len, wcsz = sizeof(wchar_t);
	size_t src_wcsz = _evlTmplArchTypeInfo[arch][TY_WCHAR].ti_size;
	if (conv_type == LE_32_64 || conv_type == LE_64_32 ||
		conv_type == BE_32_64 || conv_type == BE_64_32 || 
		conv_type ==NO_CONV) {
		return data;
	}
	
	/* Figure out whole array size */
	for (ns=0; ns < nel; ns++) {
		len = wcslen((wchar_t *) (data + offset));
		offset +=(len + 1) * src_wcsz;
	}
	dest = malloc(offset);
	assert(dest);	

	offset = 0;
	for (ns=0 ; ns < nel; ns++)	{	
		/* Now we got an element - convert it */
		converted_ws = _evl_conv_wstring(arch, data + offset);  
		len = wcslen((wchar_t *) (data + offset)) + 1 ;		
		memcpy(dest + offset, converted_ws, len * wcsz);
			 
		if ( converted_ws != (data + offset)) {
			free(converted_ws);
		}
		offset +=len * src_wcsz;
	}
	
	return dest;
}


