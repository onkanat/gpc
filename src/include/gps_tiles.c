#include "gps_tiles.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double clamp_lat(double lat) {
    if (lat > 85.05112878) return 85.05112878;
    if (lat < -85.05112878) return -85.05112878;
    return lat;
}

static double clamp_lon(double lon) {
    while (lon < -180.0) lon += 360.0;
    while (lon > 180.0) lon -= 360.0;
    return lon;
}

int map_tiles_resolve_zoom_level(double view_zoom) {
    // Existing map view zoom uses ~10.0 as default; convert to slippy-style [0..19]
    double normalized = (view_zoom <= 0.0) ? 0.1 : view_zoom;
    int zoom = (int)floor(log2(normalized) + 12.0);

    if (zoom < 0) zoom = 0;
    if (zoom > 19) zoom = 19;
    return zoom;
}

void map_lat_lon_to_tile(double lat, double lon, int zoom, int* tile_x, int* tile_y) {
    if (!tile_x || !tile_y) return;

    double clamped_lat = clamp_lat(lat);
    double clamped_lon = clamp_lon(lon);

    double lat_rad = clamped_lat * M_PI / 180.0;
    double n = pow(2.0, zoom);

    double x = (clamped_lon + 180.0) / 360.0 * n;
    double y = (1.0 - asinh(tan(lat_rad)) / M_PI) / 2.0 * n;

    *tile_x = (int)floor(x);
    *tile_y = (int)floor(y);
}

void map_tile_to_lat_lon(int tile_x, int tile_y, int zoom, double* lat, double* lon) {
    if (!lat || !lon) return;

    double n = pow(2.0, zoom);

    *lon = (double)tile_x / n * 360.0 - 180.0;

    double y = M_PI * (1.0 - 2.0 * (double)tile_y / n);
    *lat = atan(sinh(y)) * 180.0 / M_PI;
}

tile_range_t map_view_visible_tile_range(const map_view_t* view, float screen_width, float screen_height, int zoom) {
    tile_range_t range = {0, 0, 0, 0, zoom, false};
    if (!view || screen_width <= 0.0f || screen_height <= 0.0f) {
        return range;
    }

    // Use existing map projection helpers for now (MVP integration)
    double lat_tl, lon_tl, lat_br, lon_br;
    map_screen_to_lat_lon(view, 0.0f, 0.0f, screen_width, screen_height, &lat_tl, &lon_tl);
    map_screen_to_lat_lon(view, screen_width, screen_height, screen_width, screen_height, &lat_br, &lon_br);

    int tx1, ty1, tx2, ty2;
    map_lat_lon_to_tile(lat_tl, lon_tl, zoom, &tx1, &ty1);
    map_lat_lon_to_tile(lat_br, lon_br, zoom, &tx2, &ty2);

    range.min_x = (tx1 < tx2) ? tx1 : tx2;
    range.max_x = (tx1 > tx2) ? tx1 : tx2;
    range.min_y = (ty1 < ty2) ? ty1 : ty2;
    range.max_y = (ty1 > ty2) ? ty1 : ty2;

    int max_tile = (1 << zoom) - 1;
    if (range.min_x < 0) range.min_x = 0;
    if (range.min_y < 0) range.min_y = 0;
    if (range.max_x > max_tile) range.max_x = max_tile;
    if (range.max_y > max_tile) range.max_y = max_tile;

    range.valid = true;
    return range;
}
