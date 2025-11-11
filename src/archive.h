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

#define MAGIC "ARCHv1.0"
#define ALGO_PPM 1

/* Archive header structure */
typedef struct {
    char magic[8];            /* magic number "ARCHv1.0" */
    uint16_t file_count;      /* number of files */
    uint64_t total_size;      /* total archive size */
    uint8_t has_password;     /* password protection flag */
} ArchiveHeader;

/* Function declarations */
int create_archive(const char* dir_path, const char* archive_path, const char* password);
int extract_archive(const char* archive_path, const char* output_dir, const char* password);
void list_archive_contents(const char* archive_path);
int verify_archive(const char* archive_path);
int is_archive_file(const char* filename);

/* Utility functions */
void process_directory(const char* base_path, const char* rel_path, 
                      FILE* archive, uint16_t* file_count, uint64_t* total_size);
/* void process_single_file(const char* filepath, const char* rel_path, 
                        FILE* archive, uint16_t* file_count, 
			uint64_t* total_size, struct stat* stat_buf); */
int create_directory(const char* path);
int create_parent_dirs(const char* filepath);
int should_compress_file(const char* filename);

#endif
