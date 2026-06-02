#include "gps_tile_loader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#if defined(GPC_USE_LIBPNG) && (GPC_USE_LIBPNG == 1)
#include <png.h>
#endif

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

static bool g_loader_tools_detected = false;
static bool g_loader_has_sips = false;
static bool g_loader_has_magick = false;
static bool g_loader_has_convert = false;
static bool g_loader_has_ffmpeg = false;

static int run_quiet_command(const char* cmd) {
    if (!cmd || cmd[0] == '\0') return -1;
    return system(cmd);
}

static bool command_exists_cached(const char* cmd_name) {
    if (!cmd_name || cmd_name[0] == '\0') return false;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", cmd_name);
    return (run_quiet_command(cmd) == 0);
}

static void detect_loader_toolchain_once(void) {
    if (g_loader_tools_detected) return;

#if defined(__APPLE__)
    g_loader_has_sips = command_exists_cached("sips");
#else
    g_loader_has_sips = false;
#endif
    g_loader_has_magick = command_exists_cached("magick");
    g_loader_has_convert = command_exists_cached("convert");
    g_loader_has_ffmpeg = command_exists_cached("ffmpeg");
    g_loader_tools_detected = true;
}

static bool convert_png_to_bmp_with_fallbacks(const char* png_path, const char* bmp_path) {
    if (!png_path || !bmp_path) return false;

    char cmd[1200];
    detect_loader_toolchain_once();

#if defined(__APPLE__)
    if (g_loader_has_sips) {
        snprintf(cmd,
                 sizeof(cmd),
                 "/usr/bin/sips -s format bmp '%s' --out '%s' >/dev/null 2>&1",
                 png_path,
                 bmp_path);
        if (run_quiet_command(cmd) == 0) return true;
    }
#endif

    if (g_loader_has_magick) {
        snprintf(cmd,
                 sizeof(cmd),
                 "magick convert '%s' BMP3:'%s' >/dev/null 2>&1",
                 png_path,
                 bmp_path);
        if (run_quiet_command(cmd) == 0) return true;
    }

    if (g_loader_has_convert) {
        snprintf(cmd,
                 sizeof(cmd),
                 "convert '%s' BMP3:'%s' >/dev/null 2>&1",
                 png_path,
                 bmp_path);
        if (run_quiet_command(cmd) == 0) return true;
    }

    if (g_loader_has_ffmpeg) {
        snprintf(cmd,
                 sizeof(cmd),
                 "ffmpeg -y -loglevel error -i '%s' '%s' >/dev/null 2>&1",
                 png_path,
                 bmp_path);
        if (run_quiet_command(cmd) == 0) return true;
    }

    return false;
}

static unsigned char* map_tile_load_png_rgb_libpng(const char* png_path, int* width, int* height) {
#if defined(GPC_USE_LIBPNG) && (GPC_USE_LIBPNG == 1)
    if (!png_path || !width || !height) return NULL;

    FILE* fp = fopen(png_path, "rb");
    if (!fp) return NULL;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        return NULL;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    png_uint_32 w = 0, h = 0;
    int bit_depth = 0, color_type = 0, interlace = 0, compression = 0, filter = 0;
    png_get_IHDR(png, info, &w, &h, &bit_depth, &color_type, &interlace, &compression, &filter);

    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
        png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
    }

    png_read_update_info(png, info);

    png_size_t rowbytes = png_get_rowbytes(png, info);
    png_bytep rgba = (png_bytep)malloc((size_t)rowbytes * (size_t)h);
    if (!rgba) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_bytep* rows = (png_bytep*)malloc(sizeof(png_bytep) * (size_t)h);
    if (!rows) {
        free(rgba);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    for (png_uint_32 y = 0; y < h; y++) {
        rows[y] = rgba + (size_t)y * rowbytes;
    }

    png_read_image(png, rows);
    png_read_end(png, NULL);

    unsigned char* rgb = (unsigned char*)malloc((size_t)w * (size_t)h * 3u);
    if (!rgb) {
        free(rows);
        free(rgba);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    for (png_uint_32 y = 0; y < h; y++) {
        png_bytep src = rows[y];
        unsigned char* dst = rgb + ((size_t)y * (size_t)w * 3u);
        for (png_uint_32 x = 0; x < w; x++) {
            dst[x * 3u + 0] = src[x * 4u + 0];
            dst[x * 3u + 1] = src[x * 4u + 1];
            dst[x * 3u + 2] = src[x * 4u + 2];
        }
    }

    free(rows);
    free(rgba);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    *width = (int)w;
    *height = (int)h;
    return rgb;
#else
    (void)png_path;
    (void)width;
    (void)height;
    return NULL;
#endif
}

static unsigned char* map_tile_load_png_rgb_native(const char* png_path, int* width, int* height) {
#if defined(__APPLE__)
    if (!png_path || !width || !height) return NULL;

    CGDataProviderRef provider = CGDataProviderCreateWithFilename(png_path);
    if (!provider) return NULL;

    CGImageSourceRef source = CGImageSourceCreateWithDataProvider(provider, NULL);
    CGDataProviderRelease(provider);
    if (!source) return NULL;

    CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, NULL);
    CFRelease(source);
    if (!image) return NULL;

    size_t w = CGImageGetWidth(image);
    size_t h = CGImageGetHeight(image);
    if (w == 0 || h == 0) {
        CGImageRelease(image);
        return NULL;
    }

    size_t rgba_row_bytes = w * 4u;
    size_t rgba_size = rgba_row_bytes * h;
    unsigned char* rgba = (unsigned char*)malloc(rgba_size);
    if (!rgba) {
        CGImageRelease(image);
        return NULL;
    }

    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    if (!color_space) {
        free(rgba);
        CGImageRelease(image);
        return NULL;
    }

    CGContextRef ctx = CGBitmapContextCreate(rgba,
                                             w,
                                             h,
                                             8,
                                             rgba_row_bytes,
                                             color_space,
                                             kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(color_space);

    if (!ctx) {
        free(rgba);
        CGImageRelease(image);
        return NULL;
    }

    CGContextDrawImage(ctx, CGRectMake(0, 0, (CGFloat)w, (CGFloat)h), image);
    CGContextRelease(ctx);
    CGImageRelease(image);

    size_t rgb_size = w * h * 3u;
    unsigned char* rgb = (unsigned char*)malloc(rgb_size);
    if (!rgb) {
        free(rgba);
        return NULL;
    }

    for (size_t i = 0; i < w * h; i++) {
        rgb[i * 3u + 0] = rgba[i * 4u + 0];
        rgb[i * 3u + 1] = rgba[i * 4u + 1];
        rgb[i * 3u + 2] = rgba[i * 4u + 2];
    }

    free(rgba);
    *width = (int)w;
    *height = (int)h;
    return rgb;
#else
    (void)png_path;
    (void)width;
    (void)height;
    return NULL;
#endif
}

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
    if (!file) {
        char png_path[512];
        snprintf(png_path, sizeof(png_path), "%s/%d/%d/%d.png", base_dir, zoom, tile_x, tile_y);

        FILE* png_file = fopen(png_path, "rb");
        if (png_file) {
            fclose(png_file);

            // 0) Native cross-platform path (if libpng is available at build-time).
            unsigned char* libpng_pixels = map_tile_load_png_rgb_libpng(png_path, width, height);
            if (libpng_pixels) {
                return libpng_pixels;
            }

            // 1) Native decode path (macOS): no external converter required.
            unsigned char* native_png_pixels = map_tile_load_png_rgb_native(png_path, width, height);
            if (native_png_pixels) {
                return native_png_pixels;
            }

            // 2) Fallback: convert PNG -> BMP with external tools, then load BMP.
            (void)convert_png_to_bmp_with_fallbacks(png_path, path);

            file = fopen(path, "rb");
        }
    }

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
    if (pixels) {
        return pixels;
    }

    // BMP decode basarisizsa ayni tile icin PNG fallback dene.
    char png_path[512];
    snprintf(png_path, sizeof(png_path), "%s/%d/%d/%d.png", base_dir, zoom, tile_x, tile_y);

    unsigned char* libpng_pixels = map_tile_load_png_rgb_libpng(png_path, width, height);
    if (libpng_pixels) {
        return libpng_pixels;
    }

    return map_tile_load_png_rgb_native(png_path, width, height);
}

void map_tile_free_pixels(unsigned char* pixels) {
    free(pixels);
}
