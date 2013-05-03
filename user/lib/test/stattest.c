#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <posix_evlog.h>

main()
{
	struct stat st;
	int status;
	
	memset(&st, 0xcd, sizeof(st));
	status = stat("/etc/passwd", &st);
	if (status != 0) {
		perror("/etc/passwd");
		exit(1);
	}

	/* Write out the stat structure as an event record. */
	status = posix_log_write(LOG_USER, 2050, LOG_INFO, &st, sizeof(st),
		POSIX_LOG_BINARY, 0);
	if (status != 0) {
		errno = status;
		perror("posix_log_write");
		exit(1);
	}

	/* Here's what we should see when we view this record with evlview. */
	printf("dev, ino = %llu, %lu\n", st.st_dev, st.st_ino);
	printf("mode = %o\n", st.st_mode);
#ifdef __s390x__
	printf("nlink = %lu\n", st.st_nlink);
#else
	printf("nlink = %u\n", st.st_nlink);
#endif
	printf("uid, gid = %u, %u\n", st.st_uid, st.st_gid);
	printf("rdev = %llu\n", st.st_rdev);
	printf("size, blksize, blocks = %ld, %ld, %ld\n", st.st_size,
		st.st_blksize, st.st_blocks);
	printf("atime, mtime, ctime = %ld, %ld, %ld\n", st.st_atime,
		st.st_mtime, st.st_ctime);

	exit(0);
}
