#pragma once

typedef struct {
    char* key;
    char* value;
} ConfigItem;

typedef struct {
    ConfigItem* items;
    size_t count;
    size_t capacity;
} ConfigParser;

ConfigParser* parse_config(const char* filename);
const char* get_config_value(const ConfigParser* parser, const char* key);
void free_config_parser(ConfigParser* parser);
