#include "pyfuze_config.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define INITIAL_CAPACITY 16

// Helper function to trim whitespace from both ends of a string
static char* trim_whitespace(char* str) {
    char* end;
    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    if (*str == 0) return str;
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    *(end + 1) = '\0';
    return str;
}

// Parse the config file and return a Config pointer
Config* parse_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return NULL;

    Config* config = (Config*)malloc(sizeof(Config));
    if (!config) {
        fclose(file);
        return NULL;
    }
    config->count = 0;
    config->capacity = INITIAL_CAPACITY;
    config->items = (ConfigItem*)malloc(sizeof(ConfigItem) * config->capacity);
    if (!config->items) {
        free(config);
        fclose(file);
        return NULL;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char* equal_sign = strchr(line, '=');
        if (!equal_sign) continue; // Skip lines without '='
        *equal_sign = '\0';
        char* key = trim_whitespace(line);
        char* value = trim_whitespace(equal_sign + 1);
        // Remove trailing newline from value
        size_t vlen = strlen(value);
        if (vlen > 0 && (value[vlen - 1] == '\n' || value[vlen - 1] == '\r')) {
            value[vlen - 1] = '\0';
        }
        // Expand capacity if needed
        if (config->count >= config->capacity) {
            size_t new_capacity = config->capacity * 2;
            ConfigItem* new_items = (ConfigItem*)realloc(config->items, sizeof(ConfigItem) * new_capacity);
            if (!new_items) break;
            config->items = new_items;
            config->capacity = new_capacity;
        }
        config->items[config->count].key = strdup(key);
        config->items[config->count].value = strdup(value);
        config->count++;
    }
    fclose(file);
    return config;
}

// Get the value for a given key
const char* get_config_value(const Config* config, const char* key) {
    if (!config || !key) return NULL;
    for (size_t i = 0; i < config->count; ++i) {
        if (strcmp(config->items[i].key, key) == 0) {
            return config->items[i].value;
        }
    }
    return NULL;
}

// Free the Config and its resources
void free_config(Config* config) {
    if (!config) return;
    for (size_t i = 0; i < config->count; ++i) {
        free(config->items[i].key);
        free(config->items[i].value);
    }
    free(config->items);
    free(config);
}
