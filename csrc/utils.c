#include "utils.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "dirent.h"
#include "limits.h"
#include "stdarg.h"
#include "spawn.h"
#include "sys/stat.h"
#include "windowsesque.h"

#include "libc/dce.h"
#include "libc/x/x.h"
#include "libc/nt/console.h"
#include "libc/nt/process.h"
#include "libc/nt/runtime.h"
#include "libc/nt/synchronization.h"
#include "libc/nt/enum/processcreationflags.h"
#include "libc/errno.h"

#include "config.h"

int attach_console = 0;
int alloc_console = 0;

const char *uv_dir = "uv";
const char *cache_dir = "cache";
const char *dot_python_version_path = ".python-version";
const char *python_dir = "python";
const char *venv_path = ".venv";
const char *pyvenv_cfg_path = ".venv/pyvenv.cfg";
const char *src_dir = "src";
const char *pyproject_toml_path = "pyproject.toml";
const char *requirements_txt_path = "requirements.txt";
const char *uv_lock_path = "uv.lock";

char uv_path[PATH_MAX] = {0};
char python_path[PATH_MAX] = {0};

char config_unzip_path[PATH_MAX] = {0};
char config_uv_install_script_windows[PATH_MAX] = {0};
char config_uv_install_script_unix[PATH_MAX] = {0};
char config_entry[PATH_MAX] = {0};
int config_win_gui = 0;

char cmdline[8192];

void windows_attach_or_alloc_console() {
    if (IsWindows() && config_win_gui && !attach_console) {
        attach_console = 1;
        if (!AttachConsole(kNtAttachParentProcess)) {
            alloc_console = 1;
            AllocConsole();
        }
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
}

void console_log(const char *format, ...) {
    windows_attach_or_alloc_console();
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void close_console() {
    attach_console = 0;
    alloc_console = 0;
    fclose(stdout);
    fclose(stderr);
    FreeConsole();
}

void exit_with_message(const char *format, ...) {
    config_win_gui = 1;
    windows_attach_or_alloc_console();
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\nPress Enter to exit...\n");
    freopen("CONIN$", "r", stdin);
    getchar();
    exit(1);
}

int path_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

void path_join(char *result, size_t result_size, const char *p1, const char *p2) {
    if (p1[strlen(p1) - 1] == '/') {
        snprintf(result, result_size, "%s%s", p1, p2);
    } else {
        snprintf(result, result_size, "%s/%s", p1, p2);
    }
}

void find_python_path() {
    if (path_exists(python_dir)) {
        DIR *d = opendir(python_dir);
        struct dirent *ent;
        if (d) {
            while ((ent = readdir(d))) {
                if (ent->d_name[0] == '.') continue;
                path_join(python_path, sizeof(python_path), python_dir, ent->d_name);
                break;
            }
            closedir(d);
        }
    }
}

Config *read_config() {
    Config *config = parse_config("/zip/config.txt");
    if (!config) {
        exit_with_message("Failed to parse config.txt");
    }

    strcpy(config_unzip_path, get_config_value(config, "unzip_path"));
    strcpy(config_uv_install_script_windows, get_config_value(config, "uv_install_script_windows"));
    strcpy(config_uv_install_script_unix, get_config_value(config, "uv_install_script_unix"));
    strcpy(config_entry, get_config_value(config, "entry"));
    config_win_gui = atoi(get_config_value(config, "win_gui"));

    return config;
}

void init() {
    Config *config = read_config();

    path_join(uv_path, sizeof(uv_path), uv_dir, IsWindows() ? "uv.exe" : "uv");
    mkdir_recursive(config_unzip_path);
    chdir(config_unzip_path);

    // set environment variables
    set_env("UV_CACHE_DIR", cache_dir);
    set_env("UV_UNMANAGED_INSTALL", uv_dir);

    for (size_t i = 0; i < config->count; i++) {
        char *key = config->items[i].key;
        char *value = config->items[i].value;
        if (strncmp(key, "env_", 4) != 0) {
            continue;
        }
        key = key + 4;
        set_env(key, value);
    }

    free_config(config);
}

void copy_file(const char *src_path, const char *dst_path) {
    struct stat st;
    int src_fd, dst_fd;

    if ((src_fd = open(src_path, O_RDONLY)) == -1) exit_with_message("Failed to open %s", src_path);
    fstat(src_fd, &st);

    if ((dst_fd = creat(dst_path, st.st_mode)) == -1) {
        close(src_fd);
        exit_with_message("Failed to create %s", dst_path);
    }

    ssize_t result = copyfd(src_fd, dst_fd, -1);
    close(src_fd);
    close(dst_fd);

    if (result == -1) exit_with_message("Failed to copy %s to %s", src_path, dst_path);
}

void mkdir_recursive(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0) {
                if (errno != EEXIST) exit_with_message("mkdir %s failed", tmp);
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0) {
        if (errno != EEXIST) exit_with_message("mkdir %s failed", tmp);
    }
}

void copy_directory(const char *src_dir, const char *dst_dir) {
    struct stat st;
    if (stat(src_dir, &st) != 0) exit_with_message("stat %s failed", src_dir);

    if (!path_exists(dst_dir)) {
        mkdir_recursive(dst_dir);
    }

    DIR *dir = opendir(src_dir);
    if (!dir) exit_with_message("opendir %s failed", src_dir);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX];
        char dst_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, entry->d_name);

        if (stat(src_path, &st) != 0) {
            closedir(dir);
            exit_with_message("stat %s failed", src_path);
        }

        if (S_ISDIR(st.st_mode)) {
            copy_directory(src_path, dst_path);
        } else if (S_ISREG(st.st_mode)) {
            copy_file(src_path, dst_path);
        }
    }

    closedir(dir);
}

void set_env(const char *key, const char *value) {
    if (IsWindows()) {
        char16_t *key_utf16 = utf8to16(key, -1, 0);
        char16_t *value_utf16 = utf8to16(value, -1, 0);
        SetEnvironmentVariable(key_utf16, value_utf16);
        free(key_utf16);
        free(value_utf16);
    } else {
        setenv(key, value, 1);
    }
}

void unzip() {
    if (!path_exists(dot_python_version_path) && path_exists("/zip/.python-version")) {
        console_log("found /zip/.python-version, copying to %s ...\n", dot_python_version_path);
        copy_file("/zip/.python-version", dot_python_version_path);
    }
    if (!path_exists(uv_dir) && path_exists("/zip/uv")) {
        console_log("found /zip/uv, copying to %s ...\n", uv_dir);
        copy_directory("/zip/uv", uv_dir);
    }
    if (!path_exists(python_dir) && path_exists("/zip/python")) {
        console_log("found /zip/python, copying to %s ...\n", python_dir);
    }
    if (!path_exists(pyproject_toml_path) && path_exists("/zip/pyproject.toml")) {
        console_log("found /zip/pyproject.toml, copying to %s ...\n", pyproject_toml_path);
        copy_file("/zip/pyproject.toml", pyproject_toml_path);
    }
    if (!path_exists(requirements_txt_path) && path_exists("/zip/requirements.txt")) {
        console_log("found /zip/requirements.txt, copying to %s ...\n", requirements_txt_path);
        copy_file("/zip/requirements.txt", requirements_txt_path);
    }
    if (!path_exists(uv_lock_path) && path_exists("/zip/uv.lock")) {
        console_log("found /zip/uv.lock, copying to %s ...\n", uv_lock_path);
        copy_file("/zip/uv.lock", uv_lock_path);
    }
    if (!path_exists(venv_path) && path_exists("/zip/.venv")) {
        console_log("found /zip/.venv, copying to %s ...\n", venv_path);
        copy_directory("/zip/.venv", venv_path);
    }
    if (!path_exists(src_dir) && path_exists("/zip/src")) {
        console_log("found /zip/src, copying to %s ...\n", src_dir);
        copy_directory("/zip/src", src_dir);
    }
}

void run_command_windows_utf16(char16_t *cmd) {
    struct NtStartupInfo si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;

    // inherit stdout and stderr
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    // don't inherit stdin
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn && hIn != INVALID_HANDLE_VALUE) {
        SetHandleInformation(hIn, HANDLE_FLAG_INHERIT, 0);
    }

    struct NtProcessInformation pi = {0};

    uint32_t creation_flags = (config_win_gui && !attach_console) ? kNtCreateNoWindow : 0;

    if (!CreateProcess(
            NULL,            // No module name (use command line)
            cmd,             // Command line
            NULL,            // Process handle not inheritable
            NULL,            // Thread handle not inheritable
            1,               // Set handle inheritance to TRUE
            creation_flags,  // creation flags
            NULL,            // Use parent's environment block
            NULL,            // Use parent's starting directory
            &si,             // Pointer to STARTUPINFO structure
            &pi              // Pointer to PROCESS_INFORMATION structure
            )) {
        exit_with_message("CreateProcess failed: %lu", GetLastError());
    }

    WaitForSingleObject(pi.hProcess, 0xFFFFFFFF);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void run_command_windows(char *cmd) {
    char16_t *cmdline_utf16 = utf8to16(cmd, -1, 0);
    run_command_windows_utf16(cmdline_utf16);
    free(cmdline_utf16);
}

void run_command_unix(const char *const argv[]) {
    pid_t pid;
    int status;
    if (posix_spawnp(&pid, argv[0], NULL, NULL, (char *const *)argv, environ) != 0) exit_with_message("posix_spawnp at %s failed", argv[0]);
    if (waitpid(pid, &status, 0) == -1) exit_with_message("waitpid at %s failed", argv[0]);
    if (!WIFEXITED(status)) exit_with_message("command %s did not exit", argv[0]);
}

#define RUN_COMMAND_UNIX(...) \
    run_command_unix((const char *const[]){__VA_ARGS__, NULL})

void install_uv() {
    if (IsWindows()) {
        if (path_exists(config_uv_install_script_windows)) {
            snprintf(cmdline, sizeof(cmdline), "\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoProfile -ExecutionPolicy Bypass -File \"%s\"", config_uv_install_script_windows);
        } else {
            snprintf(cmdline, sizeof(cmdline), "\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoProfile -ExecutionPolicy Bypass -c \"irm %s | iex\"", config_uv_install_script_windows);
        }
        run_command_windows(cmdline);
    } else {
        if (path_exists(config_uv_install_script_unix)) {
            RUN_COMMAND_UNIX("sh", config_uv_install_script_unix);
        } else {
            snprintf(cmdline, sizeof(cmdline), "curl -LsSf %s | sh", config_uv_install_script_unix);
            RUN_COMMAND_UNIX("sh", "-c", cmdline);
        }
    }
}

void install_python() {
    if (IsWindows()) {
        snprintf(cmdline, sizeof(cmdline), "\"%s\" python install --install-dir %s", uv_path, python_dir);
        run_command_windows(cmdline);
    } else {
        RUN_COMMAND_UNIX(uv_path, "python", "install", "--install-dir", python_dir);
    }
}

void uv_init() {
    if (IsWindows()) {
        snprintf(cmdline, sizeof(cmdline), "\"%s\" init --bare --no-workspace", uv_path);
        run_command_windows(cmdline);
    } else {
        RUN_COMMAND_UNIX(uv_path, "init", "--bare", "--no-workspace");
    }
}

void uv_add_dependencies() {
    if (IsWindows()) {
        snprintf(cmdline, sizeof(cmdline), "\"%s\" add -r %s --python %s", uv_path, requirements_txt_path, python_path);
        run_command_windows(cmdline);
    } else {
        RUN_COMMAND_UNIX(uv_path, "add", "-r", requirements_txt_path, "--python", python_path);
    }
}

void uv_sync(int frozen, int python) {
    if (IsWindows()) {
        snprintf(cmdline, sizeof(cmdline), "\"%s\" sync --quiet", uv_path);
        if (frozen) {
            strcat(cmdline, " --frozen");
        }
        if (python) {
            strcat(cmdline, " --python ");
            strcat(cmdline, python_path);
        }
        run_command_windows(cmdline);
    } else {
        if (frozen) {
            if (python) {
                RUN_COMMAND_UNIX(uv_path, "sync", "--quiet", "--frozen", "--python", python_path);
            } else {
                RUN_COMMAND_UNIX(uv_path, "sync", "--quiet", "--frozen");
            }
        } else {
            if (python) {
                RUN_COMMAND_UNIX(uv_path, "sync", "--quiet", "--python", python_path);
            } else {
                RUN_COMMAND_UNIX(uv_path, "sync", "--quiet");
            }
        }
    }
}

void uv_run(int gui) {
    if (IsWindows()) {
        if (gui) {
            snprintf(cmdline, sizeof(cmdline), "\"%s\" run --project %s --directory %s --gui-script %s", uv_path, config_unzip_path, src_dir, config_entry);
        } else {
            snprintf(cmdline, sizeof(cmdline), "\"%s\" run --project %s --directory %s --script %s", uv_path, config_unzip_path, src_dir, config_entry);
        }
        run_command_windows(cmdline);
    } else {
        RUN_COMMAND_UNIX(uv_path, "run", "--project", config_unzip_path, "--directory", src_dir, "--script", config_entry);
    }
}
