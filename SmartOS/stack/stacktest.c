
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include "stack.h"

main(int argc, char *argv[])
{
    int fd;
    int cmd;

    if (argc != 2) {
	fprintf(stderr, "Usage: %s red|corrupt\n", argv[0]);
	exit(1);
    }
    if (strcmp(argv[1], "red") == 0)
	cmd = REDZONE_OVERFLOW;
    else if (strcmp(argv[1], "corrupt") == 0)
	cmd = CORRUPT_OVERFLOW;
    else {
	fprintf(stderr, "Usage: %s red|corrupt\n", argv[0]);
	exit(1);
    }

    if ((fd = open("/devices/pseudo/stack@0:stack", O_RDWR)) < 0) {
	perror("open failed");
	exit(1);
    }

    if (ioctl(fd, cmd, 0) == -1) {
	perror("ioctl failed");
	exit(1);
    }
}


