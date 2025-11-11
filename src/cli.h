#ifndef CLI_H
#define CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "archive.h"

int print_usage(const char* program_name);
int print_version();
int show_archive_info(const char* archive_path);
#endif
