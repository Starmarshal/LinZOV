#include "cli.h"

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
