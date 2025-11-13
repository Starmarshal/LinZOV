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
