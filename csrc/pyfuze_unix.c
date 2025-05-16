#include "pyfuze_unix.h"

#include "stdio.h"
#include "unistd.h"
#include "spawn.h"

#include "pyfuze_utils.h"

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

    if (path_exists("requirements.txt")) {
        char *add_argv[] = {
            uv_binary, "pip", "install", "-r", "requirements.txt", "--python", python_path, "--quiet", NULL
        };
        run_command_unix(add_argv);
    }

    char *run_argv[] = {
        uv_binary, "run", script_name, "--python", python_path, NULL
    };
    run_command_unix(run_argv);

    return 0;
}
