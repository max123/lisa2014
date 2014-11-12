
#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stropts.h>
#include <errno.h>
#include <stdlib.h>

#include "corrupt.h"

int
main(int argc, char *argv[])
{
	int fd, rval;

	fd = open("/devices/pseudo/corrupt@0:corruptmajor", O_RDWR);  /* too bad it's hard coded... */

	if (fd < 0) {
		perror("open failed\n");
		exit(1);
	}

	if (argc > 1) {
		if (strcmp("deadbeef", argv[1]) == 0) {
			ioctl(fd, CRASH_ALLOC, 0);
			sleep(10);
			ioctl(fd, CRASH_USEAFTERFREE, 0);
			printf("finished\n");
			exit(0);
		} else
			printf("not yet\n");
	}
}






