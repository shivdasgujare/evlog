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
#include <fcntl.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <values.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>		/* Defines bcopy() */
#include <sys/stat.h>
#include <time.h>
#include <assert.h>
#include <zlib.h>

#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evl_logmgmt.h"
#include "evlog.h"


//#define __CRASH_TEST__
//#define DEBUG2
#ifdef DEBUG2
#define TRACE(fmt, args...)		fprintf(stderr, fmt, ##args)
#else
#define TRACE(fmt, args...)		/* fprintf(stderr, fmt, ##args) */
#endif

#define LOGREC_MAGIC_HIWORD	(LOGREC_MAGIC & 0xffff0000)

typedef enum set_gen { GEN_RESET, GEN_INCR, GEN_EVEN, GEN_ODD } set_gen_t;


struct file_offset_info {
	off_t offset;
	off_t beginhole;
	off_t bytes_to_process;
};

static char logmgmt_msg[128]="";
static int g_num_delrec = 0;
static size_t reducedBytes = 0;
static posix_log_query_t query;
char error_str[256];
static int logfd;
static int g_backupCreated = 0;
static size_t sizeb4repair;
extern int gzBackUpFlag;


/*
 * Valdiates the log record that is pointed at by offset.
 * Returns the file pointer to the next record if the current record validated,
 * otherwise return -1.
 */
off_t
validateRec(off_t offset)
{
	struct posix_log_entry rechdr;
	evlrecsize_t recsize;

	if (lseek(logfd, offset, SEEK_SET) == (off_t) -1) {
		TRACE("lseek failed.\n");
		return -1;
	}
	if (read(logfd, &rechdr, REC_HDR_SIZE) != REC_HDR_SIZE ) {
		TRACE("read recheader failed.\n");
		return -1;
	}
	if (lseek(logfd, offset + rechdr.log_size + REC_HDR_SIZE, SEEK_SET) == (off_t) -1) {
		TRACE("lseek failed.\n");
		return -1;
	}
	if (read(logfd, &recsize, sizeof(evlrecsize_t)) != sizeof(evlrecsize_t)) {
		TRACE("read recsize at end of record failed.\n");
		return -1;
	}
	if (((rechdr.log_magic & 0xffff0000) == LOGREC_MAGIC_HIWORD) && 
		 (rechdr.log_size + REC_HDR_SIZE) == recsize) {
		
		offset += (recsize + sizeof(evlrecsize_t)); 
		return offset;
	}
	return -1;
}

/*
 * Returns the entire record size (including the evlrecsize at the end
 * of the record)
 */
int
getRecLen(const struct posix_log_entry *p_rhdr)
{
	if ((p_rhdr->log_magic & 0xffff0000) == LOGREC_MAGIC_HIWORD) {
		return p_rhdr->log_size + REC_HDR_SIZE + sizeof(evlrecsize_t);
	}
	else 
		return -1;
}

/* 
 * Verify that record is matched the deletion query - also we don't want
 * to remove the mgmt message we wrote to the log. 
 */
int
doesRecMatchForDel(const struct posix_log_entry *p_rhdr, const char * varbuf)
{
	if (_evlEvaluateQuery(&query, p_rhdr, (char *) varbuf) &&
				((strcmp((char *)logmgmt_msg, (char *) varbuf) != 0))) {
		g_num_delrec++;
		reducedBytes +=(REC_HDR_SIZE + p_rhdr->log_size + sizeof(evlrecsize_t));
		return 1;
	}
	return 0;
}


int 
moveRecordUp(struct file_offset_info *fi, 
			const struct posix_log_entry *rechdr, const char * buf)
{
	char buff[POSIX_LOG_ENTRY_MAXLEN + sizeof(evlrecsize_t)];	 
	size_t remainder = rechdr->log_size + sizeof(evlrecsize_t);
	const char * varbuf;

	if (rechdr->log_size > POSIX_LOG_ENTRY_MAXLEN) {
		TRACE("Record size (%zu) exceeds %u\n", rechdr->log_size, 
			POSIX_LOG_ENTRY_MAXLEN);
		return -1;
	}
	if (!buf) {
		if (read(logfd, &buff, remainder) != remainder) {
			TRACE("Failed to read remainder of record to be moved.\n");
			return -1;
		}
		varbuf = buff;
	}
	else {
		varbuf = buf;
	} 

	if (lseek(logfd, fi->beginhole, SEEK_SET) == (off_t)-1) {
		TRACE("Failed to seek to beginning of the hole.\n");
		return -1;
	}
	if (write(logfd, rechdr, REC_HDR_SIZE) != REC_HDR_SIZE) {
		TRACE("Failed to write recheader failed\n");
		return -1;
	}
	
	if (write(logfd, varbuf, remainder) != remainder) {
		TRACE("write varbuf failed\n");
		return -1;
	}
	fi->beginhole += REC_HDR_SIZE + remainder;
	return 0;
}
/* 
 * Walk the log and count records that match the query for deletion. 
 * Assume that the log file is opened and exclusively locked.
 */ 
int countRecsForDeletion(posix_log_query_t *query, int * num_delrec, size_t *delbytes)
{
	size_t bytes_to_process;
	size_t reclen, offset;
	evlrecsize_t evlrecsize;
	struct posix_log_entry rechdr;
	char varbuf[POSIX_LOG_ENTRY_MAXLEN + 1];
	size_t nbytes = 0;
	int ret = 0, delcnt = 0, reccnt = 0;
	struct stat st;
	
	st.st_size = 0;
	
	if (fstat(logfd, &st) || !st.st_size) {
		return EBADF;
	}
	TRACE("size = %u\n", st.st_size);
	offset = sizeof(log_header_t);		/* Do not process the log header */
	bytes_to_process = st.st_size - offset;
	
	while(bytes_to_process > 0) {
		if(lseek(logfd, offset, SEEK_SET) == (off_t) -1) {
			return EIO;
		}
		if (read(logfd, &rechdr, REC_HDR_SIZE) != REC_HDR_SIZE) {
			return EIO;
		}
		
		if (rechdr.log_size > 0 && rechdr.log_size <= POSIX_LOG_ENTRY_MAXLEN) {
			if(read(logfd, &varbuf, rechdr.log_size) != rechdr.log_size) {
				return EIO;	
			}
		} 

		if(read(logfd, &evlrecsize, sizeof(evlrecsize_t)) != sizeof(evlrecsize_t)) {
			return EIO;	
		}	

		if (evlrecsize != (REC_HDR_SIZE + rechdr.log_size) )	{
			TRACE("Something wrong! Record size does not compute.\n");
			return EIO;
		}
		
		if (_evlEvaluateQuery(query, &rechdr, (char *) varbuf) &&
				((strlen(logmgmt_msg) == 0) || (strcmp((char *)logmgmt_msg, (char *) varbuf) != 0))) {
			/* count number of records that would be deleted */
			delcnt++;		
			nbytes +=(REC_HDR_SIZE + rechdr.log_size + sizeof(evlrecsize_t));
		}	
		/*
		 * Adjust offset so we can move file pointer forward
		 * to the next record 
		 */
		offset += (evlrecsize + sizeof(evlrecsize_t));
		bytes_to_process -= (evlrecsize + sizeof(evlrecsize_t));
		reccnt++;
	}
	fprintf(stdout, "Total number of records is %d.\n", reccnt);
	*num_delrec = delcnt;
	*delbytes = nbytes; 
	return ret;
}

/* 
 *  Assume that the log file is openned and locked -
 */
int compactLog(const char *logfilepath, int flags)
{
	int ret = 0;
	char varbuf[POSIX_LOG_ENTRY_MAXLEN + sizeof(evlrecsize_t)];
	char offset_info_file[256];
	struct file_offset_info fi;
	size_t reclen;
	off_t nextRecOffset;
	struct posix_log_entry rechdr;
	struct stat st;
	
	st.st_size = 0;
	
	if (fstat(logfd, &st) || !st.st_size) {
		return EIO;
	}
	
	fi.beginhole = 0;
	/* Do not process the log header, start at 1st rec */
	fi.offset = sizeof(log_header_t);		
	fi.bytes_to_process = st.st_size - fi.offset;
		

	g_num_delrec = 0;	
	while (fi.bytes_to_process > 0) {
#ifdef __CRASH_TEST__
	static int crash_now = 0;
	if (crash_now == 10) {
		fprintf(stderr, "System crashed!\n");
		exit(1);
	}
	crash_now++;
#endif	
		lseek(logfd, fi.offset, SEEK_SET);
		if (read(logfd, &rechdr, REC_HDR_SIZE) != REC_HDR_SIZE) {
			fprintf(stderr, "Failed to read record header.\n");
			ret = EIO;
			goto err_exit;
		}
		if ((reclen = getRecLen(&rechdr)) == -1) {
			fprintf(stderr, "getRecLen failed! Log file is corrupted.\n");
			ret = EIO;
			goto err_exit;
		}
		if(read(logfd, &varbuf, rechdr.log_size + sizeof(evlrecsize_t)) != 
			(rechdr.log_size+sizeof(evlrecsize_t))) {
			fprintf(stderr, "Failed to read variable data portion.\n");
			ret = EIO;
			goto err_exit;	
		}
		if (doesRecMatchForDel(&rechdr, varbuf)) {
			if (fi.beginhole == 0) {
				fi.beginhole = fi.offset;
			}
		}
		else if (fi.beginhole != 0) {
			/* Updates fi.beginhole */
			if (moveRecordUp(&fi, &rechdr, varbuf) != 0) {
				fprintf(stderr, "Log file is corrupted. Compaction aborted.\n");
				ret = EIO;
				goto err_exit;
			}
		}
		fi.offset += reclen;
		fi.bytes_to_process -= reclen;
	}
	TRACE("beginhole = %d\n", fi.beginhole);
	/* 
	 * Now the hole should be at the bottom of the file
	 * truncate the log file right at the beginhole
	 */
	if (fi.beginhole > 0 && ftruncate(logfd, fi.beginhole) == -1) {
		fprintf(stderr, "Failed to truncate log file\n");
		return EBADF;
	}
err_exit:
	return ret;
}

/*
 * Scans the log file looking for a valid record.
 * If found returns pointer to that record otherwise -1.
 */
off_t
searchForNextGoodRecord(off_t offset)
{
	static struct posix_log_entry *entry = 0; 
	unsigned int log_magic;
	size_t log_size;
	evlrecsize_t recsize;
	static int log_size_offset = (char *) &entry->log_size - (char *) entry;
	
	lseek(logfd, offset, SEEK_SET);
	while(read(logfd, &log_magic, sizeof(log_magic)) /*!= -1*/ == sizeof(log_magic)) {
		if ((log_magic & 0xffff0000) == LOGREC_MAGIC_HIWORD) {
			lseek(logfd, offset + log_size_offset, SEEK_SET);
			if (read(logfd, &log_size, sizeof(log_size)) != sizeof(log_size)) {
				fprintf(stderr, "Fail to read the record header.\n");
				return -1;
			}
			lseek(logfd, offset + log_size + REC_HDR_SIZE, SEEK_SET);
			if (read(logfd, &recsize, sizeof(evlrecsize_t)) != sizeof(evlrecsize_t)) {
				fprintf(stderr,"read recsize at end of record failed\n");
				return -1;
			}
			if ((log_size + REC_HDR_SIZE) == recsize) {
				/* Found a good record */
				return offset;
			}
		}
		/*
		 * We should increment offset one byte at a time, it is very
		 * expensive, but it is OK.
		 */
		offset ++;
		lseek(logfd, offset, SEEK_SET);
	}
	return -1;			
}

/*
 * Tries to repair a corrupted log file. 
 */
int 
repairLog(int * status)
{
	int ret = 0;
	struct file_offset_info fi;
	struct posix_log_entry rechdr;
	off_t nextRecOffset;
	size_t reclen;
	struct stat st;
	st.st_size = 0;

	*status = 0;
	if (fstat(logfd, &st) || !st.st_size ) {
		return EIO;
	}
	sizeb4repair = st.st_size;
repair_again:

	fi.beginhole = 0;
	/* Do not process the log header, start at 1st rec */
	fi.offset = sizeof(log_header_t);		
	fi.bytes_to_process = st.st_size - fi.offset;
	
	while (fi.bytes_to_process > 0) {
		if ((nextRecOffset = validateRec(fi.offset)) == -1) {
			*status = 1;
			/* found the corrupted record */
			fprintf(stdout, "Found a corrupted event record."
					  " Trying to resync with next good record...\n");
			fi.beginhole = fi.offset;
			if ((nextRecOffset = searchForNextGoodRecord(fi.offset)) == -1) {
				/* Can't find another good record */
				break;
			}
			fprintf(stdout, "Found a good record after the bad block!\n");
			fi.bytes_to_process -= (nextRecOffset - fi.offset);
			fi.offset = nextRecOffset;
			TRACE("beginhole = %d\n", fi.beginhole);
			TRACE("bytes_to_process = %d\n", fi.bytes_to_process );
			TRACE("nextOffset = %d\n", fi.offset);
			/* 
			 * We found a good record after the bad block.
			 * Now we move records to the hole.
			 */
			while (fi.bytes_to_process > 0) {	
				lseek(logfd, fi.offset, SEEK_SET);
				if (read(logfd, &rechdr, REC_HDR_SIZE) != REC_HDR_SIZE) {
					fprintf(stderr, "Failed to read record header.\n");
					return EIO;
				}	
				if ((reclen  = getRecLen(&rechdr)) == -1) {
					fprintf(stderr, "getRecLen failed.\n");
					goto repair_again;
				}

				/* moveRecordUp also update beginhole */
				if (moveRecordUp(&fi, &rechdr, NULL) == -1 ) {
					goto repair_again;
				}
	
				fi.offset += reclen;
				fi.bytes_to_process -= reclen;
			}
		} 
		else {
			fi.bytes_to_process -= (nextRecOffset - fi.offset);
			fi.offset = nextRecOffset;
		}		
	}
	if (fi.beginhole > 0 && ftruncate(logfd, fi.beginhole) == -1) {
		fprintf(stderr, "Failed to truncate log file.\n");
		return EBADF;
	}

	return ret;
}
	
/* 
 * Change generation number. Type = RESET| INCR | EVEN | ODD
 * Assume that logfile is locked.
 */
int
setGenerationNumber(set_gen_t type)
{
	log_header_t log_hdr;
	struct posix_log_entry rec_hdr;

	/* Make sure that we are the beginning of the file */
	if (lseek(logfd, 0, SEEK_SET) == (off_t)-1) {
		return EBADF;
	}
	if (read(logfd, &log_hdr,  sizeof(log_header_t)) != sizeof(log_header_t)) {
		return EBADF;
	}	
	if (log_hdr.log_magic != LOGFILE_MAGIC) {
		return EBADF;
	}

	switch(type) {
		case GEN_INCR:
			/* Increase the generation number */
			log_hdr.log_generation++;
			break;
		case GEN_RESET:
			log_hdr.log_generation = 0;
			break;
		case GEN_EVEN:
			if (log_hdr.log_generation % 2) { 
				log_hdr.log_generation++;
			}
			break;
		case GEN_ODD:
			if (!(log_hdr.log_generation % 2)) {
				log_hdr.log_generation++;
			}
			break;
		default:
			break;
	}
	TRACE("log_generation = %d\n", log_hdr.log_generation);
	
	if(lseek(logfd, 0, SEEK_SET) == (off_t)-1 ) {
		return EBADF;
	}
	/* write log_generation back to logfile */
	if (write(logfd, &log_hdr, sizeof(log_header_t)) != sizeof(log_header_t)) {
		return EBADF;
	}	
	
	return 0;
}

	
/*
 * Creates a backup log file -
 * Returns 0 if success otherwise -1;
 */
#define BUF_SIZE			8 * 1024
#define MAX_FILEPATH_SIZE	256
int
createBackUp(const char *path)
{
	int bkfd;
	char backup_filepath[MAX_FILEPATH_SIZE];
	char bytes[BUF_SIZE];
	int n, nbytesRead;
	int ret = 0;

	gzFile gzbk;

	sigset_t oldset;
	int sigsBlocked;
	/* Mask all signals so we don't get interrupted */
	sigsBlocked = (_evlBlockSignals(&oldset) == 0);
	g_backupCreated = 0;
	
	if (gzBackUpFlag) {
		snprintf(backup_filepath, sizeof(backup_filepath), "%s.bak.gz", path);
	}
	else {
		snprintf(backup_filepath, sizeof(backup_filepath), "%s.bak", path);
	}

	if ((bkfd = open(backup_filepath, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {	
		(void)fprintf(stderr, "Failed to create back up copy of the log.\n");
		ret = -1;
		goto err_exit;
	}
	if (gzBackUpFlag) {
		if ((gzbk = gzdopen(bkfd, "wb")) == NULL) {
			fprintf(stderr, "Failed to create compressed file handle.\n");
			ret = -1;
			goto err_exit;
		}
	}
		
	/* Copy the contents of the log file to the back up file */
	while((nbytesRead = read(logfd, bytes, BUF_SIZE)) > 0) {	
		
		if (gzBackUpFlag) {
			if ((n = gzwrite(gzbk, bytes, nbytesRead)) != nbytesRead) {
				TRACE("Failed to write to back up log file\n");
				gzclose(gzbk);
				close(bkfd);
		 		ret = -1;
		 		goto err_exit;
			}	
		}
		else {
			if((n = write(bkfd, bytes, nbytesRead)) != nbytesRead) {
				TRACE("Failed to write to back up log file\n");
				close(bkfd);
		 		ret = -1;
		 		goto err_exit;
			}	
		}	
	}
	if (gzBackUpFlag) gzclose(gzbk);
	close(bkfd);
	g_backupCreated = 1;
	
err_exit:
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	
	TRACE("Backup log created.\n");
	return ret;
}

/* 
 * Remove back up log
 */
void 
removeBackup(const char *path)
{
	char backup_filepath[MAX_FILEPATH_SIZE];
	if (gzBackUpFlag) {
		snprintf(backup_filepath, sizeof(backup_filepath), "%s.bak.gz", path);
	}
	else {
		snprintf(backup_filepath, sizeof(backup_filepath), "%s.bak", path);
	}
	unlink(backup_filepath);
}	
/*
 * Restoring the original log file by copying back the backup file
 * to the original.
 */
int
restoreLogfileFromBackup(const char *path)
{
	char backup_filepath[MAX_FILEPATH_SIZE];
	char bytes[BUF_SIZE];
	int n, nbytesRead;
	int bkfd, ret = 0;
	gzFile gzbk;
	sigset_t oldset;
	int sigsBlocked;
	
	if (gzBackUpFlag) {
		snprintf(backup_filepath, sizeof(backup_filepath), "%s.bak.gz", path);
	}
	else {
		snprintf(backup_filepath, sizeof(backup_filepath), "%s.bak", path);
	}
	/* Mask all signals so we don't get interrupted */
	sigsBlocked = (_evlBlockSignals(&oldset) == 0);
	fprintf(stderr, "Restoring original logfile...\n");
	
	if (g_backupCreated) { 
		if ((bkfd = open(backup_filepath, O_RDONLY, S_IRUSR | S_IWUSR)) < 0) {
			(void)fprintf(stderr, "Can't back up log file to restore.\n");
			ret = -1;
			goto err_exit;
		} 
		if ((gzbk = gzdopen(bkfd, "wb")) == NULL) {
			fprintf(stderr, "Failed to create compressed file handle.\n");
			ret = -1;
			goto err_exit;
		}
	}
	else {
		(void)fprintf(stderr, "There is no back up log file or it was not completely created. Restore aborts.\n");
		ret = -1;
		goto err_exit;
	}	

	if (lseek(logfd, 0, SEEK_SET) == (off_t)-1) {
		ret = -1;
		fprintf(stderr, "Failed to seek to the beginning of the log.\n");
		goto err_exit;
	}
	/* Copy the contents of the back up file to the original log file */
	/* Use gzread for both compressed file or uncompressed file. It works */
	while ((nbytesRead = gzread(gzbk, bytes, BUF_SIZE)) > 0) {	
		if((n = write(logfd, bytes, nbytesRead)) != nbytesRead) {
			fprintf(stderr, "Failed to restore original log file.\n");
			gzclose(gzbk);
			close(bkfd);
			ret = -1;
			goto err_exit;
		}
	}
	gzclose(gzbk);
	close(bkfd);
	unlink(backup_filepath);
	fprintf(stderr, "Original log is restored.\n");
err_exit:
//	evl_file_unlock();
//	close(logfd);
	if (sigsBlocked) {
		_evlRestoreSignals(&oldset);
	}
	return ret;
}
				
int 
backupFileExists(const char *path)
{
	int fd;
	char filepath[256];
	snprintf(filepath, sizeof(filepath), "%s.bak~", path);
	if ((fd =open(filepath, O_RDWR, S_IRWXU))== -1) {
		if (errno != ENOENT) {
			perror("open");
		}
		return 0;
	}
	else {
		close(fd);
		return 1;
	}
}


/*
 * Repair logfile. If succeeded return 0 also set the log generation number
 * to the next even value.
 */
int 
evl_repair_log(const char *path)
{
	int ret, status;
	struct stat st;
	st.st_size = 0;
	
	
	
	if (posix_log_printf(LOG_LOGMGMT, POSIX_LOG_MGMT_STARTMAINT,
						LOG_NOTICE, 0, "%s", "Log repair started.") != 0) {
			if (errno != ENOENT && errno != ECONNREFUSED)
				perror("posix_log_printf of STARTMAINT message");
			return errno;
	}
	
	if ((logfd = open(path, O_RDWR)) == -1) {
		perror("open");
		posix_log_printf(LOG_LOGMGMT, POSIX_LOG_MGMT_ENDMAINT,
						LOG_NOTICE, 0, "%s", "Log repair failed, could not open log file.");
		return EBADF;
	}
	
	(void) evl_file_lock();
	
	if ((ret = repairLog(&status)) == 0) {
		if (setGenerationNumber(GEN_EVEN) != 0) {
			fprintf(stderr, "evl_repair_log: Failed to increase log file generation number.\n");
			posix_log_printf(LOG_LOGMGMT, POSIX_LOG_MGMT_ENDMAINT,
						LOG_NOTICE, 0, "%s", 
						"Log repair finished but failed to change "
						"the log generation number to an even value.");
	
			return EBADF;
		}
		fstat(logfd, &st);
		(void) evl_file_unlock();
		close(logfd);
		if (status == 0) {
			fprintf(stdout, "Log repair finished. No problem found.\n");
			posix_log_printf(LOG_LOGMGMT, POSIX_LOG_MGMT_ENDMAINT,
						LOG_NOTICE, 0, "%s", "Log repair finished. No problem found.");
		} else if (status == 1) {	
			fprintf(stdout, "Log repair finished, problem fixed. "
							" Discarded %u bytes.\n", sizeb4repair - st.st_size);
			snprintf(logmgmt_msg, sizeof(logmgmt_msg), "Log repair finished, problem fixed. "
								 "Discarded %u bytes.", sizeb4repair - st.st_size);
			posix_log_printf(LOG_LOGMGMT, POSIX_LOG_MGMT_ENDMAINT,
						LOG_NOTICE, 0, "%s", logmgmt_msg);
		}
	} 
	else {
		(void) evl_file_unlock();
		close(logfd);
		fprintf(stdout, "Log repair failed, errno=%u!\n", ret);	
		posix_log_printf(LOG_LOGMGMT, POSIX_LOG_MGMT_ENDMAINT,
						LOG_NOTICE, 0, "%s", "Log repair failed");
	}
	
	return ret;
}	

/**************************************************************************************
 
evl_compact_log() does the lion's share of log management...

1. Create a query object from the filter expression provided by the user.
2. Log a "start" event (defined in the POSIX standard as POSIX_LOG_MGMT_STARTMAINT).
3. Lock the log file.
4. Create a backup copy of the original log file.  Create a compressed backup copy
   if the user has specified that option.
 * If there is insufficient space to create a backup, and the --force option is not
   specified, the log file is unlocked and the command returns an error without any
   changes to the log file.
 * If the --force option is specified, this function continues even if it is unable
   to create a backup copy.
5. Set the "generation number" in the header of the log file to an odd value.
6. Perform compaction by moving unmarked records to fill the space occupied by the
   records selected for deletion, and truncates the log file.  The sequential order
   of events in the event log is maintained during this process and the record ids
   of the remaining event records are left unchanged.
7. Set the "generation number" in the header of the log file to an even value.
8. Unlock the log file.
9. Delete the backup copy of the log file (if a backup was created in step 4).
10. Destroy the query object created in step 1.
11. Log an "end" event (defined in the POSIX standard as POSIX_LOG_MGMT_ENDMAINT).

***************************************************************************************/

int
evl_compact_log(const char *path, const char *filter, int flags)
{
	int ret, num_delrec;
	int backup = 0;
	time_t t;
	
	
	if(posix_log_query_create(filter, POSIX_LOG_PRPS_GENERAL,
             &query, error_str, 256) != 0) {
		fprintf(stderr, "ERROR: could not create query object! Error message: \n   %s.\n", error_str);
		return EINVAL;
	}

	/* 
	 * Log this message with the time stamp to them unique so that we can
	 * identify it later then don't delete it.
	 */
	time(&t);
	snprintf(logmgmt_msg, sizeof(logmgmt_msg), "Log compaction on %s starts at %s", path, ctime(&t));

	if (posix_log_printf(LOG_LOGMGMT, POSIX_LOG_MGMT_STARTMAINT,
						LOG_NOTICE, 0, "%s", logmgmt_msg) != 0) {
			if (errno != ENOENT && errno != ECONNREFUSED)
				perror("posix_log_printf of STARTMAINT message");
			return errno;
	}
	if ((logfd = open(path, O_RDWR)) == -1) {
		perror("open of event log to be compacted.");
		return EBADF;
	}
	(void) evl_file_lock();
	
	/* Make backup copy of the log */
	if (createBackUp(path) == -1 ) {
		/*
		 * If force option is selected then we go ahead
		 * with compaction event we failed to create 
		 * the back up copy of the log due to lack of disk
		 * space etc..
		 */
		if (flags & EVL_FORCE_COMPACT) {
			/* continue compaction */
		} 
		else {
			fprintf(stderr, "evl_compact_log: Failed to create backup logfile\n");
			ret = EBADF;
			goto err_exit;
		}
	}
	else {
		/* Back up file created */
		backup=1;
	}

	if (setGenerationNumber(GEN_ODD) != 0 ) {
		fprintf(stderr, "evl_compact_log: Failed to change log file generation number.\n");
		ret = EBADF;
		goto err_exit;
	}

	if ((ret = compactLog(path, flags)) != 0) {
		fprintf(stderr, "evl_compact_log: Failed to compact log.\n");
		goto err_exit;
	}	
	
err_exit:	
	if (setGenerationNumber(GEN_EVEN) != 0) {
		fprintf(stderr, "evl_compact_log: Failed to increase log file generation number.\n");
		ret = EBADF;
	}	
	(void) evl_file_unlock();
	close(logfd);
	if (backup) {
		removeBackup(path);
	}
	posix_log_query_destroy(&query);
	if (ret == 0) {
		snprintf(logmgmt_msg, sizeof(logmgmt_msg),
			"Log compaction on %s ended. %d events were removed.", path, g_num_delrec);		
	} else {
		snprintf(logmgmt_msg, sizeof(logmgmt_msg),
			"Log compaction on %s ended. Operation failed with errno = %d.", path, ret);
	}
	posix_log_printf(LOG_LOGMGMT, POSIX_LOG_MGMT_ENDMAINT,
						LOG_NOTICE, 0, "%s", logmgmt_msg);
	return ret;
}

/*
 * Returns 0 if succeeded and the total number of records that match the query for deletion.
 * Note: This function does not mark records for deletion, just counts them.
 */
int 
evlGetNumDelRec(const char *path, const char *filter, int *num_delrec, size_t *delBytes) 
{
	int ret = 0, delrecs = -1;
	size_t nbytes = 0;

	if(posix_log_query_create(filter, POSIX_LOG_PRPS_GENERAL,
             &query, error_str, 256) != 0) {
		fprintf(stderr, "ERROR: could not create query object! Error message: \n   %s.\n", error_str);

		return EINVAL;
	}
	
	if ((logfd = open(path, O_RDONLY)) == -1) {
		perror("open");
		return EBADF;
	}

	if ((ret = countRecsForDeletion(&query, &delrecs, &nbytes)) != 0 ) {
		fprintf(stderr, "evlGetNumDelRec: Failed to count record for deletion.\n");
		goto err_exit;
	}
	
err_exit:
	close(logfd);
	posix_log_query_destroy(&query);
	*num_delrec = delrecs;
	*delBytes = nbytes;
	return ret;
}

/*
 * File locking routines
 */
int 
evl_file_lock() 
{
	return file_lock(logfd, F_SETLKW, F_RDLCK | F_WRLCK);	
}

int 
evl_file_unlock()
{
	return file_lock(logfd, F_SETLKW, F_UNLCK);	
}
/* 
 * cmd is F_GETLK see if lock exists on a file descriptor fd,
 * or F_SETLK set a lock on file descriptor fd,
 * or F_SETLKW, the blocking version of F_SETLK.
 * Process sleeps until lock can be obtained. 
 * 
 */
int 
file_lock(int fd, int cmd, int type)
{
  struct flock lock;
  lock.l_type=type;					/*F_RDLCK, F_WRLCK, or F_UNLOCK */
  lock.l_len= lock.l_start= 0;		/* byte offset relative to lock.l_whence */
  lock.l_whence=SEEK_SET;			/* or SEEK_CUR or SEEK_END*/
  return( fcntl(fd, cmd, &lock) );
}
