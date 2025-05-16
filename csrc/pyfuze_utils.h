#pragma once

#include "limits.h"

extern char cwd[PATH_MAX];
extern char *project_name;
extern char script_name[PATH_MAX];

void chdir_to_executable_folder(char *argv[]);
int path_exists(const char *filename);
const char *find_python_folder_name();
