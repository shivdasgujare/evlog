#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "posix_evlog.h"
#include "posix_evlsup.h"
#include "evlog.h"

typedef struct tester {
	int one;
	int two;
} struct_t;

static void
check_status(int status, int event_type, const char *func)
{
	if (status != 0) {
		fprintf(stderr, "Error logging event type %d (0x%x):\n",
			event_type, event_type);
		errno = status;
		perror(func);
		exit(1);
	}
}

int main(int argc, char **argv)
{
	struct_t array[2];
	int status;
	array[0].one = 1;
	array[0].two = 2;
	array[1].one = 10;
	array[1].two = 20;
	
        status = posix_log_write(LOG_USER, 6000, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6000, "posix_log_write");
	
        status = posix_log_write(LOG_USER, 6001, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6001, "posix_log_write");

        status = posix_log_write(LOG_USER, 6002, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6002, "posix_log_write");

        status = posix_log_write(LOG_USER, 6003, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6003, "posix_log_write");

        status = posix_log_write(LOG_USER, 6012, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6012, "posix_log_write");

        status = posix_log_write(LOG_USER, 6013, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6013, "posix_log_write");

        status = posix_log_write(LOG_USER, 6015, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6015, "posix_log_write");

        status = posix_log_write(LOG_USER, 6016, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6016, "posix_log_write");

        status = posix_log_write(LOG_USER, 6017, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6017, "posix_log_write");

        status = evl_log_write(LOG_USER, 6021, LOG_INFO, 0, 
		"char[]", sizeof(array), &array,
		"endofdata");
	check_status(status, 6021, "evl_log_write");

        status = evl_log_write(LOG_USER, 6022, LOG_INFO, 0, 
		"char[]", sizeof(array), &array,
		"endofdata");
	check_status(status, 6022, "evl_log_write");

        status = evl_log_write(LOG_USER, 6023, LOG_INFO, 0, 
		"char[]", sizeof(array), &array,
		"endofdata");
	check_status(status, 6023, "evl_log_write");

        status = evl_log_write(LOG_USER, 6024, LOG_INFO, 0, 
		"char[]", sizeof(array), &array,
		"endofdata");
	check_status(status, 6024, "evl_log_write");

        status = evl_log_write(LOG_USER, 6025, LOG_INFO, 0, 
		"char[]", sizeof(array), &array,
		"endofdata");
	check_status(status, 6025, "evl_log_write");

        status = evl_log_write(LOG_USER, 6026, LOG_INFO, 0, 
		"char[]", sizeof(array), &array,
		"endofdata");
	check_status(status, 6026, "evl_log_write");

        status = posix_log_write(LOG_USER, 6031, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6031, "posix_log_write");

        status = posix_log_write(LOG_USER, 6032, LOG_INFO, NULL, 0, POSIX_LOG_NODATA, 0);
	check_status(status, 6032, "posix_log_write");

	status = evl_log_write(LOG_USER, 6033, LOG_INFO, 0,
		"string", "\t",
		"3*int", 10, 20, 30,
		"endofdata");
	check_status(status, 6033, "evl_log_write");

	status = evl_log_write(LOG_USER, 6034, LOG_INFO, 0,
		"string", ".",
		"3*int", 10, 20, 30,
		"endofdata");
	check_status(status, 6034, "evl_log_write");

        exit(0);
}
