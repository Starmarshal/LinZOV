#include "zov.h"

static int print_usage(const char* program_name);
static int print_version();
static int show_archive_info(const char* archive_path);

/* Print usage information */
int print_usage(const char* program_name){
	fprintf(stdout, "Archive Utility v%s - PPM Compression Tool\n", VERSION);
	fprintf(stdout, "Built: %s\n\n", BUILD_DATE);
	fprintf(stdout, "Usage: %s <command> [arguments]\n\n", program_name);
	fprintf(stdout, "Commands:\n");
	fprintf(stdout, "  c <archive>  <directory>    Create archive from directory\n");
	fprintf(stdout, "  x <archive>  <directory>    Extract archive to directory\n");
	fprintf(stdout, "  l <archive>                   List archive contents\n");
	fprintf(stdout, "  e <archive>                 Verify archive integrity\n");
	fprintf(stdout, "  i <archive>                   Show archive information\n\n");
	fprintf(stdout, "  v 	                   	Verbose\n\n");
	fprintf(stdout, "  V, --version	                   Show version information\n\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "  h	                      Show this help message\n");
	fprintf(stdout, "Examples:\n");
	exit(0);
}

/* Print version information */
int print_version(){
	fprintf(stdout, "ZOV v%s\n", VERSION);
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

	if(memcmp(arch_header.magic, MAGIC, 8) != 0){
		fclose(archive);
		printErr("%d: Error: Not a valid archive file\n", __LINE__ - 2);
	}
	fprintf(stdout, "Archive Information:\n");
	fprintf(stdout, "====================\n");
	fprintf(stdout, "File: %s\n", archive_path);
	fprintf(stdout, "Size: %ld bytes\n", archive_size);
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

/* Main function */
int main(int argc, char* argv[]) {
	/* If no arguments, show help */
	if(strcmp(argv[1], "--help") == 0)
		print_usage(argv[0]);
	if(strcmp(argv[1], "--version") == 0)
		print_version();

	int state = 0, vflag = 0;

	char opt[BUFFER] = {0};
	strcpy(opt, argv[1]);
	for(size_t i = 0; i < strlen(opt); ++i){
		switch(opt[i]){
			case 'x':
				/* extract flag */
				state = 1;
				break;
			case 'c':
				/* compress flag */
				state = 2;
				break;
			case 'l':
				/* list flag */
				state = 3;
				break;
			case 'v':
				/* verbose flag */
				vflag = 1;
				break;
			case 'V':
				/* version flag */
				print_version();
				break;
			case 'e':
				/* verify flag */
				state = 4;
				break;
			case 'i':
				/* info flag */
				state = 5;
				break;
			default:
				printf("unknown flag: %c", opt[i]);
				break;
		}
	}

	if(argc <= 2)
		printErr("Usage: zov <flags> <argument> ...\n");

	char directory[BUFFER];
	if(argc >= 3 && state == 1)
		strcpy(directory, ".");
	else if(state == 2)
		strcpy(directory, argv[3]);
	else
		strcpy(directory, argv[2]);
	const char* archive = argv[2];

	/* Handle archive commands */
	switch(state){
		case 1:
			if(argc < 3)
				printErr("%d: Error: Missing arguments for extract command\n \
				Usage: %s x <archive>\n", __LINE__, argv[0]);

			if(vflag == 1)
				fprintf(stdout, "Extracting archive: %s to directory %s\n", archive, directory);
			
			if(extract_archive(archive, directory, NULL, vflag) != 0)
				printErr("%d: Error: Failed to extract archive\n", __LINE__);
			
			
			if(vflag == 1)
				fprintf(stdout, "Archive extracted successfully!\n");
			break;
		case 2:
			if(argc < 4)
				printErr("%d: Error: Missing arguments for create command\n \
					Usage: %s c <directory> <archive>\n", __LINE__, argv[0]);
			
			if(vflag == 1)
				fprintf(stdout, "Creating archive '%s' from directory '%s'\n  \
					Using PPM compression algorithm...\n", archive, directory);
			
			if(create_archive(directory, archive, NULL, vflag) != 0)
				printErr("%d: Error: Failed to create archive\n", __LINE__ - 1);
			
			if(vflag == 1)
				fprintf(stdout, "Archive created successfully!\n");
			break;
		case 3:
			if (argc < 3)
				printErr("%d: Error: Missing archive file for list command\n \
				Usage: %s l <archive>\n", __LINE__, argv[0]);
		
			list_archive_contents(argv[2]);
			break;
        
		case 4:
			if(argc < 3)
				printErr("%d: Error: Missing archive file for verify command\n \
				Usage: %s e <archive>\n", __LINE__, argv[0]);
		
			if(verify_archive(argv[2]) != 0)
				printErr("%d: Archive verification failed!\n", __LINE__);
			break;
        
		case 5:
			if(argc < 3)
				printErr("%dError: Missing archive file for info command\n \
				Usage: %s i <archive>\n", argv[0]);
			
			show_archive_info(argv[2]);
			break;
		
		default:
			printErr("Error: Unknown command \'%s\'");
			break;
	}
    return 0;
}
