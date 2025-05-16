#include "pyfuze_utils.h"

#include "string.h"
#include "unistd.h"
#include "dirent.h"
#include "limits.h"

char cwd[PATH_MAX] = {0};
char *project_name = NULL;
char script_name[PATH_MAX] = {0};

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
