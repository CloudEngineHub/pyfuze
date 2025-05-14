#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <spawn.h>
#include <libc/dce.h>
#include <libc/nt/runtime.h>
#include <libc/nt/process.h>
#include <libc/nt/console.h>
#include <libc/nt/synchronization.h>
#include <libc/nt/enum/processcreationflags.h>
#include <libc/x/x.h>

extern char **environ;

char cwd[PATH_MAX] = {0};
char *project_name = NULL;
char script_name[PATH_MAX] = {0};

// NOTE: Cosmopolitan linker script is hard-coded to change the
// subsystem from TUI to GUI when GetMessage() is defined.
void GetMessage() {}

void chdir_to_executable_folder(char *argv[]);
int path_exists(const char *filename);
const char *find_python_folder_name();

void windows_attach_console(int alloc_console);
void run_command_windows(char *cmdline, int win_gui);
int main_windows(int argc, char *argv[]);

void run_command_unix(char *const argv[]);
int main_unix(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    chdir_to_executable_folder(argv);
    getcwd(cwd, sizeof(cwd));
    project_name = strrchr(cwd, '/') + 1;
    snprintf(script_name, sizeof(script_name), "%s.py", project_name);

    if (IsWindows()) {
        return main_windows(argc, argv);
    } else {
        return main_unix(argc, argv);
    }
}

void chdir_to_executable_folder(char *argv[]) {
    char *executable_path = strdup(argv[0]);
    char *last_slash = strrchr(executable_path, '/');
    *last_slash = '\0';
    chdir(executable_path);
}

int path_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

const char* find_python_folder_name() {
    static char folder_name[PATH_MAX] = {0};
    if (path_exists("python")) {
        DIR *d = opendir("python");
        struct dirent *ent;
        if (d) {
            while ((ent = readdir(d))) {
                if (ent->d_name[0] == '.') continue;
                strcpy(folder_name, ent->d_name);
                break;
            }
            closedir(d);
        }
    } else {
        return NULL;
    }
    return folder_name;
}

void windows_attach_console(int alloc_console) {
    if (!AttachConsole(kNtAttachParentProcess)) {
        if (alloc_console) AllocConsole();
    }
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$",  "r", stdin);
    freopen("CONOUT$", "w", stderr);
}

void _run_command_windows(char16_t *cmdline, int win_gui) {
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
    _run_command_windows(cmdline_utf16, win_gui);
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
    snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe add --requirements requirements.txt --python %s --quiet", python_path);
    run_command_windows(cmdline, win_gui);

    if (win_gui) {
        snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe run %s --python %s --gui-script", script_name, python_path);
    } else {
        snprintf(cmdline, sizeof(cmdline), ".\\uv\\uv.exe run %s --python %s", script_name, python_path);
    }
    run_command_windows(cmdline, win_gui);

    return 0;
}

void run_command_unix(char *const argv[]) {
    pid_t pid;
    int status;
    if (posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ) != 0) {
        printf("posix_spawnp at %s failed\n", argv[0]);
        exit(1);
    }

    if (waitpid(pid, &status, 0) == -1) {
        printf("waitpid at %s failed\n", argv[0]);
        exit(1);
    }

    if (!WIFEXITED(status)) {
        printf("command %s did not exit\n", argv[0]);
        exit(1);
    }
}

int main_unix(int argc, char *argv[]) {
    char *uv_binary = "./uv/uv";

    if (!path_exists(uv_binary)) {
        printf("%s not found, installing...\n", uv_binary);
        setenv("UV_UNMANAGED_INSTALL", "uv", 1);
        char *sh_argv[] = {
            "sh", "-c", "curl -LsSf https://astral.sh/uv/install.sh | sh",
            NULL
        };
        run_command_unix(sh_argv);
    }

    if (!path_exists("pyproject.toml")) {
        printf("initializing new project with uv...\n");
        char *init_argv[] = {uv_binary, "init", "--bare", "--no-workspace", NULL};
        run_command_unix(init_argv);
    }

    const char *python_folder_name = find_python_folder_name();
    if (!python_folder_name) {
        printf("python not found, installing to ./python ...\n");
        char *install_argv[] = {uv_binary, "python", "install", "--install-dir", "python", NULL};
        run_command_unix(install_argv);
    }
    python_folder_name = find_python_folder_name();
    if (!python_folder_name) {
        printf("ERROR: python installation failed\n");
        exit(1);
    }
    char python_path[PATH_MAX];
    snprintf(python_path, sizeof(python_path), "./python/%s", python_folder_name);

    char *add_argv[] = {
        uv_binary, "add", "--requirements", "requirements.txt", "--python", python_path, "--quiet", NULL
    };
    run_command_unix(add_argv);

    char *run_argv[] = {
        uv_binary, "run", script_name, "--python", python_path, NULL
    };
    run_command_unix(run_argv);

    return 0;
}
