#include "utils.h"
#include "archive.h"
#include "ppm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>  /* для strcasecmp */
#include <utime.h>    /* для utime */

/* Process directory recursively */
void process_directory(const char* base_path, const char* rel_path, 
                      FILE* archive, uint16_t* file_count,
                      uint64_t* total_size) {
    char full_path[1024];
    if (strlen(rel_path) == 0) {
        snprintf(full_path, sizeof(full_path), "%s", base_path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, rel_path);
    }
    
    DIR* dir = opendir(full_path);
    if (!dir) {
        fprintf(stderr, "Warning: Cannot open directory %s: %s\n", full_path, strerror(errno));
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. entries */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Build relative path */
        char new_rel_path[1024];
        if (strlen(rel_path) == 0) {
            snprintf(new_rel_path, sizeof(new_rel_path), "%s", entry->d_name);
        } else {
            snprintf(new_rel_path, sizeof(new_rel_path), "%s/%s", rel_path, entry->d_name);
        }
        
        /* Build full path */
        char entry_full_path[1024];
        snprintf(entry_full_path, sizeof(entry_full_path), "%s/%s", base_path, new_rel_path);
        
        struct stat stat_buf;
        if (stat(entry_full_path, &stat_buf) != 0) {
            fprintf(stderr, "Warning: Cannot stat %s: %s\n", entry_full_path, strerror(errno));
            continue;
        }
        
        if (S_ISDIR(stat_buf.st_mode)) {
            /* Recursively process subdirectory */
            process_directory(base_path, new_rel_path, archive, file_count, total_size);
        } else if (S_ISREG(stat_buf.st_mode)) {
            /* Process regular file */
            process_single_file(entry_full_path, new_rel_path, archive, 
                              file_count, total_size, &stat_buf);
        }
    }
    
    closedir(dir);
}

/* Process single file for archiving */
void process_single_file(const char* filepath, const char* rel_path, 
                        FILE* archive, uint16_t* file_count,
                        uint64_t* total_size, struct stat* stat_buf) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Warning: Cannot open file %s: %s\n", filepath, strerror(errno));
        return;
    }
    
    /* Get file size safely */
    fseek(file, 0, SEEK_END);
    long file_size_long = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size_long <= 0) {
        fclose(file);
        printf("Skipped: %s (empty file)\n", rel_path);
        return;
    }
    
    size_t file_size = (size_t)file_size_long;
    
    /* Read file data */
    uint8_t* file_data = malloc(file_size);
    if (!file_data) {
        fclose(file);
        fprintf(stderr, "Error: Memory allocation failed for %s\n", filepath);
        return;
    }
    
    size_t bytes_read = fread(file_data, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != file_size) {
        free(file_data);
        fprintf(stderr, "Error: Cannot read file %s\n", filepath);
        return;
    }
    
    /* Prepare file header */
    FileHeader header;
    memset(&header, 0, sizeof(FileHeader));
    strncpy(header.filename, rel_path, sizeof(header.filename) - 1);
    header.permissions = stat_buf->st_mode;
    header.offset = *total_size;
    header.algorithm = 1; /* PPM */
    
    /* Try compression only for files larger than 100 bytes */
    uint8_t* compressed_data = NULL;
    size_t compressed_size = 0;
    int should_compress = should_compress_file(filepath) && file_size > 100;
    
    if (should_compress) {
        compressed_size = ppm_compress(file_data, file_size, &compressed_data);
    }
    
    /* Decide whether to use compressed or original data */
    if (compressed_data && compressed_size > 0 && compressed_size < file_size) {
        header.file_size = compressed_size;
        header.is_compressed = 1;
        printf("Processed: %s (PPM) %zu -> %zu bytes\n", 
               rel_path, file_size, compressed_size);
    } else {
        header.file_size = file_size;
        header.is_compressed = 0;
        if (compressed_data) {
            free(compressed_data);
        }
        compressed_data = file_data;
        compressed_size = file_size;
        file_data = NULL;
        printf("Processed: %s (store) %zu bytes\n", rel_path, file_size);
    }
    
    /* Write to archive */
    size_t header_written = fwrite(&header, sizeof(FileHeader), 1, archive);
    size_t data_written = fwrite(compressed_data, 1, header.file_size, archive);
    
    if (header_written != 1 || data_written != header.file_size) {
        fprintf(stderr, "Error: Write failed for %s\n", rel_path);
    } else {
        /* Update counters */
        (*file_count)++;
        *total_size += sizeof(FileHeader) + header.file_size;
    }
    
    /* Cleanup */
    if (compressed_data != file_data) {
        free(compressed_data);
    }
    if (file_data) {
        free(file_data);
    }
}

/* Create directory if it doesn't exist */
int create_directory(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            fprintf(stderr, "Error: Cannot create directory %s: %s\n", path, strerror(errno));
            return -1;
        }
    }
    return 0;
}

/* Create parent directories for file path */
int create_parent_dirs(const char* filepath) {
    char path[1024];
    strncpy(path, filepath, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    
    char* slash = strrchr(path, '/');
    if (slash) {
        *slash = '\0';
        return create_directory(path);
    }
    return 0;
}

/* Add timestamp to file */
void add_timestamp_to_file(const char* filepath) {
    /* Update the file's modification time to current time */
    struct utimbuf new_times;
    new_times.actime = time(NULL);   /* access time */
    new_times.modtime = time(NULL);  /* modification time */
    utime(filepath, &new_times);
}

/* Check if file should be compressed */
int should_compress_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return 1;
    
    /* Don't compress already compressed formats */
    const char* compressed_exts[] = {
        ".zip", ".gz", ".bz2", ".xz", ".7z", ".rar", ".tar",
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff", 
        ".mp3", ".mp4", ".avi", ".mkv", ".flac", ".wav",
        ".pdf", ".doc", ".docx", ".xls", ".ppt",
        NULL
    };
    
    for (int i = 0; compressed_exts[i]; i++) {
        if (strcasecmp(ext, compressed_exts[i]) == 0) {
            return 0;
        }
    }
    
    return 1;
}
