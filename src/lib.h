#ifndef LIB_H
#define LIB_H

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

/* defines */

#define SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

/* File header structure */
typedef struct {
    char filename[256];       /* original file name */
    uint64_t file_size;       /* file size in bytes */
    uint32_t permissions;     /* file permissions */
    uint64_t offset;          /* offset in archive */
    uint8_t is_compressed;    /* compression flag */
    uint8_t algorithm;        /* compression algorithm */
} FileHeader;

int printErr(char *msg, ...);
int printrr(char *msg, ...);
void printErrNE(char *msg, ...);
long getFileSize(FILE *archive);

#endif
