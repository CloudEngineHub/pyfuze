#include "pyfuze_utils.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "dirent.h"
#include "limits.h"

#include "pyfuze_config.h"

char python_folder_name[PATH_MAX] = {0};

char config_uv_install_script_windows[PATH_MAX] = {0};
char config_uv_install_script_unix[PATH_MAX] = {0};
char config_entry[PATH_MAX] = {0};
int config_win_gui = 0;


Config* config = NULL;

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
