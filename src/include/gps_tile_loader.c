#include "gps_tile_loader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

void map_tile_build_path(const char* base_dir, int zoom, int tile_x, int tile_y, char* out_path, int out_path_len) {
    if (!base_dir || !out_path || out_path_len <= 0) {
        return;
    }

    // Primary MVP format: BMP tiles for zero external image dependency.
    snprintf(out_path, (size_t)out_path_len, "%s/%d/%d/%d.bmp", base_dir, zoom, tile_x, tile_y);
}

bool map_tile_file_exists(const char* base_dir, int zoom, int tile_x, int tile_y) {
    if (!base_dir) return false;

    char path[512];
    map_tile_build_path(base_dir, zoom, tile_x, tile_y, path, (int)sizeof(path));

    struct stat st;
    if (stat(path, &st) != 0) {
        // Backward compatible fallback: check PNG tile naming.
        snprintf(path, sizeof(path), "%s/%d/%d/%d.png", base_dir, zoom, tile_x, tile_y);
        if (stat(path, &st) != 0) {
            return false;
        }
    }

    return S_ISREG(st.st_mode);
}

static uint16_t read_u16_le_at(const unsigned char* data, int size, int offset) {
    if (!data || offset < 0 || offset + 1 >= size) return 0;
    return (uint16_t)(data[offset] | ((uint16_t)data[offset + 1] << 8));
}

static uint32_t read_u32_le_at(const unsigned char* data, int size, int offset) {
    if (!data || offset < 0 || offset + 3 >= size) return 0;
    return (uint32_t)(data[offset] |
                      ((uint32_t)data[offset + 1] << 8) |
                      ((uint32_t)data[offset + 2] << 16) |
                      ((uint32_t)data[offset + 3] << 24));
}

bool map_tile_blob_is_bmp(const unsigned char* data, int size) {
    return (data && size >= 2 && data[0] == 'B' && data[1] == 'M');
}

unsigned char* map_tile_decode_bmp_rgb_from_memory(const unsigned char* bmp_data, int bmp_size, int* width, int* height) {
    if (!bmp_data || bmp_size < 54 || !width || !height) return NULL;

    uint16_t bfType = read_u16_le_at(bmp_data, bmp_size, 0);
    if (bfType != 0x4D42) return NULL;

    uint32_t bfOffBits = read_u32_le_at(bmp_data, bmp_size, 10);
    uint32_t biSize = read_u32_le_at(bmp_data, bmp_size, 14);
    if (biSize < 40) return NULL;

    int32_t w_signed = (int32_t)read_u32_le_at(bmp_data, bmp_size, 18);
    int32_t h_signed = (int32_t)read_u32_le_at(bmp_data, bmp_size, 22);
    uint16_t biPlanes = read_u16_le_at(bmp_data, bmp_size, 26);
    uint16_t biBitCount = read_u16_le_at(bmp_data, bmp_size, 28);
    uint32_t biCompression = read_u32_le_at(bmp_data, bmp_size, 30);

    if (w_signed <= 0 || h_signed == 0) return NULL;
    if (biPlanes != 1 || (biBitCount != 24 && biBitCount != 32) || biCompression != 0) return NULL;

    int w = (int)w_signed;
    int h = (h_signed < 0) ? -(int)h_signed : (int)h_signed;
    bool top_down = (h_signed < 0);

    int src_channels = (biBitCount == 32) ? 4 : 3;
    int src_row_bytes = w * src_channels;
    int src_row_stride = (src_row_bytes + 3) & ~3;

    size_t pixel_data_bytes = (size_t)h * (size_t)src_row_stride;
    if ((size_t)bfOffBits + pixel_data_bytes > (size_t)bmp_size) return NULL;

    size_t out_size = (size_t)w * (size_t)h * 3u;
    unsigned char* pixels = (unsigned char*)malloc(out_size);
    if (!pixels) return NULL;

    const unsigned char* src_base = bmp_data + bfOffBits;
    for (int y = 0; y < h; y++) {
        const unsigned char* src_row = src_base + ((size_t)y * (size_t)src_row_stride);
        int dst_y = top_down ? y : (h - 1 - y);
        unsigned char* dst = pixels + ((size_t)dst_y * (size_t)w * 3u);

        for (int x = 0; x < w; x++) {
            unsigned char b = src_row[x * src_channels + 0];
            unsigned char g = src_row[x * src_channels + 1];
            unsigned char r = src_row[x * src_channels + 2];

            dst[x * 3 + 0] = r;
            dst[x * 3 + 1] = g;
            dst[x * 3 + 2] = b;
        }
    }

    *width = w;
    *height = h;
    return pixels;
}

unsigned char* map_tile_load_bmp_rgb(const char* base_dir, int zoom, int tile_x, int tile_y, int* width, int* height) {
    if (!base_dir || !width || !height) return NULL;

    char path[512];
    snprintf(path, sizeof(path), "%s/%d/%d/%d.bmp", base_dir, zoom, tile_x, tile_y);

    FILE* file = fopen(path, "rb");
    if (!file) return NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long file_size_long = ftell(file);
    if (file_size_long <= 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    int file_size = (int)file_size_long;
    unsigned char* file_data = (unsigned char*)malloc((size_t)file_size);
    if (!file_data) {
        fclose(file);
        return NULL;
    }

    if (fread(file_data, 1, (size_t)file_size, file) != (size_t)file_size) {
        free(file_data);
        fclose(file);
        return NULL;
    }

    fclose(file);

    unsigned char* pixels = map_tile_decode_bmp_rgb_from_memory(file_data, file_size, width, height);
    free(file_data);
    return pixels;
}

void map_tile_free_pixels(unsigned char* pixels) {
    free(pixels);
}
