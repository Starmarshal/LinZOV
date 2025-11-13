#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>
#include <time.h>

#include <sys/stat.h>

#include "lib.h"

/* defines */
#define MAGIC "HxKl1488" 
#define ALGO_PPM 1

#define SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

/* File header structure */
typedef struct {
	char filename[BUFFER*2];       /* original file name */
	uint64_t file_size;       /* file size in bytes */
	uint32_t permissions;     /* file permissions */
	uint64_t offset;          /* offset in archive */
	uint8_t is_compressed;    /* compression flag */
	uint8_t algorithm;        /* compression algorithm */
} FileHeader;

/* PPM context structure */
typedef struct PPMNode {
	uint8_t symbol;
	uint32_t count;
	struct PPMNode* next;
} PPMNode;

/* PPM model structure */
typedef struct {
	PPMNode** contexts;
	int order;
	size_t memory_limit;
} PPMModel;

/* Archive header structure */
typedef struct {
	char magic[8];            /* magic number*/
	uint16_t file_count;      /* number of files */
	uint64_t total_size;      /* total archive size */
	uint8_t has_password;     /* password protection flag */
} ArchiveHeader;

/* Function declarations */
long getFileSize(FILE *archive);
int create_archive(const char* dir_path, const char* archive_path, const char* password, int vflag);
int extract_archive(const char* archive_path, const char* output_dir, const char* password, int vflag);
void list_archive_contents(const char* archive_path);
int verify_archive(const char* archive_path);

#endif
