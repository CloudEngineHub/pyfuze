/*
This script will be compiled using cosmocc

License from the https://github.com/jart/cosmopolitan project:

ISC License

Copyright 2020 Justine Alexandra Roberts Tunney

Permission to use, copy, modify, and/or distribute this software for
any purpose with or without fee is hereby granted, provided that the
above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#include "stdlib.h"

#include "utils.h"

// NOTE: Cosmopolitan linker script is hard-coded to change the
// subsystem from TUI to GUI when GetMessage() is defined.
void GetMessage() {}

int main(int argc, char *argv[]) {
    read_config();

    mkdir_recursive(config_unzip_path);
    chdir(config_unzip_path);

    set_env("UV_CACHE_DIR", cache_dir);
    set_env("UV_UNMANAGED_INSTALL", uv_dir);
    set_config_env();

    char cmdline[8192];

    // install uv
    if (!path_exists(uv_path)) {
        if (path_exists("/zip/uv")) {
            console_log("found /zip/uv, copying to %s ...\n", uv_dir);
            copy_directory("/zip/uv", uv_dir);
        } else {
            console_log("uv not found, installing to %s ...\n", uv_dir);
            install_uv();
        }

        if (!path_exists(uv_path)) {
            console_log("ERROR: uv installation failed\n");
            exit(1);
        }
    }

    // install python
    find_python_path();
    if (python_path[0] == '\0') {
        if (path_exists("/zip/python")) {
            console_log("found /zip/python, copying to %s ...\n", python_dir);
            copy_directory("/zip/python", python_dir);
        } else {
            console_log("python not found, installing to %s ...\n", python_dir);
            install_python();
        }

        find_python_path();
        if (python_path[0] == '\0') {
            console_log("ERROR: python installation failed\n");
            exit(1);
        }
    }

    // make sure pyproject.toml exists with dependencies
    if (!path_exists(pyproject_toml_path)) {
        if (path_exists("/zip/pyproject.toml")) {
            console_log("found /zip/pyproject.toml, copying to %s ...\n", config_unzip_path);
            copy_file("/zip/pyproject.toml", pyproject_toml_path);
        } else {
            console_log("pyproject.toml not found, creating new project...\n");
            uv_init();
        }

        if (!path_exists(requirements_txt_path) && path_exists("/zip/requirements.txt")) {
            console_log("found /zip/requirements.txt, copying to %s ...\n", requirements_txt_path);
            copy_file("/zip/requirements.txt", requirements_txt_path);
        }

        if (path_exists(requirements_txt_path)) {
            console_log("add dependencies from requirements.txt...\n");
            uv_add_dependencies();
        }
    }

    // uv sync
    uv_sync(path_exists(uv_lock_path), !path_exists(pyvenv_cfg_path));

    // close allocated console
    if (alloc_console) close_console();

    // uv run
    uv_run(config_win_gui);

    return 0;
}
