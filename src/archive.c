#include "archive.h"

/* Create archive from directory */
int create_archive(const char* dir_path, const char* archive_path, const char* password, int vflag){
	/* Check if source directory exists */
	struct stat dir_stat;
	if(stat(dir_path, &dir_stat) != 0 || !S_ISDIR(dir_stat.st_mode))
		printErr("%d: Error: Source directory '%s' does not exist or is not a directory\n", __LINE__ - 1, dir_path);

	FILE* archive = fopen(archive_path, "wb");
	if(!archive)
		printErr("%d: Error: Cannot create archive file '%s': %s\n", __LINE__ - 2, archive_path, strerror(errno));

	/* Write archive header */
	ArchiveHeader arch_header;
	memcpy(arch_header.magic, MAGIC, 8);
	arch_header.file_count = 0;
	arch_header.total_size = sizeof(ArchiveHeader);
	arch_header.has_password = (password != NULL) ? 1 : 0;

	if(fwrite(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1){
		fclose(archive);
		printErr("%d: Error: Cannot write archive header\n", __LINE__ - 2);
	}

	/* Process directory recursively */
	if(vflag == 1)
		fprintf(stdout, "Scanning directory: %s\n", dir_path);
	process_directory(dir_path, "", archive, &arch_header.file_count, &arch_header.total_size, vflag);

	if(arch_header.file_count == 0)
		printErr("%d: Warning: No files found to archive\n", __LINE__ - 1);

	/* Update header with actual counts */
	fseek(archive, 0, SEEK_SET);
	if(fwrite(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1){
		fclose(archive);
		printErr("%d: Error: Cannot update archive header\n", __LINE__ - 2);
	}

	fclose(archive);

	/* Add timestamp to archive file */
	add_timestamp_to_file(archive_path);

	fprintf(stdout, "Archive created successfully: %s\n", archive_path);
	if(vflag == 1)
		fprintf(stdout, "Total files: %d, Archive size: %lu bytes\n", arch_header.file_count, (unsigned long)arch_header.total_size);

	return 0;
}

/* Extract archive to directory */
int extract_archive(const char* archive_path, const char* output_dir, const char* password, int vflag){
	/* Check if archive file exists */
	struct stat archive_stat;
	if(stat(archive_path, &archive_stat) != 0)
		printErr("%d: Error: Archive file '%s' does not exist\n", __LINE__ - 1, archive_path);

	FILE* archive = fopen(archive_path, "rb");
	if(!archive)
		printErr("%d: Error: Cannot open archive file '%s': %s\n", __LINE__ - 2, archive_path, strerror(errno));

	long int archive_size = getFileSize(archive);

	if(archive_size < (long)sizeof(ArchiveHeader)){
		fclose(archive);
		printErr("%d: Error: Archive file is too small or empty\n", __LINE__ - 2);
	}

	/* Read archive header */
	ArchiveHeader arch_header;
	if(fread(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1) {
		fclose(archive);
		printErr("%d: Error: Cannot read archive header\n", __LINE__ - 2);
	}

	/* Verify magic number */
	if(memcmp(arch_header.magic, MAGIC, 8) != 0){
		fclose(archive);
		printErr("%d: Error: Invalid archive format - wrong magic number\n", __LINE__ - 2);
	}

	/* Check password if required */
	if(arch_header.has_password && password == NULL){
		fclose(archive);
		printErr("%d: Error: Archive is password protected\n", __LINE__ - 2);
	}

	if(vflag == 1)
		fprintf(stdout, "Extracting %d files from archive...\n", arch_header.file_count);

	/* Create output directory if needed */
	if(create_directory(output_dir) != 0){
		fclose(archive);
		printErr("%d: Error: Cannot create output directory '%s'\n", __LINE__ - 2,output_dir);
	}

	/* Process each file in archive */
	int extracted_count = 0;
	FileHeader file_header = {0};
	for(int i = 0; i < arch_header.file_count; i++){
		memset(&file_header, 0, sizeof(FileHeader));
		if(fread(&file_header, sizeof(FileHeader), 1, archive) != 1){
			printErrNE("%d: Error: Cannot read file header for file %d\n", __LINE__ - 1, i);
			break;
		}

		/* Validate file header */
		if (file_header.file_size == 0) {
			printErrNE("%d: Warning: Skipping zero-length file: %s\n", __LINE__ - 1, file_header.filename);
			continue;
		}

		if(file_header.file_size > 100 * 1024 * 1024){ /* 100MB limit */
			fprintf(stderr, "Warning: File too large, skipping: %s (%lu bytes)\n", file_header.filename, (unsigned long)file_header.file_size);
			fseek(archive, file_header.file_size, SEEK_CUR);
			continue;
		}

		/* Create directory structure */
		char full_path[1024] = {0};
		snprintf(full_path, sizeof(full_path), "%s/%s", output_dir, file_header.filename);

		if(create_parent_dirs(full_path) != 0){
			fprintf(stderr, "Warning: Cannot create parent directories for %s\n", file_header.filename);
			fseek(archive, file_header.file_size, SEEK_CUR);
			continue;
		}

		FILE* output_file = fopen(full_path, "wb");
		if(!output_file){
			printErrNE("%d: Warning: Cannot create file %s: %s\n", __LINE__ - 2, full_path, strerror(errno));
			fseek(archive, file_header.file_size, SEEK_CUR);
			continue;
		}

		/* Read compressed data */
		uint8_t* compressed_data = calloc(file_header.file_size, sizeof(uint8_t));
		if(!compressed_data){
			printErrNE("%d: Error: Memory allocation failed for %s\n", __LINE__ - 1, file_header.filename);
			fclose(output_file);
			fseek(archive, file_header.file_size, SEEK_CUR);
			continue;
		}

		if(fread(compressed_data, 1, file_header.file_size, archive) != file_header.file_size){
			printErrNE("%d: Error: Cannot read file data for %s\n", __LINE__ - 1, file_header.filename);
			free(compressed_data);
			fclose(output_file);
			continue;
		}

		/* Process data based on compression flag */
		if(file_header.is_compressed){
			uint8_t* decompressed_data = NULL;
			size_t decompressed_size = ppm_decompress(compressed_data, file_header.file_size, &decompressed_data);

			if(decompressed_data && decompressed_size > 0){
			size_t written = fwrite(decompressed_data, 1, decompressed_size, output_file);
			if(written != decompressed_size)
				printErr("%d: Warning: Incomplete write for %s\n", __LINE__ - 2, file_header.filename);
			free(decompressed_data);
			} else {
				/* Fallback: write compressed data if decompression fails */
				fwrite(compressed_data, 1, file_header.file_size, output_file);
				printErrNE("%d: Warning: Decompression failed for %s, storing compressed data\n", __LINE__ - 12,file_header.filename);
			}
		} else {
			/* Write uncompressed data */
			size_t written = fwrite(compressed_data, 1, file_header.file_size, output_file);
			if(written != file_header.file_size)
				printErrNE("%d: Warning: Incomplete write for %s\n", __LINE__ - 2, file_header.filename);
		}

		free(compressed_data);

		if(fclose(output_file) != 0)
		    printErr("%d: Warning: Error closing file %s\n", __LINE__ - 1, full_path);

		/* Restore file permissions */
		if(chmod(full_path, file_header.permissions) != 0)
		    printErrNE("%d: Warning: Cannot set permissions for %s: %s\n", __LINE__ - 1, full_path, strerror(errno));

		/* Add extraction timestamp */
		add_timestamp_to_file(full_path);

		extracted_count++;
		if(vflag == 1)
			fprintf(stdout, "Extracted: %s (%lu bytes)\n", file_header.filename, (unsigned long)file_header.file_size);
	}

	fclose(archive);

	if(extracted_count != arch_header.file_count){
		if(vflag == 1)
			printErr("%d: Warning: Extracted %d out of %d files\n", __LINE__ - 1, extracted_count, arch_header.file_count);
	} else
		if(vflag == 1)
			printf("Successfully extracted %d files to: %s\n", extracted_count, output_dir);

	return (extracted_count == arch_header.file_count) ? 0 : -1;
}

/* List archive contents */
void list_archive_contents(const char* archive_path) {
	FILE* archive = fopen(archive_path, "rb");
	if(!archive)
		printErr("%d: Error: Cannot open archive %s: %s\n", __LINE__ - 2, archive_path, strerror(errno));

	long int archive_size = getFileSize(archive);

	if(archive_size < (long)sizeof(ArchiveHeader)){
		fclose(archive);
		printErr("%d: Error: Archive file is empty\n", __LINE__ - 4);
	}

	ArchiveHeader arch_header;
	if(fread(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1){
		fclose(archive);
		printErr("%d: Error: Cannot read archive header\n", __LINE__ - 2);
	}

	if(memcmp(arch_header.magic, MAGIC, 8) != 0){
		fclose(archive);
		printErr("%d: Error: Invalid archive format\n", __LINE__ - 2);
	}

	fprintf(stdout, "Archive: %s\n", archive_path);
	fprintf(stdout, "Files: %d\n", arch_header.file_count);
	fprintf(stdout, "Total size: %lu bytes\n", (unsigned long)arch_header.total_size);
	fprintf(stdout, "Password protected: %s\n", arch_header.has_password ? "yes" : "no");
	fprintf(stdout, "\nFiles:\n");
	fprintf(stdout, "%-50s %-12s %-10s %s\n", "Filename", "Size", "Compressed", "Permissions");
	fprintf(stdout, "-------------------------------------------------- ------------ ---------- ----------\n");

	uint64_t total_files_size = 0;
	FileHeader file_header = {0};
	for(int i = 0; i < arch_header.file_count; i++){
		memset(&file_header, 0, sizeof(FileHeader));
		if(fread(&file_header, sizeof(FileHeader), 1, archive) != 1){
			printErrNE("%d: Error: Cannot read file header for file %d\n", __LINE__ - 1, i);
			break;
		}

		/* Skip file data */
		fseek(archive, file_header.file_size, SEEK_CUR);

		total_files_size += file_header.file_size;

		/* Format permissions string */
		char perm_str[11];
		snprintf(perm_str, sizeof(perm_str), "%04o", file_header.permissions & 0777);

		printf("%-50s %-12lu %-10s %s\n", file_header.filename,(unsigned long)file_header.file_size,
			file_header.is_compressed ? "PPM" : "no", perm_str);
	}

	printf("-------------------------------------------------- ------------ ---------- ----------\n");
	printf("%-50s %-12lu %-10s\n", "TOTAL", (unsigned long)total_files_size, "");

	fclose(archive);
}

/* Check if file is archive */
int is_archive_file(const char* filename) {
	if (!filename) return 0;

	const char* ext = strrchr(filename, '.');
	return (ext && (strcmp(ext, ".zov") == 0 || strcmp(ext, ".fem") == 0));
}

/* Verify archive integrity */
int verify_archive(const char* archive_path) {
	FILE* archive = fopen(archive_path, "rb");
	if(!archive)
		printErr("%d: Error: Cannot open archive %s\n", __LINE__ - 2, archive_path);

	long int archive_size = getFileSize(archive);

	if(archive_size < (long)sizeof(ArchiveHeader)){
		fclose(archive);
		printErr("%d: Error: Archive file is too small\n", __LINE__ - 2);
	}

	ArchiveHeader arch_header;
	if(fread(&arch_header, sizeof(ArchiveHeader), 1, archive) != 1){
		fclose(archive);
		printErr("%d: Error: Cannot read archive header\n", __LINE__ - 2);
	}

	if(memcmp(arch_header.magic, MAGIC, 8) != 0){
		fclose(archive);
		printErr("%d: Error: Invalid archive format\n", __LINE__ - 2);
	}

	fprintf(stdout, "Verifying archive: %s\n", archive_path);
	fprintf(stdout, "Files in archive: %d\n", arch_header.file_count);
    
	int valid_files = 0;
	uint64_t current_offset = sizeof(ArchiveHeader);
    
	FileHeader file_header = {0};
	for(int i = 0; i < arch_header.file_count; i++){
		memset(&file_header, 0, sizeof(FileHeader));
		if (fread(&file_header, sizeof(FileHeader), 1, archive) != 1) {
			printErrNE("%d: Error: Cannot read file header for file %d\n", __LINE__ - 1, i);
			break;
		}

		/* Check if offset matches */
		if (file_header.offset != current_offset) {
			printErrNE("%d: Warning: File offset mismatch for %s\n", __LINE__ - 1, file_header.filename);
		}

		/* Skip file data */
		if (fseek(archive, file_header.file_size, SEEK_CUR) != 0) {
			printErr("%d: Error: Cannot skip file data for %s\n", __LINE__ - 1, file_header.filename);
			break;
		}

		current_offset += sizeof(FileHeader) + file_header.file_size;
		valid_files++;

		fprintf(stdout, "  ✓ %s\n", file_header.filename);
	}
    
	fclose(archive);

	if (valid_files == arch_header.file_count){
		fprintf(stdout, "Archive verification successful: all %d files are valid\n", valid_files);
	} else
		printErr("%d: Archive verification failed: %d/%d files valid\n", __LINE__ - 4, valid_files, arch_header.file_count);
	return 0;
}
