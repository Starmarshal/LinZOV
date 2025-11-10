#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "archive.h"

#define VERSION "1.0"
#define BUILD_DATE __DATE__

/* Print usage information */
void print_usage(const char* program_name) {
    printf("Archive Utility v%s - PPM Compression Tool\n", VERSION);
    printf("Built: %s\n\n", BUILD_DATE);
    printf("Usage: %s <command> [arguments]\n\n", program_name);
    printf("Commands:\n");
    printf("  create <directory> <archive>     Create archive from directory\n");
    printf("  extract <archive> <directory>    Extract archive to directory\n");
    printf("  list <archive>                   List archive contents\n");
    printf("  verify <archive>                 Verify archive integrity\n");
    printf("  info <archive>                   Show archive information\n\n");
    printf("Options:\n");
    printf("  -h, --help                      Show this help message\n");
    printf("  -v, --version                   Show version information\n\n");
    printf("Examples:\n");
    printf("  %s create ./documents my_files.arc\n", program_name);
    printf("  %s extract my_files.arc ./extracted\n", program_name);
    printf("  %s list my_files.arc\n", program_name);
    printf("  %s verify my_files.arc\n", program_name);
    printf("  %s info my_files.arc\n", program_name);
}

/* Print version information */
void print_version() {
    printf("Archiver v%s\n", VERSION);
    printf("A file archiver with PPM compression algorithm\n");
    printf("Built for Void Linux on %s\n", BUILD_DATE);
    printf("License: MIT\n");
}

/* Show detailed archive information */
void show_archive_info(const char* archive_path) {
    FILE* archive = fopen(archive_path, "rb");
    if (!archive) {
        fprintf(stderr, "Error: Cannot open archive %s\n", archive_path);
        return;
    }

    /* Get file size */
    fseek(archive, 0, SEEK_END);
    long file_size = ftell(archive);
    fseek(archive, 0, SEEK_SET);

    if (file_size < (long)sizeof(ArchiveHeader)) {
        fprintf(stderr, "Error: File is too small to be a valid archive\n");
        fclose(archive);
        return;
    }

    ArchiveHeader arch_header;
    if (fread(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1) {
        fprintf(stderr, "Error: Cannot read archive header\n");
        fclose(archive);
        return;
    }

    if (memcmp(arch_header.magic, "ARCHv1.0", 8) != 0) {
        fprintf(stderr, "Error: Not a valid archive file\n");
        fclose(archive);
        return;
    }

    printf("Archive Information:\n");
    printf("====================\n");
    printf("File: %s\n", archive_path);
    printf("Size: %ld bytes\n", file_size);
    printf("Format: ARCHv1.0\n");
    printf("File count: %d\n", arch_header.file_count);
    printf("Total archive size: %lu bytes\n", (unsigned long)arch_header.total_size);
    printf("Password protected: %s\n", arch_header.has_password ? "yes" : "no");
    
    /* Calculate compression ratio if possible */
    if (file_size > 0) {
        double ratio = ((double)arch_header.total_size / file_size) * 100.0;
        printf("Structure overhead: %.2f%%\n", 100.0 - ratio);
    }

    fclose(archive);
}

/* Interactive mode for archive creation */
void interactive_create() {
    char source_dir[512];
    char archive_name[512];
    
    printf("Interactive Archive Creation\n");
    printf("============================\n");
    
    printf("Enter source directory path: ");
    if (fgets(source_dir, sizeof(source_dir), stdin) == NULL) {
        fprintf(stderr, "Error reading input\n");
        return;
    }
    
    /* Remove newline character */
    source_dir[strcspn(source_dir, "\n")] = 0;
    
    printf("Enter archive name: ");
    if (fgets(archive_name, sizeof(archive_name), stdin) == NULL) {
        fprintf(stderr, "Error reading input\n");
        return;
    }
    
    /* Remove newline character */
    archive_name[strcspn(archive_name, "\n")] = 0;
    
    /* Add .arc extension if not present */
    if (!strstr(archive_name, ".arc")) {
        strcat(archive_name, ".arc");
    }
    
    printf("\nCreating archive '%s' from directory '%s'\n", archive_name, source_dir);
    printf("Using PPM compression...\n\n");
    
    if (create_archive(source_dir, archive_name, NULL) == 0) {
        printf("\nArchive created successfully!\n");
    } else {
        fprintf(stderr, "\nFailed to create archive.\n");
    }
}

/* Interactive mode for archive extraction */
void interactive_extract() {
    char archive_path[512];
    char output_dir[512];
    
    printf("Interactive Archive Extraction\n");
    printf("==============================\n");
    
    printf("Enter archive path: ");
    if (fgets(archive_path, sizeof(archive_path), stdin) == NULL) {
        fprintf(stderr, "Error reading input\n");
        return;
    }
    
    /* Remove newline character */
    archive_path[strcspn(archive_path, "\n")] = 0;
    
    printf("Enter output directory: ");
    if (fgets(output_dir, sizeof(output_dir), stdin) == NULL) {
        fprintf(stderr, "Error reading input\n");
        return;
    }
    
    /* Remove newline character */
    output_dir[strcspn(output_dir, "\n")] = 0;
    
    printf("\nExtracting archive '%s' to directory '%s'\n", archive_path, output_dir);
    printf("Using PPM decompression...\n\n");
    
    if (extract_archive(archive_path, output_dir, NULL) == 0) {
        printf("\nArchive extracted successfully!\n");
    } else {
        fprintf(stderr, "\nFailed to extract archive.\n");
    }
}

/* Show interactive menu */
void show_interactive_menu() {
    int choice;
    
    printf("\nArchive Utility - Interactive Mode\n");
    printf("==================================\n");
    printf("1. Create archive\n");
    printf("2. Extract archive\n");
    printf("3. List archive contents\n");
    printf("4. Verify archive\n");
    printf("5. Show archive info\n");
    printf("6. Exit\n");
    printf("\nEnter your choice (1-6): ");
    
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Invalid input\n");
        return;
    }
    
    /* Clear input buffer */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    switch (choice) {
        case 1:
            interactive_create();
            break;
        case 2:
            interactive_extract();
            break;
        case 3:
            {
                char archive_path[512];
                printf("Enter archive path: ");
                if (fgets(archive_path, sizeof(archive_path), stdin)) {
                    archive_path[strcspn(archive_path, "\n")] = 0;
                    list_archive_contents(archive_path);
                }
            }
            break;
        case 4:
            {
                char archive_path[512];
                printf("Enter archive path: ");
                if (fgets(archive_path, sizeof(archive_path), stdin)) {
                    archive_path[strcspn(archive_path, "\n")] = 0;
                    verify_archive(archive_path);
                }
            }
            break;
        case 5:
            {
                char archive_path[512];
                printf("Enter archive path: ");
                if (fgets(archive_path, sizeof(archive_path), stdin)) {
                    archive_path[strcspn(archive_path, "\n")] = 0;
                    show_archive_info(archive_path);
                }
            }
            break;
        case 6:
            printf("Goodbye!\n");
            exit(0);
            break;
        default:
            fprintf(stderr, "Invalid choice. Please enter 1-6.\n");
            break;
    }
}

/* Main function */
int main(int argc, char* argv[]) {
    /* If no arguments, show interactive mode */
    if (argc == 1) {
        show_interactive_menu();
        return 0;
    }
    
    const char* command = argv[1];
    
    /* Handle help and version flags */
    if (strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    } else if (strcmp(command, "--version") == 0 || strcmp(command, "-v") == 0) {
        print_version();
        return 0;
    } else if (strcmp(command, "interactive") == 0 || strcmp(command, "i") == 0) {
        show_interactive_menu();
        return 0;
    }
    
    /* Handle archive commands */
    if (strcmp(command, "create") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Error: Missing arguments for create command\n");
            fprintf(stderr, "Usage: %s create <directory> <archive>\n", argv[0]);
            return 1;
        }
        
        const char* directory = argv[2];
        const char* archive = argv[3];
        
        printf("Creating archive '%s' from directory '%s'\n", archive, directory);
        printf("Using PPM compression algorithm...\n");
        
        if (create_archive(directory, archive, NULL) != 0) {
            fprintf(stderr, "Error: Failed to create archive\n");
            return 1;
        }
        
        printf("Archive created successfully!\n");
        
    } else if (strcmp(command, "extract") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Error: Missing arguments for extract command\n");
            fprintf(stderr, "Usage: %s extract <archive> <directory>\n", argv[0]);
            return 1;
        }
        
        const char* archive = argv[2];
        const char* directory = argv[3];
        
        printf("Extracting archive '%s' to directory '%s'\n", archive, directory);
        
        if (extract_archive(archive, directory, NULL) != 0) {
            fprintf(stderr, "Error: Failed to extract archive\n");
            return 1;
        }
        
        printf("Archive extracted successfully!\n");
        
    } else if (strcmp(command, "list") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing archive file for list command\n");
            fprintf(stderr, "Usage: %s list <archive>\n", argv[0]);
            return 1;
        }
        
        list_archive_contents(argv[2]);
        
    } else if (strcmp(command, "verify") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing archive file for verify command\n");
            fprintf(stderr, "Usage: %s verify <archive>\n", argv[0]);
            return 1;
        }
        
        if (verify_archive(argv[2]) != 0) {
            fprintf(stderr, "Archive verification failed!\n");
            return 1;
        }
        
    } else if (strcmp(command, "info") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing archive file for info command\n");
            fprintf(stderr, "Usage: %s info <archive>\n", argv[0]);
            return 1;
        }
        
        show_archive_info(argv[2]);
        
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        fprintf(stderr, "Use '%s --help' for usage information.\n", argv[0]);
        return 1;
    }
    
    return 0;
}
