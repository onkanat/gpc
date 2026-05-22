#ifndef GPS_TILES_H
#define GPS_TILES_H

#include <stdbool.h>
#include "gps_map.h"

typedef struct {
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int zoom;
    bool valid;
} tile_range_t;

int map_tiles_resolve_zoom_level(double view_zoom);
void map_lat_lon_to_tile(double lat, double lon, int zoom, int* tile_x, int* tile_y);
void map_tile_to_lat_lon(int tile_x, int tile_y, int zoom, double* lat, double* lon);
tile_range_t map_view_visible_tile_range(const map_view_t* view, float screen_width, float screen_height, int zoom);

#endif // GPS_TILES_H
