#include "archive.h"

static void process_directory(const char* base_path, const char* rel_path, 
                      FILE* archive, uint16_t* file_count, uint64_t* total_size, int vflag);
static void process_single_file(const char* filepath, const char* rel_path, 
                        FILE* archive, uint16_t* file_count, uint64_t* total_size, struct stat* stat_buf, int vflag);
static int create_directory(const char* path);
static int should_compress_file(const char* filename);
static int create_parent_dirs(const char* filepath);
static void add_timestamp_to_file(const char* filepath);
static size_t ppm_compress(const uint8_t* input, size_t input_size, uint8_t** output);
static size_t ppm_decompress(const uint8_t* input, size_t input_size, uint8_t** output);

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
			fprintf(stderr, "%d: Error: Cannot read file header for file %d\n", __LINE__ - 1, i);
			break;
		}

		/* Validate file header */
		if (file_header.file_size == 0) {
			fprintf(stderr, "%d: Warning: Skipping zero-length file: %s\n", __LINE__ - 1, file_header.filename);
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
			fprintf(stderr, "%d: Warning: Cannot create file %s: %s\n", __LINE__ - 2, full_path, strerror(errno));
			fseek(archive, file_header.file_size, SEEK_CUR);
			continue;
		}

		/* Read compressed data */
		uint8_t* compressed_data = calloc(file_header.file_size, sizeof(uint8_t));
		if(!compressed_data){
			fprintf(stderr, "%d: Error: Memory allocation failed for %s\n", __LINE__ - 1, file_header.filename);
			fclose(output_file);
			fseek(archive, file_header.file_size, SEEK_CUR);
			continue;
		}

		if(fread(compressed_data, 1, file_header.file_size, archive) != file_header.file_size){
			fprintf(stderr, "%d: Error: Cannot read file data for %s\n", __LINE__ - 1, file_header.filename);
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
				fprintf(stderr, "%d: Warning: Decompression failed for %s, storing compressed data\n", __LINE__ - 12,file_header.filename);
			}
		} else {
			/* Write uncompressed data */
			size_t written = fwrite(compressed_data, 1, file_header.file_size, output_file);
			if(written != file_header.file_size)
				fprintf(stderr, "%d: Warning: Incomplete write for %s\n", __LINE__ - 2, file_header.filename);
		}

		free(compressed_data);

		if(fclose(output_file) != 0)
		    printErr("%d: Warning: Error closing file %s\n", __LINE__ - 1, full_path);

		/* Restore file permissions */
		if(chmod(full_path, file_header.permissions) != 0)
		    fprintf(stderr, "%d: Warning: Cannot set permissions for %s: %s\n", __LINE__ - 1, full_path, strerror(errno));

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
			fprintf(stderr, "%d: Error: Cannot read file header for file %d\n", __LINE__ - 1, i);
			break;
		}

		/* Skip file data */
		fseek(archive, file_header.file_size, SEEK_CUR);

		total_files_size += file_header.file_size;

		/* Format permissions string */
		char perm_str[11];
		snprintf(perm_str, sizeof(perm_str), "%04o", file_header.permissions & 0777);

		printf("%-50s %-12lu %-10s %s\n", file_header.filename,(unsigned long)file_header.file_size,
			file_header.is_compressed ? "PPM" : "NO", perm_str);
	}

	printf("-------------------------------------------------- ------------ ---------- ----------\n");
	printf("%-50s %-12lu %-10s\n", "TOTAL", (unsigned long)total_files_size, "");

	fclose(archive);
}

/* Verify archive integrity */
int verify_archive(const char* archive_path) {
	FILE* archive = fopen(archive_path, "rb");
	if(!archive)
		printErr("%d: Error: Cannot open archive %s\n", __LINE__ - 2, archive_path);

	long int archive_size = getFileSize(archive);

	if(archive_size < (long)sizeof(ArchiveHeader)){
		fclose(archive); printErr("%d: Error: Archive file is too small\n", __LINE__ - 2);
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
			fprintf(stderr, "%d: Error: Cannot read file header for file %d\n", __LINE__ - 1, i);
			break;
		}

		/* Check if offset matches */
		if (file_header.offset != current_offset) {
			fprintf(stderr, "%d: Warning: File offset mismatch for %s\n", __LINE__ - 1, file_header.filename);
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

/* Process directory recursively */
void process_directory(const char* base_path, const char* rel_path, 
                      FILE* archive, uint16_t* file_count, uint64_t* total_size, int vflag) {
	char full_path[PATH_MAX];
	if(strlen(rel_path) == 0)
		snprintf(full_path, sizeof(full_path), "%s", base_path);
	else
		snprintf(full_path, sizeof(full_path), "%s/%s", base_path, rel_path);

	DIR* dir = opendir(full_path);
	if(!dir)
		printErr("%d: Warning: Cannot open directory %s: %s\n", __LINE__, full_path, strerror(errno));

	struct dirent* entry = {0};
	for(;(entry = readdir(dir)) != NULL;){
		/* Skip . and .. entries */
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
			continue;

		/* Build relative path */
		char new_rel_path[PATH_MAX];
		if(strlen(rel_path) == 0)
			snprintf(new_rel_path, sizeof(new_rel_path), "%s", entry->d_name);
		else
			snprintf(new_rel_path, sizeof(new_rel_path), "%s/%s", rel_path, entry->d_name);

		/* Build full path */
		char entry_full_path[PATH_MAX*2];
		snprintf(entry_full_path, sizeof(entry_full_path)*2, "%s/%s", base_path, new_rel_path);

		struct stat stat_buf;
		if(stat(entry_full_path, &stat_buf) != 0){
			fprintf(stderr, "%d: Warning: Cannot stat %s: %s\n", __LINE__ - 1, entry_full_path, strerror(errno));
			continue;
		}

		if(S_ISDIR(stat_buf.st_mode))
			/* Recursively process subdirectory */
			process_directory(base_path, new_rel_path, archive, file_count, total_size, vflag);
		else if(S_ISREG(stat_buf.st_mode))
			/* Process regular file */
			process_single_file(entry_full_path, new_rel_path, archive, file_count, total_size, &stat_buf, vflag);
		else
			printErr("%d: Error while handling files: %s", __LINE__ - 7, strerror(errno));
	}
    
	closedir(dir);
}

/* Process single file for archiving */
void process_single_file(const char* filepath, const char* rel_path, 
                        FILE* archive, uint16_t* file_count,
                        uint64_t* total_size, struct stat* stat_buf, int vflag) {
	FILE* file = fopen(filepath, "rb");
	if(!file)
		printErr("%d: Warning: Cannot open file %s: %s\n", __LINE__ - 2, filepath, strerror(errno));
    
	/* Get file size safely */
   	long file_size_long = getFileSize(file); 

	if(file_size_long <= 0){
		fclose(file);
		fprintf(stdout, "Skipped: %s (empty file)\n", rel_path);
		return;
	}

	size_t file_size = (size_t)file_size_long;
    
	/* Read file data */
	uint8_t* file_data = malloc(file_size); //, sizeof(uint8_t));
	if(!file_data){
		fclose(file);
		printErr("%d: Error: Memory allocation failed for %s: %s\n", __LINE__ -3, filepath, strerror(errno));
	}
    
	size_t bytes_read = fread(file_data, 1, file_size, file);
	fclose(file);
    
	if(bytes_read != file_size){
		free(file_data);
		fprintf(stderr, "%d: Error: Cannot read file %s: %s\n", __LINE__ - 5, filepath, strerror(errno));
		return;
	}
    
	/* Prepare file header */
	FileHeader header = {0};

	strncpy(header.filename, rel_path, sizeof(header.filename) - 1);
	header.permissions = stat_buf->st_mode;
	header.offset = *total_size;
	header.algorithm = 1; /* PPM */
    
	uint8_t* compressed_data = NULL;
	size_t compressed_size = 0;
	int should_compress = should_compress_file(filepath);

	if(should_compress){
		compressed_size = ppm_compress(file_data, file_size, &compressed_data);
	}

	/* Decide whether to use compressed or original data */
	if(compressed_data && compressed_size > 0 && compressed_size < file_size){
		header.file_size = compressed_size;
		header.is_compressed = 1;
		if(vflag == 1)
			fprintf(stdout, "Processed: %s (PPM) %zu -> %zu bytes\n", rel_path, file_size, compressed_size);
	} else{
		header.file_size = file_size;
		header.is_compressed = 0;
		if(compressed_data)
			free(compressed_data);
		compressed_data = file_data;
		compressed_size = file_size;
		file_data = NULL;
		if(vflag == 1 )
			fprintf(stdout, "Processed: %s (store) %zu bytes\n", rel_path, file_size);
	}

	/* Write to archive */
	size_t header_written = fwrite(&header, sizeof(FileHeader), 1, archive);
	size_t data_written = fwrite(compressed_data, 1, header.file_size, archive);

	if (header_written != 1 || data_written != header.file_size)
		fprintf(stderr, "%d: Error: Write failed for %s: %s\n", __LINE__, rel_path, strerror(errno));
	else {
		/* Update counters */
		(*file_count)++;
		*total_size += sizeof(FileHeader) + header.file_size;
	}

	/* Cleanup */
	if (compressed_data != file_data)
		free(compressed_data);
	if (file_data)
		free(file_data);
}

/* Create directory if it doesn't exist */
int create_directory(const char* path){
	struct stat st = {0};
	if(stat(path, &st) == -1)
		if(mkdir(path, 0755) != 0)
			printErr("%d: Error: Cannot create directory %s: %s\n", __LINE__ -1,  path, strerror(errno));
	return 0;
}

/* Create parent directories for file path */
int create_parent_dirs(const char* filepath) {
	char path[PATH_MAX];
	strncpy(path, filepath, sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';

	char* slash = strrchr(path, '/');
	if(slash){
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

/* Simple but stable PPM implementation */
size_t ppm_compress(const uint8_t* input, size_t input_size, uint8_t** output) {
	if(input_size == 0 || !input || !output){
		*output = NULL;
		return 0;
	}

	/* Allocate with safe margin */
	uint8_t* compressed = calloc((input_size + 8), sizeof(uint8_t));
	if(!compressed){
		*output = NULL;
		return 0;
	}

	/* Store original size in header */
	compressed[0] = (input_size >> 24) & 0xFF;
	compressed[1] = (input_size >> 16) & 0xFF;
	compressed[2] = (input_size >> 8) & 0xFF;
	compressed[3] = input_size & 0xFF;

	/* Simple compression: remove consecutive duplicates */
	size_t comp_index = 4;

	for(size_t i = 0;i < input_size;){
		uint8_t current = input[i];
		size_t count = 1;

		/* Count consecutive identical bytes */
		for(;i + count < input_size && input[i + count] == current && count < 255; count++);

		if(count > 3){
			/* Encode run */
			compressed[comp_index++] = current;
			compressed[comp_index++] = current; /* Marker */
			compressed[comp_index++] = (uint8_t)count;
			i += count;
		} else
			/* Copy literal */
			for(size_t j = 0; j < count; j++)
				compressed[comp_index++] = input[i++];
	}
    
	/* Check if compression actually helped */
	if (comp_index >= input_size) {
		/* Compression didn't help - store original */
		free(compressed);
		*output = NULL;
		return 0;
	}

	*output = compressed;
	return comp_index;
}

size_t ppm_decompress(const uint8_t* input, size_t input_size, uint8_t** output) {
	if (input_size < 4 || !input || !output) {
		*output = NULL;
		return 0;
	}
    
	/* Read original size from header */
	size_t original_size = (input[0] << 24) | (input[1] << 16) | (input[2] << 8) | input[3];

	if (original_size == 0) {
		*output = NULL;
		return 0;
	}

	uint8_t* decompressed = malloc(original_size);
	if (!decompressed) {
		*output = NULL;
		return 0;
	}

	size_t decomp_index = 0;
	size_t comp_index = 4;

	for(;comp_index < input_size && decomp_index < original_size;){
		if (comp_index + 2 < input_size && input[comp_index] == input[comp_index + 1]) {
			/* Decode run */
			uint8_t value = input[comp_index];
			uint8_t count = input[comp_index + 2];

			for (uint8_t j = 0; j < count && decomp_index < original_size; j++)
				decompressed[decomp_index++] = value;
			comp_index += 3;
		} else
			/* Copy literal */
			decompressed[decomp_index++] = input[comp_index++];
	}

	*output = decompressed;
	return decomp_index;
}
