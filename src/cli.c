#include "cli.h"

/* Print usage information */
int print_usage(const char* program_name){
	fprintf(stdout, "Archive Utility v%s - PPM Compression Tool\n", VERSION);
	fprintf(stdout, "Built: %s\n\n", BUILD_DATE);
	fprintf(stdout, "Usage: %s <command> [arguments]\n\n", program_name);
	fprintf(stdout, "Commands:\n");
	fprintf(stdout, "  create <directory> <archive>     Create archive from directory\n");
	fprintf(stdout, "  extract <archive> <directory>    Extract archive to directory\n");
	fprintf(stdout, "  list <archive>                   List archive contents\n");
	fprintf(stdout, "  verify <archive>                 Verify archive integrity\n");
	fprintf(stdout, "  info <archive>                   Show archive information\n\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "  -h, --help                      Show this help message\n");
	fprintf(stdout, "  -v, --version                   Show version information\n\n");
	fprintf(stdout, "Examples:\n");
	fprintf(stdout, "  %s create ./documents my_files.arc\n", program_name);
	fprintf(stdout, "  %s extract my_files.arc ./extracted\n", program_name);
	fprintf(stdout, "  %s list my_files.arc\n", program_name);
	fprintf(stdout, "  %s verify my_files.arc\n", program_name);
	fprintf(stdout, "  %s info my_files.arc\n", program_name);
	exit(0);
}

/* Print version information */
int print_version(){
	fprintf(stdout, "Archiver v%s\n", VERSION);
	fprintf(stdout, "A file archiver with PPM compression algorithm\n");
	fprintf(stdout, "Built by POSIX %s\n", BUILD_DATE);
	fprintf(stdout, "License: GNU GPL v3\n");
	exit(0);
}

/* Show detailed archive information */
int show_archive_info(const char* archive_path){
	FILE* archive = fopen(archive_path, "rb");
	if(!archive)
		printErr("%d: Error: Cannot open archive %s\n", __LINE__ - 2, archive_path);

	long int archive_size = getFileSize(archive);
	if(archive_size < (long)sizeof(ArchiveHeader)){
		fclose(archive);
		printErr("%d: Error: File is too small to be a valid archive\n", __LINE__ - 5);
	}

	ArchiveHeader arch_header;
	if(fread(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1){
		fclose(archive);
		printErr("%d: Error: Cannot read archive header\n", __LINE__ - 2);
	}

	if(memcmp(arch_header.magic, "ARCHv1.0", 8) != 0){
		fclose(archive);
		printErr("%d: Error: Not a valid archive file\n", __LINE__ - 2);
	}

	fprintf(stdout, "Archive Information:\n");
	fprintf(stdout, "====================\n");
	fprintf(stdout, "File: %s\n", archive_path);
	fprintf(stdout, "Size: %ld bytes\n", archive_size);
	fprintf(stdout, "Format: ARCHv1.0\n");
	fprintf(stdout, "File count: %d\n", arch_header.file_count);
	fprintf(stdout, "Total archive size: %lu bytes\n", (unsigned long)arch_header.total_size);
	fprintf(stdout, "Password protected: %s\n", arch_header.has_password ? "yes" : "no");

	/* Calculate compression ratio if possible */
	if(archive_size > 0){
		double ratio = ((double)arch_header.total_size / archive_size) * 100.0;
		fprintf(stdout, "Structure overhead: %.2f%%\n", 100.0f - ratio);
	}

	fclose(archive);
	exit(0);
}

/* Interactive mode for archive creation */
void interactive_create(){
	char source_dir[512];
	char archive_name[512];

	fprintf(stdout, "Interactive Archive Creation\n");
	fprintf(stdout, "============================\n");

	fprintf(stdout, "Enter source directory path: ");
	if(fgets(source_dir, sizeof(source_dir), stdin) == NULL)
		printErr("%d: Error reading input\n", __LINE__ - 1);

	/* Remove newline character */
	source_dir[strcspn(source_dir, "\n")] = 0;

	fprintf(stdout, "Enter archive name: ");
	if(fgets(archive_name, sizeof(archive_name), stdin) == NULL)
		printErr("%d: Error reading input\n", __LINE__ - 1);

	/* Remove newline character */
	archive_name[strcspn(archive_name, "\n")] = 0;

	/* Add .arc extension if not present */
	if(!strstr(archive_name, ".arc"))
		strcat(archive_name, ".arc");

	fprintf(stdout, "\nCreating archive '%s' from directory '%s'\n", archive_name, source_dir);
	fprintf(stdout, "Using PPM compression...\n\n");

	if(create_archive(source_dir, archive_name, NULL) == 0)
		fprintf(stdout, "\nArchive created successfully!\n");
	else
		printErr("\n%d: Failed to create archive.\n", __LINE__ - 3);
}

/* Interactive mode for archive extraction */
void interactive_extract(){
	char archive_path[512];
	char output_dir[512];

	fprintf(stdout, "Interactive Archive Extraction\n");
	fprintf(stdout, "==============================\n");

	printf("Enter archive path: ");
	if(fgets(archive_path, sizeof(archive_path), stdin) == NULL)
		printErr("%d: Error reading input\n", __LINE__ - 1);

	/* Remove newline character */
	archive_path[strcspn(archive_path, "\n")] = 0;

	fprintf(stdout, "Enter output directory: ");
	if(fgets(output_dir, sizeof(output_dir), stdin) == NULL)
		printErr("%d: Error reading input\n", __LINE__ - 1);

	/* Remove newline character */
	output_dir[strcspn(output_dir, "\n")] = 0;

	fprintf(stdout, "\nExtracting archive '%s' to directory '%s'\n", archive_path, output_dir);
	fprintf(stdout, "Using PPM decompression...\n\n");

	if (extract_archive(archive_path, output_dir, NULL) == 0)
		fprintf(stdout, "\nArchive extracted successfully!\n");
	else
		printErr("\n%d: Failed to extract archive.\n", __LINE__ - 3);
}

/* Show interactive menu */
void show_interactive_menu(){
	int choice;

	fprintf(stdout, "\nArchive Utility - Interactive Mode\n");
	fprintf(stdout, "==================================\n");
	fprintf(stdout, "1. Create archive\n");
	fprintf(stdout, "2. Extract archive\n");
	fprintf(stdout, "3. List archive contents\n");
	fprintf(stdout, "4. Verify archive\n");
	fprintf(stdout, "5. Show archive info\n");
	fprintf(stdout, "6. Exit\n");
	fprintf(stdout, "\nEnter your choice (1-6): ");

	if(scanf("%d", &choice) != 1)
		printErr("%d: Invalid input\n", __LINE__ - 1);

	/* Clear input buffer */
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
    
	char archive_path[512];
	switch (choice) {
		case 1:
			interactive_create();
			break;
		case 2:
			interactive_extract();
			break;
		case 3:
			fprintf(stdout, "Enter archive path: ");
			if(fgets(archive_path, sizeof(archive_path), stdin)){
				archive_path[strcspn(archive_path, "\n")] = 0;
				list_archive_contents(archive_path);
			}
			break;
		case 4:
			fprintf(stdout, "Enter archive path: ");
			if(fgets(archive_path, sizeof(archive_path), stdin)){
				archive_path[strcspn(archive_path, "\n")] = 0;
				verify_archive(archive_path);
			}
            	break;
		case 5:
			fprintf(stdout, "Enter archive path: ");
			if(fgets(archive_path, sizeof(archive_path), stdin)){
				archive_path[strcspn(archive_path, "\n")] = 0;
				show_archive_info(archive_path);
			}
			break;
		case 6:
			fprintf(stdout, "Goodbye!\n");
			exit(0);
			break;
		default:
			printErrNE("Invalid choice. Please enter 1-6.\n");
			break;
    }
}
