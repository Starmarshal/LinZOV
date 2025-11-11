#include "lib.h"


int printrr(char *msg, ...){
	va_list args;
	va_start(args, msg);
	vfprintf(stdout, msg, args);
	va_end(args);

	fflush(stdout);
	return 1;
}

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

/* I print err with many args without ending the prog */
void printErrNE(char *msg, ...){
	va_list args;
	va_start(args, msg);

	vfprintf(stdout, msg, args);
	va_end(args);

	fflush(stdout);
	fflush(stderr);
}

long getFileSize(FILE *fd){
	/* Check archive size */
	fseek(fd, 0, SEEK_END);
	long archive_size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	return archive_size;
}
