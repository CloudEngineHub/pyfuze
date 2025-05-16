#include "pyfuze_config_parser.h"

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

// Parse the config file and return a ConfigParser pointer
ConfigParser* parse_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return NULL;

    ConfigParser* parser = (ConfigParser*)malloc(sizeof(ConfigParser));
    if (!parser) {
        fclose(file);
        return NULL;
    }
    parser->count = 0;
    parser->capacity = INITIAL_CAPACITY;
    parser->items = (ConfigItem*)malloc(sizeof(ConfigItem) * parser->capacity);
    if (!parser->items) {
        free(parser);
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
        if (parser->count >= parser->capacity) {
            size_t new_capacity = parser->capacity * 2;
            ConfigItem* new_items = (ConfigItem*)realloc(parser->items, sizeof(ConfigItem) * new_capacity);
            if (!new_items) break;
            parser->items = new_items;
            parser->capacity = new_capacity;
        }
        parser->items[parser->count].key = strdup(key);
        parser->items[parser->count].value = strdup(value);
        parser->count++;
    }
    fclose(file);
    return parser;
}

// Get the value for a given key
const char* get_config_value(const ConfigParser* parser, const char* key) {
    if (!parser || !key) return NULL;
    for (size_t i = 0; i < parser->count; ++i) {
        if (strcmp(parser->items[i].key, key) == 0) {
            return parser->items[i].value;
        }
    }
    return NULL;
}

// Free the ConfigParser and its resources
void free_config_parser(ConfigParser* parser) {
    if (!parser) return;
    for (size_t i = 0; i < parser->count; ++i) {
        free(parser->items[i].key);
        free(parser->items[i].value);
    }
    free(parser->items);
    free(parser);
}
