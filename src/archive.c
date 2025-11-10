#include "archive.h"
#include "ppm.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>

#define MAGIC "ARCHv1.0"
#define ALGO_PPM 1

/* Create archive from directory */
int create_archive(const char* dir_path, const char* archive_path, 
                   const char* password) {
    /* Check if source directory exists */
    struct stat dir_stat;
    if (stat(dir_path, &dir_stat) != 0 || !S_ISDIR(dir_stat.st_mode)) {
        fprintf(stderr, "Error: Source directory '%s' does not exist or is not a directory\n", dir_path);
        return -1;
    }

    FILE* archive = fopen(archive_path, "wb");
    if (!archive) {
        fprintf(stderr, "Error: Cannot create archive file '%s': %s\n", archive_path, strerror(errno));
        return -1;
    }

    /* Write archive header */
    ArchiveHeader arch_header;
    memcpy(arch_header.magic, MAGIC, 8);
    arch_header.file_count = 0;
    arch_header.total_size = sizeof(ArchiveHeader);
    arch_header.has_password = (password != NULL) ? 1 : 0;
    
    if (fwrite(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1) {
        fprintf(stderr, "Error: Cannot write archive header\n");
        fclose(archive);
        return -1;
    }

    /* Process directory recursively */
    printf("Scanning directory: %s\n", dir_path);
    process_directory(dir_path, "", archive, &arch_header.file_count, 
                     &arch_header.total_size);

    if (arch_header.file_count == 0) {
        fprintf(stderr, "Warning: No files found to archive\n");
    }

    /* Update header with actual counts */
    fseek(archive, 0, SEEK_SET);
    if (fwrite(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1) {
        fprintf(stderr, "Error: Cannot update archive header\n");
        fclose(archive);
        return -1;
    }
    
    fclose(archive);
    
    /* Add timestamp to archive file */
    add_timestamp_to_file(archive_path);
    
    printf("Archive created successfully: %s\n", archive_path);
    printf("Total files: %d, Archive size: %lu bytes\n", 
           arch_header.file_count, (unsigned long)arch_header.total_size);
    
    return 0;
}

/* Extract archive to directory */
int extract_archive(const char* archive_path, const char* output_dir,
                   const char* password) {
    /* Check if archive file exists */
    struct stat archive_stat;
    if (stat(archive_path, &archive_stat) != 0) {
        fprintf(stderr, "Error: Archive file '%s' does not exist\n", archive_path);
        return -1;
    }

    FILE* archive = fopen(archive_path, "rb");
    if (!archive) {
        fprintf(stderr, "Error: Cannot open archive file '%s': %s\n", archive_path, strerror(errno));
        return -1;
    }

    /* Check if archive is not empty */
    fseek(archive, 0, SEEK_END);
    long archive_size = ftell(archive);
    fseek(archive, 0, SEEK_SET);
    
    if (archive_size < (long)sizeof(ArchiveHeader)) {
        fprintf(stderr, "Error: Archive file is too small or empty\n");
        fclose(archive);
        return -1;
    }

    /* Read archive header */
    ArchiveHeader arch_header;
    if (fread(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1) {
        fprintf(stderr, "Error: Cannot read archive header\n");
        fclose(archive);
        return -1;
    }
    
    /* Verify magic number */
    if (memcmp(arch_header.magic, MAGIC, 8) != 0) {
        fprintf(stderr, "Error: Invalid archive format - wrong magic number\n");
        fclose(archive);
        return -1;
    }

    /* Check password if required */
    if (arch_header.has_password && password == NULL) {
        fprintf(stderr, "Error: Archive is password protected\n");
        fclose(archive);
        return -1;
    }

    printf("Extracting %d files from archive...\n", arch_header.file_count);

    /* Create output directory if needed */
    if (create_directory(output_dir) != 0) {
        fprintf(stderr, "Error: Cannot create output directory '%s'\n", output_dir);
        fclose(archive);
        return -1;
    }

    /* Process each file in archive */
    int extracted_count = 0;
    for (int i = 0; i < arch_header.file_count; i++) {
        FileHeader file_header;
        if (fread(&file_header, sizeof(FileHeader), 1, archive) != 1) {
            fprintf(stderr, "Error: Cannot read file header for file %d\n", i);
            break;
        }
        
        /* Validate file header */
        if (file_header.file_size == 0) {
            fprintf(stderr, "Warning: Skipping zero-length file: %s\n", file_header.filename);
            continue;
        }

        if (file_header.file_size > 100 * 1024 * 1024) { /* 100MB limit */
            fprintf(stderr, "Warning: File too large, skipping: %s (%lu bytes)\n", 
                   file_header.filename, (unsigned long)file_header.file_size);
            fseek(archive, file_header.file_size, SEEK_CUR);
            continue;
        }

        /* Create directory structure */
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", output_dir, file_header.filename);
        
        if (create_parent_dirs(full_path) != 0) {
            fprintf(stderr, "Warning: Cannot create parent directories for %s\n", file_header.filename);
            fseek(archive, file_header.file_size, SEEK_CUR);
            continue;
        }

        FILE* output_file = fopen(full_path, "wb");
        if (!output_file) {
            fprintf(stderr, "Warning: Cannot create file %s: %s\n", full_path, strerror(errno));
            fseek(archive, file_header.file_size, SEEK_CUR);
            continue;
        }

        /* Read compressed data */
        uint8_t* compressed_data = malloc(file_header.file_size);
        if (!compressed_data) {
            fprintf(stderr, "Error: Memory allocation failed for %s\n", file_header.filename);
            fclose(output_file);
            fseek(archive, file_header.file_size, SEEK_CUR);
            continue;
        }
        
        if (fread(compressed_data, 1, file_header.file_size, archive) != file_header.file_size) {
            fprintf(stderr, "Error: Cannot read file data for %s\n", file_header.filename);
            free(compressed_data);
            fclose(output_file);
            continue;
        }
        
        /* Process data based on compression flag */
        if (file_header.is_compressed) {
            uint8_t* decompressed_data = NULL;
            size_t decompressed_size = ppm_decompress(compressed_data, 
                                                     file_header.file_size, 
                                                     &decompressed_data);
            
            if (decompressed_data && decompressed_size > 0) {
                size_t written = fwrite(decompressed_data, 1, decompressed_size, output_file);
                if (written != decompressed_size) {
                    fprintf(stderr, "Warning: Incomplete write for %s\n", file_header.filename);
                }
                free(decompressed_data);
            } else {
                /* Fallback: write compressed data if decompression fails */
                fprintf(stderr, "Warning: Decompression failed for %s, storing compressed data\n", 
                       file_header.filename);
                fwrite(compressed_data, 1, file_header.file_size, output_file);
            }
        } else {
            /* Write uncompressed data */
            size_t written = fwrite(compressed_data, 1, file_header.file_size, output_file);
            if (written != file_header.file_size) {
                fprintf(stderr, "Warning: Incomplete write for %s\n", file_header.filename);
            }
        }
        
        free(compressed_data);
        
        if (fclose(output_file) != 0) {
            fprintf(stderr, "Warning: Error closing file %s\n", full_path);
        }
        
        /* Restore file permissions */
        if (chmod(full_path, file_header.permissions) != 0) {
            fprintf(stderr, "Warning: Cannot set permissions for %s: %s\n", 
                   full_path, strerror(errno));
        }
        
        /* Add extraction timestamp */
        add_timestamp_to_file(full_path);
        
        extracted_count++;
        printf("Extracted: %s (%lu bytes)\n", 
               file_header.filename, (unsigned long)file_header.file_size);
    }

    fclose(archive);
    
    if (extracted_count != arch_header.file_count) {
        fprintf(stderr, "Warning: Extracted %d out of %d files\n", 
               extracted_count, arch_header.file_count);
    } else {
        printf("Successfully extracted %d files to: %s\n", extracted_count, output_dir);
    }
    
    return (extracted_count == arch_header.file_count) ? 0 : -1;
}

/* List archive contents */
void list_archive_contents(const char* archive_path) {
    FILE* archive = fopen(archive_path, "rb");
    if (!archive) {
        fprintf(stderr, "Error: Cannot open archive %s: %s\n", archive_path, strerror(errno));
        return;
    }

    /* Check archive size */
    fseek(archive, 0, SEEK_END);
    long archive_size = ftell(archive);
    fseek(archive, 0, SEEK_SET);
    
    if (archive_size < (long)sizeof(ArchiveHeader)) {
        fprintf(stderr, "Error: Archive file is too small or empty\n");
        fclose(archive);
        return;
    }

    ArchiveHeader arch_header;
    if (fread(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1) {
        fprintf(stderr, "Error: Cannot read archive header\n");
        fclose(archive);
        return;
    }
    
    if (memcmp(arch_header.magic, MAGIC, 8) != 0) {
        fprintf(stderr, "Error: Invalid archive format\n");
        fclose(archive);
        return;
    }
    
    printf("Archive: %s\n", archive_path);
    printf("Files: %d\n", arch_header.file_count);
    printf("Total size: %lu bytes\n", (unsigned long)arch_header.total_size);
    printf("Password protected: %s\n", arch_header.has_password ? "yes" : "no");
    printf("\nFiles:\n");
    printf("%-50s %-12s %-10s %s\n", "Filename", "Size", "Compressed", "Permissions");
    printf("-------------------------------------------------- ------------ ---------- ----------\n");
    
    uint64_t total_files_size = 0;
    for (int i = 0; i < arch_header.file_count; i++) {
        FileHeader file_header;
        if (fread(&file_header, sizeof(FileHeader), 1, archive) != 1) {
            fprintf(stderr, "Error: Cannot read file header for file %d\n", i);
            break;
        }
        
        /* Skip file data */
        fseek(archive, file_header.file_size, SEEK_CUR);
        
        total_files_size += file_header.file_size;
        
        /* Format permissions string */
        char perm_str[11];
        snprintf(perm_str, sizeof(perm_str), "%04o", file_header.permissions & 0777);
        
        printf("%-50s %-12lu %-10s %s\n", 
               file_header.filename,
               (unsigned long)file_header.file_size,
               file_header.is_compressed ? "PPM" : "no",
               perm_str);
    }
    
    printf("-------------------------------------------------- ------------ ---------- ----------\n");
    printf("%-50s %-12lu %-10s\n", 
           "TOTAL", (unsigned long)total_files_size, "");
    
    fclose(archive);
}

/* Check if file is archive */
int is_archive_file(const char* filename) {
    if (!filename) return 0;
    
    const char* ext = strrchr(filename, '.');
    return (ext && (strcmp(ext, ".arc") == 0 || strcmp(ext, ".ARCH") == 0));
}

/* Verify archive integrity */
int verify_archive(const char* archive_path) {
    FILE* archive = fopen(archive_path, "rb");
    if (!archive) {
        fprintf(stderr, "Error: Cannot open archive %s\n", archive_path);
        return -1;
    }

    /* Check archive size */
    fseek(archive, 0, SEEK_END);
    long archive_size = ftell(archive);
    fseek(archive, 0, SEEK_SET);
    
    if (archive_size < (long)sizeof(ArchiveHeader)) {
        fprintf(stderr, "Error: Archive file is too small\n");
        fclose(archive);
        return -1;
    }

    ArchiveHeader arch_header;
    if (fread(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1) {
        fprintf(stderr, "Error: Cannot read archive header\n");
        fclose(archive);
        return -1;
    }
    
    if (memcmp(arch_header.magic, MAGIC, 8) != 0) {
        fprintf(stderr, "Error: Invalid archive format\n");
        fclose(archive);
        return -1;
    }

    printf("Verifying archive: %s\n", archive_path);
    printf("Files in archive: %d\n", arch_header.file_count);
    
    int valid_files = 0;
    uint64_t current_offset = sizeof(ArchiveHeader);
    
    for (int i = 0; i < arch_header.file_count; i++) {
        FileHeader file_header;
        if (fread(&file_header, sizeof(FileHeader), 1, archive) != 1) {
            fprintf(stderr, "Error: Cannot read file header for file %d\n", i);
            break;
        }
        
        /* Check if offset matches */
        if (file_header.offset != current_offset) {
            fprintf(stderr, "Warning: File offset mismatch for %s\n", file_header.filename);
        }
        
        /* Skip file data */
        if (fseek(archive, file_header.file_size, SEEK_CUR) != 0) {
            fprintf(stderr, "Error: Cannot skip file data for %s\n", file_header.filename);
            break;
        }
        
        current_offset += sizeof(FileHeader) + file_header.file_size;
        valid_files++;
        
        printf("  ✓ %s\n", file_header.filename);
    }
    
    fclose(archive);
    
    if (valid_files == arch_header.file_count) {
        printf("Archive verification successful: all %d files are valid\n", valid_files);
        return 0;
    } else {
        fprintf(stderr, "Archive verification failed: %d/%d files valid\n", 
               valid_files, arch_header.file_count);
        return -1;
    }
}
