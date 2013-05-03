/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2002
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
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <byteswap.h>
#include "posix_evlog.h"
#include "evlog.h"
#include "rmt_common.h"

/* Trim leading space and trailing space */
void trim(const char * str)
{
	char const *psrc = str;
	char *pdes = (char *) str;
	int i, len;
	
	if (str == NULL || strlen(str) == 0) { 
		return;
	}
  	/* remove leading space */
	while(*psrc == ' ' || *psrc == '\t') {
		psrc++;
	}
	/* remove the trailing space */
	len = strlen(psrc);
	while (*(psrc+len-1) == ' ' || *(psrc+len-1) == '\t') {
		--len;
	}
	for (i=0 ; i < len; i++) {
		pdes[i] = psrc[i];
	}
	pdes[i] = '\0';
}


int getConfigStringParam(const char * configFile, const char * name, char * value, size_t vbufsize)
{
	char line[256];
	char *attname, *attvalue=NULL;
	char *tmp;
	FILE * f;
	
	if((f = fopen(configFile, "r")) == NULL) {
		fprintf(stderr, "can't open %s file.\n", configFile);
		return -1;
	}
	while (fgets(line, 256, f) != NULL) {
		int len;
		if (line[0] == '#' || line[0] == '\n' 
			|| strcspn(line, "") == 0) {
			continue;
		}
		len = strlen(line);
		/* replace newline with null char */
		if (line[len -1] == '\n') {
			line[len -1] = '\0';
		}
		attname = (char *) strtok(line, "=");
		trim(attname);
		if (!strcasecmp(attname, name)) {
			attvalue = (char *) strtok(NULL, "\n");
			trim(attvalue);
			strncpy(value, attvalue, vbufsize-1);
			value[vbufsize-1] = '\0';
			fclose(f);
			return 0;
		}
	}
	fclose(f);
	return -1;
}

int getConfigIntParam(const char * configFile, const char * name, int * value)
{
	char line[256];
	char *attname, *attvalue=NULL;
	char *tmp;
	FILE * f;
	
	if((f = fopen(configFile, "r")) == NULL) {
		fprintf(stderr, "can't open %s file.\n", configFile);
		return -1;
	}
	
	while (fgets(line, 256, f) != NULL) {
		int len;
		if (line[0] == '#' || line[0] == '\n' 
			|| strcspn(line, "") == 0) {
			continue;
		}
		len = strlen(line);
		/* replace newline with null char */
		if (line[len -1] == '\n') {
			line[len -1] = '\0';
		}
		attname = (char *) strtok(line, "=");
		trim(attname);
		if (!strcasecmp(attname, name)) {
			attvalue = (char *) strtok(NULL, "\n");
			trim(attvalue);
			*value = (int) strtol(attvalue, (char **)NULL, 0);
			fclose(f);
			return 0;
		}
	}
	fclose(f);
	return -1;
}

static long long htonlonglong(long long value) 
{
	unsigned int lo, hi;
	long long retvalue;

	if (htonl(1) == 1) 
		return value;
	lo = (unsigned int) (value & 0xFFFFFFFF);
	value >>= 32;
	hi = (unsigned int) (value & 0xFFFFFFFF);
	retvalue = htonl(lo);
	retvalue <<= 32;
	retvalue |= htonl(hi);
	return retvalue;
}

static long long ntohlonglong(long long value)
{
	unsigned int lo, hi;
	long long retvalue;


	if (htonl(1) == 1)
		return value;
	hi = (unsigned int) (value & 0xFFFFFFFF);
	value >>= 32;
	lo = (unsigned int) (value & 0xFFFFFFFF);
	retvalue = ntohl(hi);
	retvalue <<= 32;
	retvalue |= ntohl(lo);
	return retvalue;
}	


int hton_rechdr(struct posix_log_entry *src, struct net_rechdr *dest)
{
	int big_endian = 0;
	
	
	if (!src || !dest)
		return -1;

	dest->log_magic = htonl(src->log_magic);
	dest->log_recid = htonl(src->log_recid);
	dest->log_format = htonl(src->log_format);
	dest->log_event_type = htonl(src->log_event_type);
	dest->log_facility = htonl(src->log_facility);
	dest->log_severity = htonl(src->log_severity);
	dest->log_uid = htonl(src->log_uid);
	dest->log_gid = htonl(src->log_gid);
	dest->log_pid = htonl(src->log_pid);
	dest->log_pgrp = htonl(src->log_pgrp);
	dest->log_flags = htonl(src->log_flags);
	dest->log_processor = htonl(src->log_processor);

	dest->log_size = htonlonglong((long long) src->log_size);
	dest->log_time_tv_sec = htonlonglong((long long) src->log_time.tv_sec);
	dest->log_time_tv_nsec = htonlonglong((long long) src->log_time.tv_nsec);
	dest->log_thread = htonlonglong((long long) src->log_thread);
	
	return 0;
}

int ntoh_rechdr(struct net_rechdr *src, struct posix_log_entry *dest)
{
	int is64bit = 0;
	
	if (!src || !dest)
		return -1;

	if (sizeof(long) == 64)
		is64bit = 1;

	dest->log_magic = ntohl(src->log_magic);
	dest->log_recid = ntohl(src->log_recid);
	dest->log_format = ntohl(src->log_format);
	dest->log_event_type = ntohl(src->log_event_type);
	dest->log_facility = ntohl(src->log_facility);
	dest->log_severity = ntohl(src->log_severity);
	dest->log_uid = ntohl(src->log_uid);
	dest->log_gid = ntohl(src->log_gid);
	dest->log_pid = ntohl(src->log_pid);
	dest->log_pgrp = ntohl(src->log_pgrp);
	dest->log_flags = ntohl(src->log_flags);
	dest->log_processor = ntohl(src->log_processor);

	if (is64bit) {
		dest->log_size	= ntohlonglong(src->log_size);
		dest->log_time.tv_sec = ntohlonglong(src->log_time_tv_sec);
		dest->log_time.tv_nsec = ntohlonglong(src->log_time_tv_nsec);
		dest->log_thread = ntohlonglong(src->log_thread);	
	} else {
		dest->log_size	= (long) ntohlonglong(src->log_size);
		dest->log_time.tv_sec = (long) ntohlonglong(src->log_time_tv_sec);
		dest->log_time.tv_nsec = (long) ntohlonglong(src->log_time_tv_nsec);
		dest->log_thread = (long) ntohlonglong(src->log_thread);
	}
	
	return 0;
}

