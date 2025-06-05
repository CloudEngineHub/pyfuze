#pragma once

#include "limits.h"

#include "pyfuze_config.h"

extern char python_folder_name[PATH_MAX];

extern char config_uv_install_script_windows[PATH_MAX];
extern char config_uv_install_script_unix[PATH_MAX];
extern char config_entry[PATH_MAX];
extern int config_win_gui;

extern Config *config;

void chdir_to_executable_folder(char *argv[]);
int path_exists(const char *filename);
void find_python_folder_name();
void read_config();
void copy_file(const char *src_path, const char *dst_path);
void copy_directory(const char *src_dir, const char *dst_dir);
