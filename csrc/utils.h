#pragma once

#include "limits.h"

#include "config.h"

extern int attach_console;
extern int alloc_console;

extern char uv_dir[PATH_MAX];
extern char uv_path[PATH_MAX];
extern char cache_dir[PATH_MAX];
extern char dot_python_version_path[PATH_MAX];
extern char python_dir[PATH_MAX];
extern char python_path[PATH_MAX];
extern char venv_path[PATH_MAX];
extern char pyvenv_cfg_path[PATH_MAX];
extern char src_dir[PATH_MAX];
extern char pyproject_toml_path[PATH_MAX];
extern char requirements_txt_path[PATH_MAX];
extern char uv_lock_path[PATH_MAX];

extern char config_unzip_path[PATH_MAX];
extern char config_uv_install_script_windows[PATH_MAX];
extern char config_uv_install_script_unix[PATH_MAX];
extern char config_entry[PATH_MAX];
extern int config_win_gui;

extern Config *config;

void exit_with_message(const char *format, ...);
void console_log(const char *format, ...);
void close_console();
int path_exists(const char *filename);
void find_python_path();
void read_config();
void copy_file(const char *src_path, const char *dst_path);
void copy_directory(const char *src_dir, const char *dst_dir);
void set_env(const char *key, const char *value);
void set_config_env();
void unzip();
void install_uv();
void install_python();
void uv_init();
void uv_add_dependencies();
void uv_sync(int frozen, int python);
void uv_run(int gui);
