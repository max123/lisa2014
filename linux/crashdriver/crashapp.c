#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "crash.h"

int
main(int argc, char *argv[])
{
	int fd, rval;

	fd = open("/dev/crash0", 2);
	if (fd < 0) {
		perror("open failed\n");
		exit(1);
	}

	if (argc == 1) {  /* default is null pointer crash */
		rval = ioctl(fd, CRASH_NULLPOINTER, 0);
		printf("ioctl CRASH_NULLPOINTER returned!!! rval = %d\n", rval);
	}

	if (argc > 1) {
		if (strcmp("deadbeef", argv[1]) == 0) {
			ioctl(fd, CRASH_ALLOC, 0);
			sleep(300);
			ioctl(fd, CRASH_USEAFTERFREE, 0);
			pause();
		} else if (strcmp("nullpointer", argv[1]) == 0) {
			rval = ioctl(fd, CRASH_NULLPOINTER, 0);
			printf("ioctl CRASH_NULLPOINTER returned!!! rval = %d\n", rval);
		} else if (strcmp("stackoverflow", argv[1]) == 0) {
			rval = ioctl(fd, CRASH_STACKOVERFLOW, 0);
			printf("ioctl CRASH_STACKOVERFLOW returned!!! rval = %d\n", rval);
		}
	}
}
