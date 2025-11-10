#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

/* Utility functions */
void process_directory(const char* base_path, const char* rel_path, 
                      FILE* archive, uint16_t* file_count,
                      uint64_t* total_size);
void process_single_file(const char* filepath, const char* rel_path, 
                        FILE* archive, uint16_t* file_count,
                        uint64_t* total_size, struct stat* stat_buf);
int create_directory(const char* path);
int create_parent_dirs(const char* filepath);
void add_timestamp_to_file(const char* filepath);
int should_compress_file(const char* filename);

#endif
