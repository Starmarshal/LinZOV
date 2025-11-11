#include "main.h"

/* Main function */
int main(int argc, char* argv[]) {
    /* If no arguments, show interactive mode */
	if (argc == 1) {
		show_interactive_menu();
		return 0;
	}
    
	const char* command = argv[1];

	/* Handle help and version flags */
	if (strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0)
		print_usage(argv[0]);
	else if(strcmp(command, "--version") == 0 || strcmp(command, "-v") == 0)
		print_version();
	else if(strcmp(command, "interactive") == 0 || strcmp(command, "i") == 0)
		show_interactive_menu();
    
	/* Handle archive commands */
	if(strcmp(command, "create") == 0){
		if (argc < 4)
		printrr("%d: Error: Missing arguments for create command\n \
				Usage: %s create <directory> <archive>\n", __LINE__, argv[0]);
        
        const char* directory = argv[2];
        const char* archive = argv[3];
        
        fprintf(stdout, "Creating archive '%s' from directory '%s'\n", archive, directory);
        fprintf(stdout, "Using PPM compression algorithm...\n");
        
        if(create_archive(directory, archive, NULL) != 0)
		printrr("%d: Error: Failed to create archive\n", __LINE__ - 1);
        
        fprintf(stdout, "Archive created successfully!\n");
        
	} else if(strcmp(command, "extract") == 0){
		if(argc < 4)
			printrr("%d: Error: Missing arguments for extract command\n \
			Usage: %s extract <archive> <directory>\n", __LINE__, argv[0]);
        
		const char* archive = argv[2];
		const char* directory = argv[3];
		
		printf("Extracting archive '%s' to directory '%s'\n", archive, directory);
		
		if(extract_archive(archive, directory, NULL) != 0)
			printrr("%d: Error: Failed to extract archive\n", __LINE__);
		
		fprintf(stdout, "Archive extracted successfully!\n");
        
	} else if(strcmp(command, "list") == 0){
		if (argc < 3)
			printrr("%d: Error: Missing archive file for list command\n \
			Usage: %s list <archive>\n", __LINE__, argv[0]);
        
        list_archive_contents(argv[2]);
        
	} else if(strcmp(command, "verify") == 0){
		if(argc < 3)
			printrr("%d: Error: Missing archive file for verify command\n \
			Usage: %s verify <archive>\n", __LINE__, argv[0]);
        
        if(verify_archive(argv[2]) != 0)
		printrr("%d: Archive verification failed!\n", __LINE__);
        
    } else if(strcmp(command, "info") == 0){
        if(argc < 3)
            printrr("%dError: Missing archive file for info command\n \
            Usage: %s info <archive>\n", argv[0]);
        
        show_archive_info(argv[2]);
        
    } else
        fprintf(stderr, "Error: Unknown command '%s'\n \
        Use '%s --help' for usage information.\n", command, argv[0]);
    
    return 0;
}
