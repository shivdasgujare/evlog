#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "posix_evlog.h"
#include "evlog.h"
#include "posix_evlsup.h"

main(int argc, char **argv)
{
	int generation = atoi(argv[1]);
	const char *log;
	int fd;
	log_header_t hdr;
	int nBytes;

	if (argc == 3) {
		log = argv[2];
	} else {
		log = LOG_CURLOG_PATH;
	}

	fd = open(log, O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	nBytes = read(fd, &hdr, sizeof(hdr));
	if (nBytes != sizeof(hdr)) {
		perror("read");
		exit(1);
	}

	hdr.log_generation = generation;

	if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
		perror("lseek");
		exit(1);
	}

	nBytes = write(fd, &hdr, sizeof(hdr));
	if (nBytes != sizeof(hdr)) {
		perror("write");
		exit(1);
	}

	(void) close(fd);
	exit(0);
}
