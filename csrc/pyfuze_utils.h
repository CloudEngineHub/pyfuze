#pragma once

#include "limits.h"

#include "pyfuze_config.h"

extern char python_folder_name[PATH_MAX];

extern int config_win_gui;
extern char config_entry[PATH_MAX];

extern Config* config;

void chdir_to_executable_folder(char *argv[]);
int path_exists(const char *filename);
void find_python_folder_name();
void read_config();
