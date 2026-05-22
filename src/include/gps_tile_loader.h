#ifndef GPS_TILE_LOADER_H
#define GPS_TILE_LOADER_H

#include <stdbool.h>

void map_tile_build_path(const char* base_dir, int zoom, int tile_x, int tile_y, char* out_path, int out_path_len);
bool map_tile_file_exists(const char* base_dir, int zoom, int tile_x, int tile_y);
unsigned char* map_tile_load_bmp_rgb(const char* base_dir, int zoom, int tile_x, int tile_y, int* width, int* height);
bool map_tile_blob_is_bmp(const unsigned char* data, int size);
unsigned char* map_tile_decode_bmp_rgb_from_memory(const unsigned char* bmp_data, int bmp_size, int* width, int* height);
void map_tile_free_pixels(unsigned char* pixels);

#endif // GPS_TILE_LOADER_H
