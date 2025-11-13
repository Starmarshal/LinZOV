#include "lib.h"

/* print err with many args */
int printErr(char *msg, ...){
	va_list args;
	va_start(args, msg);

	vfprintf(stdout, msg, args);
	va_end(args);

	fflush(stdout);
	fflush(stderr);

	exit(errno);
}

long getFileSize(FILE *fd){
	/* Check archive size */
	fseek(fd, 0, SEEK_END);
	long archive_size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	return archive_size;
}
