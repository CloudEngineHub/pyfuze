#include "pyfuze_unix.h"

#include "stdio.h"
#include "unistd.h"
#include "spawn.h"

#include "pyfuze_utils.h"

void run_command_unix(const char * const argv[]) {
    pid_t pid;
    int status;
    if (posix_spawnp(&pid, argv[0], NULL, NULL, (char *const *)argv, environ) != 0) {
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
    const char *uv_binary = "./uv/uv";

    if (!path_exists(uv_binary)) {
        printf("%s not found, installing...\n", uv_binary);
        setenv("UV_UNMANAGED_INSTALL", "uv", 1);
        run_command_unix((const char *const[]){
            "sh", "-c", "curl -LsSf https://astral.sh/uv/install.sh | sh", NULL
        });
    }

    if (!path_exists("pyproject.toml")) {
        printf("initializing new project with uv...\n");
        run_command_unix((const char *const[]){uv_binary, "init", "--bare", "--no-workspace", NULL});
    }

    find_python_folder_name();
    if (python_folder_name[0] == '\0') {
        printf("python not found, installing to ./python ...\n");
        run_command_unix((const char *const[]){uv_binary, "python", "install", "--install-dir", "python", NULL});
    }
    find_python_folder_name();
    if (python_folder_name[0] == '\0') {
        printf("ERROR: python installation failed\n");
        exit(1);
    }
    char python_path[PATH_MAX];
    snprintf(python_path, sizeof(python_path), "./python/%s", python_folder_name);

    if (!path_exists(".venv")) {
        printf("creating virtual environment...\n");
        if (path_exists("requirements.txt")) {
            run_command_unix((const char *const[]){uv_binary, "add", "-r", "requirements.txt", "--python", python_path, NULL});
        } else {
            run_command_unix((const char *const[]){uv_binary, "venv", "--python", python_path, NULL});
        }
    }

    if (path_exists("requirements.txt")) {
        run_command_unix((const char *const[]){uv_binary, "pip", "install", "-r", "requirements.txt", "--python", python_path, "--quiet", NULL});
    }

    run_command_unix((const char *const[]){uv_binary, "run", "--project", ".", "--directory", "src", "--script", config_entry, NULL});

    return 0;
}
