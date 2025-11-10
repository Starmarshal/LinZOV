#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

/* File header structure */
typedef struct {
    char filename[256];       /* original file name */
    uint64_t file_size;       /* file size in bytes */
    uint32_t permissions;     /* file permissions */
    uint64_t offset;          /* offset in archive */
    uint8_t is_compressed;    /* compression flag */
    uint8_t algorithm;        /* compression algorithm */
} FileHeader;

/* Archive header structure */
typedef struct {
    char magic[8];            /* magic number "ARCHv1.0" */
    uint16_t file_count;      /* number of files */
    uint64_t total_size;      /* total archive size */
    uint8_t has_password;     /* password protection flag */
} ArchiveHeader;

/* Function declarations */
int create_archive(const char* dir_path, const char* archive_path, 
                   const char* password);
int extract_archive(const char* archive_path, const char* output_dir,
                   const char* password);
void list_archive_contents(const char* archive_path);
int verify_archive(const char* archive_path);
void add_timestamp_to_file(const char* filepath);
int is_archive_file(const char* filename);

/* Utility functions */
void process_directory(const char* base_path, const char* rel_path, 
                      FILE* archive, uint16_t* file_count,
                      uint64_t* total_size);
void process_single_file(const char* filepath, const char* rel_path, 
                        FILE* archive, uint16_t* file_count,
                        uint64_t* total_size, struct stat* stat_buf);
int create_directory(const char* path);
int create_parent_dirs(const char* filepath);
int should_compress_file(const char* filename);

#endif
