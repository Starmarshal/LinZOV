#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdint.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>

#include "lib.h"
#include "ppm.h"
#include "utils.h"

#define MAGIC "HxKl1488"
#define ALGO_PPM 1

/* Archive header structure */
typedef struct {
    char magic[8];            /* magic number*/
    uint16_t file_count;      /* number of files */
    uint64_t total_size;      /* total archive size */
    uint8_t has_password;     /* password protection flag */
} ArchiveHeader;

/* Function declarations */
int create_archive(const char* dir_path, const char* archive_path, const char* password, int vflag);
int extract_archive(const char* archive_path, const char* output_dir, const char* password, int vflag);
void list_archive_contents(const char* archive_path);
int verify_archive(const char* archive_path);
int is_archive_file(const char* filename);

#endif
