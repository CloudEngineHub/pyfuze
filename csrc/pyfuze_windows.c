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

void windows_attach_console(int alloc_console) {
    if (!AttachConsole(kNtAttachParentProcess)) {
        if (alloc_console) AllocConsole();
    }
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$",  "r", stdin);
    freopen("CONOUT$", "w", stderr);
}

void run_command_windows_utf16(char16_t *cmdline, int win_gui) {
    struct NtStartupInfo si = {0};
    si.cb = sizeof(si);
    struct NtProcessInformation pi = {0};

    uint32_t creation_flags = win_gui ? kNtCreateNoWindow : 0;

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

void run_command_windows(char *cmdline, int win_gui) {
    char16_t *cmdline_utf16 = utf8to16(cmdline, -1, 0);
    run_command_windows_utf16(cmdline_utf16, win_gui);
    free(cmdline_utf16);
}

int main_windows(int argc, char *argv[]) {
    int win_gui = path_exists("./WIN_GUI");
    int alloc_console = !win_gui;
    windows_attach_console(alloc_console);

    if (!path_exists(".\\uv\\uv.exe")) {
        printf(".\\uv\\uv.exe not found, installing...\n");
        SetEnvironmentVariable(u"UV_UNMANAGED_INSTALL", u"uv");
        SetEnvironmentVariable(u"PSModulePath", u"");
        SetEnvironmentVariable(u"PSMODULEPATH", u"");
        run_command_windows("\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoProfile -ExecutionPolicy Bypass -c \"irm https://astral.sh/uv/install.ps1 | iex\"", win_gui);
    }

    if (!path_exists("pyproject.toml")) {
        printf("initializing new project with uv...\n");
        run_command_windows(".\\uv\\uv.exe init --bare --no-workspace", win_gui);
    }

    const char *python_folder_name = find_python_folder_name();
    if (!python_folder_name) {
        printf("python not found, installing to .\\python ...\n");
        run_command_windows(".\\uv\\uv.exe python install --install-dir python", win_gui);
    }
    python_folder_name = find_python_folder_name();
    if (!python_folder_name) {
        printf("ERROR: python installation failed\n");
        exit(1);
    }
    char python_path[PATH_MAX];
    snprintf(python_path, sizeof(python_path), ".\\python\\%s", python_folder_name);

    char cmdline[8192];

    if (path_exists("requirements.txt")) {
        snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe pip install -r requirements.txt --python %s --quiet", python_path);
        run_command_windows(cmdline, win_gui);
    }

    if (win_gui) {
        snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe run %s --python %s --gui-script", script_name, python_path);
    } else {
        snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe run %s --python %s", script_name, python_path);
    }
    run_command_windows(cmdline, win_gui);

    return 0;
}
