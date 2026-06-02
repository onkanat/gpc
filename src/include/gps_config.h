#ifndef GPS_CONFIG_H
#define GPS_CONFIG_H

#include <stdbool.h>

#define GPC_CONFIG_THEME_LENGTH 32

typedef struct {
    bool use_light_theme;
    bool auto_connect_enabled;
    char selected_port[256];
    int selected_baud;
    bool offline_only;
    bool prefer_mbtiles;
    bool show_demo_window;
    char layout_ini_path[256];
} gps_config_t;

void gps_config_init_defaults(gps_config_t* config);
bool gps_config_load(const char* path, gps_config_t* config);
bool gps_config_save(const char* path, const gps_config_t* config);

#endif // GPS_CONFIG_H
