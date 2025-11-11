#include "main.h"

/* Main function */
int main(int argc, char* argv[]) {
	/* If no arguments, show help */
	if(strcmp(argv[1], "--help") == 0)
		print_usage(argv[0]);
	if(strcmp(argv[1], "--version") == 0)
		print_version();
	if(argc <= 2)
		printErr("Usage: zov <flags> <argument> ...\n");

	int state = 0,/* fflag = 0, */
	    vflag = 0;

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
				/*
			case 'f':
				 force flag 
				fflag = 1;
				break;
				*/
			case 'l':
				/* list flag */
				state = 3;
				break;
			case 'v':
				/* verbose flag */
				vflag = 1;
				break;
			case 'V':
				/* Version flag */
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
				printrr("%d: Error: Missing arguments for extract command\n \
				Usage: %s x <archive>\n", __LINE__, argv[0]);

			if(vflag == 1)
				printf("Extracting archive: %s to directory %s\n", archive, directory);
			
			if(extract_archive(archive, directory, NULL, vflag) != 0)
				printrr("%d: Error: Failed to extract archive\n", __LINE__);
			
			if(vflag == 1)
				fprintf(stdout, "Archive extracted successfully!\n");
			break;
		case 2:
			if(argc < 4)
				printrr("%d: Error: Missing arguments for create command\n \
					Usage: %s c <directory> <archive>\n", __LINE__, argv[0]);
			
			if(vflag == 1)
				fprintf(stdout, "Creating archive '%s' from directory '%s'\n  \
					Using PPM compression algorithm...\n", archive, directory);
			
			if(create_archive(directory, archive, NULL, vflag) != 0)
				printrr("%d: Error: Failed to create archive\n", __LINE__ - 1);
			
			if(vflag == 1)
				fprintf(stdout, "Archive created successfully!\n");
			break;
		case 3:
			if (argc < 3)
				printrr("%d: Error: Missing archive file for list command\n \
				Usage: %s l <archive>\n", __LINE__, argv[0]);
		
			list_archive_contents(argv[2]);
			break;
        
		case 4:
			if(argc < 3)
				printrr("%d: Error: Missing archive file for verify command\n \
				Usage: %s e <archive>\n", __LINE__, argv[0]);
		
			if(verify_archive(argv[2]) != 0)
				printrr("%d: Archive verification failed!\n", __LINE__);
			break;
        
		case 5:
			if(argc < 3)
				printrr("%dError: Missing archive file for info command\n \
				Usage: %s i <archive>\n", argv[0]);
			
			show_archive_info(argv[2]);
			break;
		
		default:
			printErr("Error: Unknown command \'%s\'");
			break;
	}
    return 0;
}
