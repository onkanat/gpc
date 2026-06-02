#include "gps_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void trim_inplace(char* text) {
    if (!text) return;

    char* start = text;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    size_t len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[len - 1] = '\0';
        len--;
    }
}

static bool parse_bool_value(const char* value, bool default_value) {
    if (!value) return default_value;
    if (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 || strcasecmp(value, "on") == 0) {
        return true;
    }
    if (strcmp(value, "0") == 0 || strcasecmp(value, "false") == 0 || strcasecmp(value, "no") == 0 || strcasecmp(value, "off") == 0) {
        return false;
    }
    return default_value;
}

void gps_config_init_defaults(gps_config_t* config) {
    if (!config) return;

    memset(config, 0, sizeof(gps_config_t));
    config->use_light_theme = false;
    config->auto_connect_enabled = true;
    config->selected_baud = 9600;
    config->offline_only = true;
    config->prefer_mbtiles = false;
    config->show_demo_window = false;
    strncpy(config->layout_ini_path, "imgui.ini", sizeof(config->layout_ini_path) - 1);
    config->layout_ini_path[sizeof(config->layout_ini_path) - 1] = '\0';
}

bool gps_config_load(const char* path, gps_config_t* config) {
    if (!path || !config) return false;

    gps_config_init_defaults(config);

    FILE* file = fopen(path, "r");
    if (!file) {
        return false;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        trim_inplace(line);
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
            continue;
        }

        char* equals = strchr(line, '=');
        if (!equals) {
            continue;
        }

        *equals = '\0';
        char* key = line;
        char* value = equals + 1;
        trim_inplace(key);
        trim_inplace(value);

        if (strcmp(key, "use_light_theme") == 0) {
            config->use_light_theme = parse_bool_value(value, config->use_light_theme);
        } else if (strcmp(key, "auto_connect_enabled") == 0) {
            config->auto_connect_enabled = parse_bool_value(value, config->auto_connect_enabled);
        } else if (strcmp(key, "selected_port") == 0) {
            strncpy(config->selected_port, value, sizeof(config->selected_port) - 1);
            config->selected_port[sizeof(config->selected_port) - 1] = '\0';
        } else if (strcmp(key, "selected_baud") == 0) {
            int baud = atoi(value);
            if (baud > 0) {
                config->selected_baud = baud;
            }
        } else if (strcmp(key, "offline_only") == 0) {
            config->offline_only = parse_bool_value(value, config->offline_only);
        } else if (strcmp(key, "prefer_mbtiles") == 0) {
            config->prefer_mbtiles = parse_bool_value(value, config->prefer_mbtiles);
        } else if (strcmp(key, "show_demo_window") == 0) {
            config->show_demo_window = parse_bool_value(value, config->show_demo_window);
        } else if (strcmp(key, "layout_ini_path") == 0) {
            strncpy(config->layout_ini_path, value, sizeof(config->layout_ini_path) - 1);
            config->layout_ini_path[sizeof(config->layout_ini_path) - 1] = '\0';
        }
    }

    fclose(file);
    return true;
}

bool gps_config_save(const char* path, const gps_config_t* config) {
    if (!path || !config) return false;

    FILE* file = fopen(path, "w");
    if (!file) {
        return false;
    }

    fprintf(file, "# GPC configuration\n");
    fprintf(file, "use_light_theme=%d\n", config->use_light_theme ? 1 : 0);
    fprintf(file, "auto_connect_enabled=%d\n", config->auto_connect_enabled ? 1 : 0);
    fprintf(file, "selected_port=%s\n", config->selected_port);
    fprintf(file, "selected_baud=%d\n", config->selected_baud);
    fprintf(file, "offline_only=%d\n", config->offline_only ? 1 : 0);
    fprintf(file, "prefer_mbtiles=%d\n", config->prefer_mbtiles ? 1 : 0);
    fprintf(file, "show_demo_window=%d\n", config->show_demo_window ? 1 : 0);
    fprintf(file, "layout_ini_path=%s\n", config->layout_ini_path);

    fclose(file);
    return true;
}
