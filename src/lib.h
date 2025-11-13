#ifndef LIB_H
#define LIB_H

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#define BUFFER 4096

int printErr(char *msg, ...);
long getFileSize(FILE *archive);

#endif
