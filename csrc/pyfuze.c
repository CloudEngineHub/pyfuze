#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

char* find_python() {
    static char python_path[PATH_MAX] = {0};
    if (access("python", F_OK) == 0) {
        DIR *d = opendir("python");
        struct dirent *ent;
        if (d) {
            while ((ent = readdir(d))) {
                if (ent->d_name[0] == '.') continue;
                snprintf(python_path, sizeof(python_path), "python/%s", ent->d_name);
                break;
            }
            closedir(d);
        }
    }
    return python_path;
}

const char* get_project_name() {
    static char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    char *p = strrchr(cwd, '/');
    return p ? p+1 : cwd;
}

int run_command(const char *prog, char *const argv[]) {
    pid_t pid;
    int status;
    if (posix_spawnp(&pid, prog, NULL, NULL, argv, environ) != 0) {
        perror("posix_spawnp failed");
        return 1;
    }
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid failed");
        return 1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 1;
}

int main() {
    struct utsname uts;
    uname(&uts);
    int is_windows = strcmp(uts.sysname, "Windows") == 0;
    int is_unix = strcmp(uts.sysname, "Linux") == 0 || strcmp(uts.sysname, "Darwin") == 0;
    if (!is_windows && !is_unix) {
        fprintf(stderr, "Unsupported OS: %s\n", uts.sysname);
        return 1;
    }

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    char uv_binary[PATH_MAX];
    snprintf(uv_binary, sizeof(uv_binary), "%s/uv/%s", cwd, is_windows ? "uv.exe" : "uv");

    if (access(uv_binary, F_OK) != 0) {
        setenv("UV_UNMANAGED_INSTALL", "uv", 1);
        if (is_windows) {
            setenv("PSModulePath", "", 1);
            setenv("PSMODULEPATH", "", 1);
            char *pwsh_argv[] = {
                "/C/Windows/System32/WindowsPowerShell/v1.0/powershell.exe",
                "-NoProfile", "-ExecutionPolicy", "Bypass",
                "-c", "irm https://astral.sh/uv/install.ps1 | iex",
                NULL
            };
            run_command(pwsh_argv[0], pwsh_argv);
        } else {
            char *sh_argv[] = {
                "sh", "-c", "curl -LsSf https://astral.sh/uv/install.sh | sh",
                NULL
            };
            run_command(sh_argv[0], sh_argv);
        }
    }

    if (access("pyproject.toml", F_OK) != 0) {
        char *init_argv[] = {uv_binary, "init", "--bare", "--no-workspace", NULL};
        run_command(uv_binary, init_argv);
    }

    char *python_path = find_python();
    if (!python_path[0]) {
        char *install_argv[] = {uv_binary, "python", "install", "--install-dir", "python", NULL};
        run_command(uv_binary, install_argv);
    }
    python_path = find_python();
    if (!python_path[0]) {
        fprintf(stderr, "Python installation not found\n");
        return 1;
    }

    char *add_argv[] = {
        uv_binary, "add", "--requirements", "requirements.txt",
        "--python", python_path, "--quiet", NULL
    };
    run_command(uv_binary, add_argv);

    const char *proj_name = get_project_name();
    char script_name[PATH_MAX];
    snprintf(script_name, sizeof(script_name), "%s.py", proj_name);

    char *run_argv[] = {
        uv_binary, "run", script_name, "--python", python_path, NULL
    };
    run_command(uv_binary, run_argv);

    return 0;
}
