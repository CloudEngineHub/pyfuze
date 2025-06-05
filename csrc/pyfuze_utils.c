#include "pyfuze_utils.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "dirent.h"
#include "limits.h"
#include "sys/stat.h"

#include "pyfuze_config.h"

char python_folder_name[PATH_MAX] = {0};

char config_uv_install_script_windows[PATH_MAX] = {0};
char config_uv_install_script_unix[PATH_MAX] = {0};
char config_entry[PATH_MAX] = {0};
int config_win_gui = 0;

Config *config = NULL;

void chdir_to_executable_folder(char *argv[]) {
    char *executable_path = strdup(argv[0]);
    char *last_slash = strrchr(executable_path, '/');
    *last_slash = '\0';
    chdir(executable_path);
}

int path_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

void find_python_folder_name() {
    if (path_exists("python")) {
        DIR *d = opendir("python");
        struct dirent *ent;
        if (d) {
            while ((ent = readdir(d))) {
                if (ent->d_name[0] == '.') continue;
                strcpy(python_folder_name, ent->d_name);
                break;
            }
            closedir(d);
        }
    }
}

void read_config() {
    config = parse_config("config.txt");
    if (!config) {
        printf("Failed to parse config.txt\n");
        exit(1);
    }

    strcpy(config_uv_install_script_windows, get_config_value(config, "uv_install_script_windows"));
    strcpy(config_uv_install_script_unix, get_config_value(config, "uv_install_script_unix"));
    strcpy(config_entry, get_config_value(config, "entry"));
    config_win_gui = atoi(get_config_value(config, "win_gui"));
}

void copy_file(const char *src_path, const char *dst_path) {
    struct stat st;
    int src_fd, dst_fd;

    if ((src_fd = open(src_path, O_RDONLY)) == -1) exit(1);
    fstat(src_fd, &st);

    if ((dst_fd = creat(dst_path, st.st_mode)) == -1) {
        close(src_fd);
        exit(1);
    }

    ssize_t result = copyfd(src_fd, dst_fd, -1);
    close(src_fd);
    close(dst_fd);

    if (result == -1) exit(1);
}

void copy_directory(const char *src_dir, const char *dst_dir) {
    struct stat st;
    if (stat(src_dir, &st) != 0) exit(1);

    if (!path_exists(dst_dir)) {
        if (mkdir(dst_dir, st.st_mode) != 0) exit(1);
    }

    DIR *dir = opendir(src_dir);
    if (!dir) exit(1);

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
            exit(1);
        }

        if (S_ISDIR(st.st_mode)) {
            copy_directory(src_path, dst_path);
        } else if (S_ISREG(st.st_mode)) {
            copy_file(src_path, dst_path);
        }
    }

    closedir(dir);
}
