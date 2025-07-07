#pragma once

#include "config.h"
#include "limits.h"

extern int attach_console;
extern int alloc_console;

extern const char *uv_dir;
extern const char *cache_dir;
extern const char *dot_python_version_path;
extern const char *python_dir;
extern const char *venv_path;
extern const char *pyvenv_cfg_path;
extern const char *src_dir;
extern const char *pyproject_toml_path;
extern const char *requirements_txt_path;
extern const char *uv_lock_path;

extern char uv_path[PATH_MAX];
extern char python_path[PATH_MAX];

extern char config_unzip_path[PATH_MAX];
extern char config_uv_install_script_windows[PATH_MAX];
extern char config_uv_install_script_unix[PATH_MAX];
extern char config_entry[PATH_MAX];
extern int config_win_gui;

void exit_with_message(const char *format, ...);
void console_log(const char *format, ...);
void close_console();
int path_exists(const char *filename);
void find_python_path();
void init();
void copy_file(const char *src_path, const char *dst_path);
void mkdir_recursive(const char *path);
void copy_directory(const char *src_dir, const char *dst_dir);
void set_env(const char *key, const char *value);
void unzip();
void install_uv();
void install_python();
void uv_init();
void uv_add_dependencies();
void uv_sync(int frozen, int python);
void uv_run(int gui, int argc, char *argv[]);
