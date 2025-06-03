#include "pyfuze_windows.h"

#include "stdio.h"
#include "stdlib.h"

#include "libc/nt/console.h"
#include "libc/nt/process.h"
#include "libc/nt/runtime.h"
#include "libc/nt/synchronization.h"
#include "libc/nt/enum/processcreationflags.h"
#include "libc/x/x.h"

#include "pyfuze_utils.h"

int attach_console = 0;

void windows_attach_console() {
    if (!attach_console) {
        attach_console = 1;
        if (!AttachConsole(kNtAttachParentProcess)) {
            AllocConsole();
        }
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);
    }
}

void run_command_windows_utf16(char16_t *cmdline) {
    struct NtStartupInfo si = {0};
    si.cb = sizeof(si);
    struct NtProcessInformation pi = {0};

    uint32_t creation_flags = config_win_gui ? kNtCreateNoWindow : 0;

    if (!CreateProcess(
        NULL,  // No module name (use command line)
        cmdline,  // Command line
        NULL, // Process handle not inheritable
        NULL, // Thread handle not inheritable
        0, // Set handle inheritance to FALSE
        creation_flags,  // creation flags
        NULL, // Use parent's environment block
        NULL, // Use parent's starting directory
        &si, // Pointer to STARTUPINFO structure
        &pi // Pointer to PROCESS_INFORMATION structure
    )) {
        printf("CreateProcess failed: %lu\n", GetLastError());
        exit(1);
    }

    WaitForSingleObject(pi.hProcess, 0xFFFFFFFF);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void run_command_windows(char *cmdline) {
    char16_t *cmdline_utf16 = utf8to16(cmdline, -1, 0);
    run_command_windows_utf16(cmdline_utf16);
    free(cmdline_utf16);
}

void set_config_env_windows() {
    for (size_t i = 0; i < config->count; i++) {
        char* key = config->items[i].key;
        char* value = config->items[i].value;
        if (strncmp(key, "env_", 4) != 0) {
            continue;
        }
        // remove the env_ prefix
        key = key + 4;
        char16_t *key_utf16 = utf8to16(key, -1, 0);
        char16_t *value_utf16 = utf8to16(value, -1, 0);
        SetEnvironmentVariable(key_utf16, value_utf16);
        free(key_utf16);
        free(value_utf16);
    }
}

int main_windows(int argc, char *argv[]) {
    if (!config_win_gui) {
        windows_attach_console();
    }
    SetEnvironmentVariable(u"UV_UNMANAGED_INSTALL", u"uv");
    SetEnvironmentVariable(u"PSModulePath", u"");
    SetEnvironmentVariable(u"PSMODULEPATH", u"");
    set_config_env_windows();

    char cmdline[8192];

    // install uv
    if (!path_exists(".\\uv\\uv.exe")) {
        windows_attach_console();
        printf(".\\uv\\uv.exe not found, installing...\n");
        if (path_exists(config_uv_install_script_windows)) {
            snprintf(cmdline, sizeof(cmdline), "\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoProfile -ExecutionPolicy Bypass -File \"%s\"", config_uv_install_script_windows);
        } else {
            snprintf(cmdline, sizeof(cmdline), "\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoProfile -ExecutionPolicy Bypass -c \"irm %s | iex\"", config_uv_install_script_windows);
        }
        run_command_windows(cmdline);
        if (!path_exists(".\\uv\\uv.exe")) {
            printf("ERROR: uv installation failed\n");
            exit(1);
        }
    }

    // install python
    find_python_folder_name();
    if (python_folder_name[0] == '\0') {
        windows_attach_console();
        printf("python not found, installing to .\\python ...\n");
        run_command_windows(".\\uv\\uv.exe python install --install-dir python");
        find_python_folder_name();
        if (python_folder_name[0] == '\0') {
            printf("ERROR: python installation failed\n");
            exit(1);
        }
    }
    char python_path[PATH_MAX];
    snprintf(python_path, sizeof(python_path), ".\\python\\%s", python_folder_name);

    // make sure pyproject.toml exists with dependencies
    if (!path_exists("pyproject.toml")) {
        windows_attach_console();
        printf("initializing new project with uv...\n");
        run_command_windows(".\\uv\\uv.exe init --bare --no-workspace");
        if (path_exists("requirements.txt")) {
            printf("add dependencies from requirements.txt...\n");
            snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe add -r requirements.txt --python %s", python_path);
            run_command_windows(cmdline);
        }
    }

    // uv sync
    if (path_exists("uv.lock")) {
        if (path_exists(".venv\\pyvenv.cfg")) {
            snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe sync --frozen --quiet");
        } else {
            snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe sync --frozen --quiet --python %s", python_path);
        }
    } else {
        if (path_exists(".venv\\pyvenv.cfg")) {
            snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe sync --quiet", python_path);
        } else {
            snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe sync --quiet --python %s", python_path);
        }
    }
    run_command_windows(cmdline);

    // uv run
    if (config_win_gui) {
        // close console
        if (attach_console) {
            fclose(stdout);
            fclose(stderr);
            fclose(stdin);
            FreeConsole();
        }
        snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe run --project . --directory src --gui-script %s", config_entry);
    } else {
        snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe run --project . --directory src --script %s", config_entry);
    }
    run_command_windows(cmdline);

    return 0;
}
