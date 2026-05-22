#ifndef GPS_MBTILES_H
#define GPS_MBTILES_H

#include <stdbool.h>

bool map_mbtiles_has_tile(const char* db_path, int zoom, int tile_x, int tile_y);
bool map_mbtiles_get_tile_blob(const char* db_path, int zoom, int tile_x, int tile_y, unsigned char** data, int* size);
void map_mbtiles_free_blob(unsigned char* data);
void map_mbtiles_shutdown(void);

#endif // GPS_MBTILES_H
