#pragma once

typedef struct {
    char* key;
    char* value;
} ConfigItem;

typedef struct {
    ConfigItem* items;
    size_t count;
    size_t capacity;
} Config;

Config* parse_config(const char* filename);
const char* get_config_value(const Config* config, const char* key);
void free_config(Config* config);
