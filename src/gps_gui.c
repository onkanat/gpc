// GPC — GPS Console
//  Serial port üzerinden GPS verilerini okuyup görselleştiren bir uygulama
//  SDL2, OpenGL ve ImGui kullanır

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(__has_include)
#if __has_include(<SDL.h>)
#include <SDL.h>
#elif __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#error "SDL2 headers not found. Please install SDL2 development packages."
#endif
#else
#include <SDL2/SDL.h> // fallback changed from <SDL.h> to satisfy linters without __has_include
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OpenGL için - deprecation uyarılarını sustur
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// SADECE cimgui
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "cimgui/cimgui_backends.h"

// GPS modules
#include "include/gps_data.h"
#include "include/gps_serial.h"
#include "include/gps_map.h"
#include "include/gps_polar.h"
#include "include/gps_compass.h"
#include "include/gps_console.h"
#include "include/tools.h"
#include "include/gps_tiles.h"
#include "include/gps_tile_loader.h"
#include "include/gps_mbtiles.h"
#include "include/gps_poi.h"
#include "include/gps_config.h"
#include "include/gps_analysis.h"
#include "include/gps_implot.h"

// GUI Configuration
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define LEFT_PANEL_WIDTH 320
#define STATUS_BAR_HEIGHT 40
#define CONNECTION_HISTORY_SIZE 5

typedef struct
{
    char port[256];
    int baud;
} connection_history_entry_t;

// Global application state
typedef struct
{
    gps_data_t gps_data;
    gps_serial_t gps_serial;
    map_system_t map_system;
    polar_view_t polar_view;
    compass_t compass;
    console_t console;
    bool show_demo_window;
    bool auto_scroll_log;
    char connection_status_text[256];
    char last_error[512];

    // UI state
    int active_tab; // 0=Telemetry, 1=Map, 2=Satellites, 3=Polar, 4=Compass, 5=Raw Data
    bool show_gpx_export_dialog;
    bool show_gpx_import_dialog;
    bool show_csv_export_dialog;
    char gpx_filename[256];
    char gpx_import_filename[256];
    char csv_export_filename[256];
    bool gpx_export_success;
    char gpx_export_message[256];
    bool gpx_import_success;
    char gpx_import_message[256];
    bool csv_export_success;
    char csv_export_message[256];

    // Help/About windows
    bool show_help_window;
    bool show_about_window;

    // Connection dialog state
    bool show_connection_dialog;
    bool auto_connect_enabled;
    char selected_port[256];
    int selected_baud;
    char available_ports[16][256];
    int available_port_count;
    connection_history_entry_t recent_connections[CONNECTION_HISTORY_SIZE];
    int recent_connection_count;
    int selected_recent_index;
    bool use_light_theme;
} app_state_t;

// Function declarations
void render_header_bar(app_state_t *app);
void render_telemetry_window(app_state_t *app);
void render_telemetry_panel(app_state_t *app);
void render_map_window(app_state_t *app);
void render_enhanced_map_panel(app_state_t *app);
void render_satellite_window(app_state_t *app);
void render_satellite_panel(app_state_t *app);
void render_polar_window(app_state_t *app);
void render_polar_view_panel(app_state_t *app);
void render_compass_window(app_state_t *app);
void render_compass_panel(app_state_t *app);
void render_raw_data_window(app_state_t *app);
void render_raw_data_panel(app_state_t *app);
void render_analysis_window(app_state_t *app);
void render_analysis_panel(app_state_t *app);
void render_status_bar(app_state_t *app);
void render_connection_dialog(app_state_t *app);
void render_gpx_export_dialog(app_state_t *app);
void render_gpx_import_dialog(app_state_t *app);
void render_csv_export_dialog(app_state_t *app);
void render_help_window(app_state_t *app);
void render_about_window(app_state_t *app);
void render_simple_markdown(const char *markdown_text);
void update_gps_data(app_state_t *app);
void setup_imgui_style(bool use_light_theme);
void ensure_data_directory(void);

static void app_state_apply_config(app_state_t *app, const gps_config_t *config)
{
    if (!app || !config)
    {
        return;
    }

    app->use_light_theme = config->use_light_theme;
    app->auto_connect_enabled = config->auto_connect_enabled;
    app->selected_baud = config->selected_baud;
    app->show_demo_window = config->show_demo_window;
    app->map_system.view.offline_only = config->offline_only;
    app->map_system.view.prefer_mbtiles = config->prefer_mbtiles;
    strncpy(app->selected_port, config->selected_port, sizeof(app->selected_port) - 1);
    app->selected_port[sizeof(app->selected_port) - 1] = '\0';
}

static void app_state_fill_config(const app_state_t *app, gps_config_t *config)
{
    if (!app || !config)
    {
        return;
    }

    gps_config_init_defaults(config);
    config->use_light_theme = app->use_light_theme;
    config->auto_connect_enabled = app->auto_connect_enabled;
    config->selected_baud = app->selected_baud;
    config->offline_only = app->map_system.view.offline_only;
    config->prefer_mbtiles = app->map_system.view.prefer_mbtiles;
    config->show_demo_window = app->show_demo_window;
    strncpy(config->selected_port, app->selected_port, sizeof(config->selected_port) - 1);
    config->selected_port[sizeof(config->selected_port) - 1] = '\0';
    strncpy(config->layout_ini_path, "imgui.ini", sizeof(config->layout_ini_path) - 1);
    config->layout_ini_path[sizeof(config->layout_ini_path) - 1] = '\0';
}

static void ui_show_tooltip(const char *text)
{
    if (text && igIsItemHovered(ImGuiHoveredFlags_None))
    {
        igSetTooltip("%s", text);
    }
}

static bool str_contains_case_insensitive(const char *haystack, const char *needle)
{
    if (!needle || needle[0] == '\0')
    {
        return true;
    }
    if (!haystack)
    {
        return false;
    }

    size_t nlen = strlen(needle);
    size_t hlen = strlen(haystack);
    if (nlen > hlen)
    {
        return false;
    }

    for (size_t i = 0; i + nlen <= hlen; i++)
    {
        bool matched = true;
        for (size_t j = 0; j < nlen; j++)
        {
            char hc = (char)tolower((unsigned char)haystack[i + j]);
            char nc = (char)tolower((unsigned char)needle[j]);
            if (hc != nc)
            {
                matched = false;
                break;
            }
        }

        if (matched)
        {
            return true;
        }
    }

    return false;
}

static void connection_history_add(app_state_t *app, const char *port, int baud)
{
    if (!app || !port || strlen(port) == 0 || baud <= 0)
    {
        return;
    }

    connection_history_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    strncpy(entry.port, port, sizeof(entry.port) - 1);
    entry.port[sizeof(entry.port) - 1] = '\0';
    entry.baud = baud;

    int existing_index = -1;
    for (int i = 0; i < app->recent_connection_count; i++)
    {
        if (app->recent_connections[i].baud == baud && strcmp(app->recent_connections[i].port, port) == 0)
        {
            existing_index = i;
            break;
        }
    }

    if (existing_index == 0)
    {
        app->selected_recent_index = 0;
        return;
    }

    if (existing_index > 0)
    {
        for (int i = existing_index; i > 0; i--)
        {
            app->recent_connections[i] = app->recent_connections[i - 1];
        }
        app->recent_connections[0] = entry;
        app->selected_recent_index = 0;
        return;
    }

    int shift_from = app->recent_connection_count;
    if (shift_from >= CONNECTION_HISTORY_SIZE)
    {
        shift_from = CONNECTION_HISTORY_SIZE - 1;
    }

    for (int i = shift_from; i > 0; i--)
    {
        app->recent_connections[i] = app->recent_connections[i - 1];
    }

    app->recent_connections[0] = entry;
    if (app->recent_connection_count < CONNECTION_HISTORY_SIZE)
    {
        app->recent_connection_count++;
    }
    app->selected_recent_index = 0;
}

#define TILE_TEXTURE_CACHE_SIZE 128

typedef struct
{
    bool valid;
    bool missing;
    int z;
    int x;
    int y;
    GLuint texture_id;
    int width;
    int height;
    uint64_t last_used_tick;
} map_tile_texture_entry_t;

static map_tile_texture_entry_t g_tile_texture_cache[TILE_TEXTURE_CACHE_SIZE];
static uint64_t g_tile_texture_tick = 1;
static uint64_t g_tile_cache_hits = 0;
static uint64_t g_tile_cache_misses = 0;
static uint64_t g_tile_cache_failures = 0;
static uint64_t g_tile_cache_mbtiles_reads = 0;
static uint64_t g_tile_cache_mbtiles_decodes = 0;
static uint64_t g_tile_cache_online_attempts = 0;
static uint64_t g_online_queue_enqueued = 0;
static uint64_t g_online_queue_dedup_skips = 0;
static uint64_t g_online_queue_full_skips = 0;
static uint64_t g_online_queue_focus_updates = 0;
static uint64_t g_online_queue_aged_picks = 0;
static uint64_t g_online_queue_retry_requeues = 0;
static uint64_t g_online_net_cooldown_events = 0;
static uint64_t g_online_net_cooldown_ms_total = 0;
static uint32_t g_online_worker_interval_last_ms = 180;
static uint32_t g_online_worker_idle_delay_last_ms = 120;
static uint64_t g_online_fetch_success = 0;
static uint64_t g_online_fetch_fail_total = 0;
static uint64_t g_online_fetch_fail_io = 0;
static uint64_t g_online_fetch_fail_download = 0;
static uint64_t g_online_fetch_fail_convert = 0;
static uint32_t g_online_last_process_ms = 0;
static uint64_t g_online_queue_fail_cache_skips = 0;
static bool g_online_tools_detected = false;
static bool g_online_has_curl = false;
static bool g_online_has_sips = false;
static bool g_online_has_magick = false;
static bool g_online_has_convert = false;
static bool g_online_has_ffmpeg = false;
static SDL_mutex *g_online_queue_mutex = NULL;
static SDL_Thread *g_online_worker_thread = NULL;
static SDL_atomic_t g_online_worker_stop;
static uint64_t g_online_worker_processed_total = 0;
static uint64_t g_online_completion_enqueued = 0;
static uint64_t g_online_completion_dropped = 0;
static uint64_t g_online_completion_drained = 0;
static uint64_t g_online_queue_enqueue_seq = 1;
static int g_online_queue_focus_zoom = -1;
static int g_online_queue_focus_x = 0;
static int g_online_queue_focus_y = 0;
static int g_online_download_fail_streak = 0;
static uint32_t g_online_network_cooldown_until_ms = 0;

#define ONLINE_FAIL_CACHE_SIZE 512
#define ONLINE_FAIL_TTL_MS 120000U

typedef struct
{
    bool valid;
    int z;
    int x;
    int y;
    uint32_t retry_after_ms;
} online_fail_cache_entry_t;

static online_fail_cache_entry_t g_online_fail_cache[ONLINE_FAIL_CACHE_SIZE];

#define ONLINE_TILE_QUEUE_SIZE 256

typedef struct
{
    bool valid;
    int z;
    int x;
    int y;
    int attempts;
    uint64_t retry_after_tick;
    uint64_t enqueue_seq;
    uint64_t enqueue_tick;
} online_tile_request_t;

static online_tile_request_t g_online_tile_queue[ONLINE_TILE_QUEUE_SIZE];
static uint64_t g_online_tile_queue_tick = 1;

#define ONLINE_COMPLETION_QUEUE_SIZE 512
typedef struct
{
    int z;
    int x;
    int y;
} online_tile_completion_t;

static online_tile_completion_t g_online_completion_queue[ONLINE_COMPLETION_QUEUE_SIZE];
static int g_online_completion_head = 0;
static int g_online_completion_tail = 0;
static int g_online_completion_count = 0;

typedef struct
{
    int z;
    int x;
    int y;
    int attempts;
} online_tile_job_t;

static int online_tile_worker_thread_fn(void *userdata);
static int online_completion_drain_and_invalidate_cache(void);

static bool ensure_directory_recursive(const char *path)
{
    if (!path || path[0] == '\0')
    {
        return false;
    }

    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);

    size_t len = strlen(tmp);
    if (len == 0)
    {
        return false;
    }

    if (tmp[len - 1] == '/')
    {
        tmp[len - 1] = '\0';
    }

    for (char *p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
            {
                return false;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
    {
        return false;
    }

    return true;
}

static bool ensure_tile_parent_dirs(const char *base_dir, int z, int x)
{
    if (!base_dir)
    {
        return false;
    }

    char z_dir[512];
    char x_dir[512];
    snprintf(z_dir, sizeof(z_dir), "%s/%d", base_dir, z);
    snprintf(x_dir, sizeof(x_dir), "%s/%d", z_dir, x);

    if (!ensure_directory_recursive(z_dir))
    {
        return false;
    }
    if (!ensure_directory_recursive(x_dir))
    {
        return false;
    }

    return true;
}

typedef enum
{
    ONLINE_FETCH_OK = 0,
    ONLINE_FETCH_FAIL_IO = 1,
    ONLINE_FETCH_FAIL_DOWNLOAD = 2,
    ONLINE_FETCH_FAIL_CONVERT = 3
} online_fetch_status_t;

static int run_quiet_command(const char *cmd)
{
    if (!cmd || cmd[0] == '\0')
    {
        return -1;
    }
    return system(cmd);
}

static void online_queue_lock(void)
{
    if (g_online_queue_mutex)
    {
        SDL_LockMutex(g_online_queue_mutex);
    }
}

static void online_queue_unlock(void)
{
    if (g_online_queue_mutex)
    {
        SDL_UnlockMutex(g_online_queue_mutex);
    }
}

static bool online_completion_enqueue_nolock(int z, int x, int y)
{
    if (g_online_completion_count >= ONLINE_COMPLETION_QUEUE_SIZE)
    {
        g_online_completion_dropped++;
        return false;
    }

    g_online_completion_queue[g_online_completion_tail].z = z;
    g_online_completion_queue[g_online_completion_tail].x = x;
    g_online_completion_queue[g_online_completion_tail].y = y;
    g_online_completion_tail = (g_online_completion_tail + 1) % ONLINE_COMPLETION_QUEUE_SIZE;
    g_online_completion_count++;
    g_online_completion_enqueued++;
    return true;
}

static bool online_completion_dequeue_nolock(online_tile_completion_t *out)
{
    if (!out || g_online_completion_count <= 0)
    {
        return false;
    }

    *out = g_online_completion_queue[g_online_completion_head];
    g_online_completion_head = (g_online_completion_head + 1) % ONLINE_COMPLETION_QUEUE_SIZE;
    g_online_completion_count--;
    return true;
}

static bool command_exists_cached(const char *cmd_name)
{
    if (!cmd_name || cmd_name[0] == '\0')
    {
        return false;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", cmd_name);
    return (run_quiet_command(cmd) == 0);
}

static void detect_online_toolchain_once(void)
{
    if (g_online_tools_detected)
    {
        return;
    }

    g_online_has_curl = command_exists_cached("curl");
#if defined(__APPLE__)
    g_online_has_sips = command_exists_cached("sips");
#else
    g_online_has_sips = false;
#endif
    g_online_has_magick = command_exists_cached("magick");
    g_online_has_convert = command_exists_cached("convert");
    g_online_has_ffmpeg = command_exists_cached("ffmpeg");
    g_online_tools_detected = true;
}

static bool convert_png_to_bmp_with_fallbacks(const char *png_path, const char *bmp_path)
{
    if (!png_path || !bmp_path)
    {
        return false;
    }

    char cmd[1600];
    detect_online_toolchain_once();

#if defined(__APPLE__)
    if (g_online_has_sips)
    {
        snprintf(cmd,
                 sizeof(cmd),
                 "/usr/bin/sips -s format bmp '%s' --out '%s' >/dev/null 2>&1",
                 png_path,
                 bmp_path);
        if (run_quiet_command(cmd) == 0)
        {
            return true;
        }
    }
#endif

    // ImageMagick (new CLI)
    if (g_online_has_magick)
    {
        snprintf(cmd,
                 sizeof(cmd),
                 "magick convert '%s' BMP3:'%s' >/dev/null 2>&1",
                 png_path,
                 bmp_path);
        if (run_quiet_command(cmd) == 0)
        {
            return true;
        }
    }

    // ImageMagick (legacy CLI)
    if (g_online_has_convert)
    {
        snprintf(cmd,
                 sizeof(cmd),
                 "convert '%s' BMP3:'%s' >/dev/null 2>&1",
                 png_path,
                 bmp_path);
        if (run_quiet_command(cmd) == 0)
        {
            return true;
        }
    }

    // FFmpeg fallback
    if (g_online_has_ffmpeg)
    {
        snprintf(cmd,
                 sizeof(cmd),
                 "ffmpeg -y -loglevel error -i '%s' '%s' >/dev/null 2>&1",
                 png_path,
                 bmp_path);
        if (run_quiet_command(cmd) == 0)
        {
            return true;
        }
    }

    return false;
}

static bool move_file_with_copy_fallback(const char *src_path, const char *dst_path)
{
    if (!src_path || !dst_path || src_path[0] == '\0' || dst_path[0] == '\0')
    {
        return false;
    }

    if (rename(src_path, dst_path) == 0)
    {
        return true;
    }

    FILE *src = fopen(src_path, "rb");
    if (!src)
    {
        return false;
    }

    FILE *dst = fopen(dst_path, "wb");
    if (!dst)
    {
        fclose(src);
        return false;
    }

    bool ok = true;
    unsigned char buffer[8192];
    while (1)
    {
        size_t nread = fread(buffer, 1, sizeof(buffer), src);
        if (nread > 0)
        {
            size_t nwritten = fwrite(buffer, 1, nread, dst);
            if (nwritten != nread)
            {
                ok = false;
                break;
            }
        }

        if (nread < sizeof(buffer))
        {
            if (ferror(src))
            {
                ok = false;
            }
            break;
        }
    }

    if (fclose(dst) != 0)
    {
        ok = false;
    }
    fclose(src);

    if (!ok)
    {
        remove(dst_path);
        return false;
    }

    if (remove(src_path) != 0)
    {
        return false;
    }

    return true;
}

static bool online_tile_fetch_and_cache(const char *base_dir, int z, int x, int y, online_fetch_status_t *status_out)
{
    if (status_out)
    {
        *status_out = ONLINE_FETCH_OK;
    }

    if (!base_dir)
    {
        if (status_out)
        {
            *status_out = ONLINE_FETCH_FAIL_IO;
        }
        return false;
    }

    detect_online_toolchain_once();
    if (!g_online_has_curl)
    {
        if (status_out)
        {
            *status_out = ONLINE_FETCH_FAIL_DOWNLOAD;
        }
        return false;
    }

    if (!ensure_tile_parent_dirs(base_dir, z, x))
    {
        if (status_out)
        {
            *status_out = ONLINE_FETCH_FAIL_IO;
        }
        return false;
    }

    char bmp_path[512];
    map_tile_build_path(base_dir, z, x, y, bmp_path, (int)sizeof(bmp_path));

    char tmp_png[512];
    const char *tmp_root = getenv("TMPDIR");
    if (!tmp_root || tmp_root[0] == '\0')
    {
        tmp_root = "data";
    }
    snprintf(tmp_png, sizeof(tmp_png), "%s/gpc_tile_%d_%d_%d.png", tmp_root, z, x, y);

    // OSM tile policy: identify the application with a stable User-Agent
    // and provide a meaningful Referer URL.
    static const char *k_osm_user_agent = "GPC-GPS-Console/1.0 (+https://github.com/onkanat/gpc)";
    static const char *k_osm_referer = "https://github.com/onkanat/gpc";

    char cmd[1800];
    snprintf(cmd,
             sizeof(cmd),
             "curl -L -sS --fail --connect-timeout 4 --max-time 10 "
             "-H 'User-Agent: %s' -e '%s' "
             "'https://tile.openstreetmap.org/%d/%d/%d.png' -o '%s'",
             k_osm_user_agent,
             k_osm_referer,
             z,
             x,
             y,
             tmp_png);
    int curl_rc = system(cmd);
    if (curl_rc != 0)
    {
        remove(tmp_png);
        if (status_out)
        {
            *status_out = ONLINE_FETCH_FAIL_DOWNLOAD;
        }
        return false;
    }

    bool converted = convert_png_to_bmp_with_fallbacks(tmp_png, bmp_path);
    if (converted)
    {
        // BMP uretimi basarili olsa bile PNG'yi de cache'te tut:
        // bazi araclarin urettigi BMP varyantlari decoder tarafinda
        // desteklenmeyebilir; PNG fallback goruntulenmeyi garanti eder.
        char png_path[512];
        snprintf(png_path, sizeof(png_path), "%s/%d/%d/%d.png", base_dir, z, x, y);
        if (!move_file_with_copy_fallback(tmp_png, png_path))
        {
            remove(tmp_png);
        }
        return true;
    }

    // Conversion başarısızsa PNG dosyasını cache'te tut.
    // Not: Loader artık native PNG decode zincirine sahip olduğundan,
    // PNG cache yazımı başarılıysa bu tile "fetch success" kabul edilir.
    char png_path[512];
    snprintf(png_path, sizeof(png_path), "%s/%d/%d/%d.png", base_dir, z, x, y);
    if (move_file_with_copy_fallback(tmp_png, png_path))
    {
        return true;
    }

    remove(tmp_png);

    if (status_out)
    {
        *status_out = ONLINE_FETCH_FAIL_IO;
    }
    return false;
}

static int online_tile_queue_pending_count(void)
{
    int count = 0;
    online_queue_lock();
    for (int i = 0; i < ONLINE_TILE_QUEUE_SIZE; i++)
    {
        if (g_online_tile_queue[i].valid)
        {
            count++;
        }
    }
    online_queue_unlock();
    return count;
}

static bool online_fail_cache_contains_active_nolock(int z, int x, int y)
{
    uint32_t now_ms = SDL_GetTicks();

    for (int i = 0; i < ONLINE_FAIL_CACHE_SIZE; i++)
    {
        online_fail_cache_entry_t *e = &g_online_fail_cache[i];
        if (!e->valid)
        {
            continue;
        }

        if (e->z == z && e->x == x && e->y == y)
        {
            if (now_ms < e->retry_after_ms)
            {
                return true;
            }

            // TTL dolduysa kaydi temizle
            e->valid = false;
            return false;
        }
    }

    return false;
}

static void online_fail_cache_mark_nolock(int z, int x, int y, uint32_t ttl_ms)
{
    uint32_t now_ms = SDL_GetTicks();
    int free_index = -1;
    int overwrite_index = 0;
    uint32_t oldest = UINT32_MAX;

    for (int i = 0; i < ONLINE_FAIL_CACHE_SIZE; i++)
    {
        online_fail_cache_entry_t *e = &g_online_fail_cache[i];
        if (e->valid)
        {
            if (e->z == z && e->x == x && e->y == y)
            {
                e->retry_after_ms = now_ms + ttl_ms;
                return;
            }

            if (e->retry_after_ms < oldest)
            {
                oldest = e->retry_after_ms;
                overwrite_index = i;
            }
        }
        else if (free_index < 0)
        {
            free_index = i;
        }
    }

    int use_index = (free_index >= 0) ? free_index : overwrite_index;
    g_online_fail_cache[use_index].valid = true;
    g_online_fail_cache[use_index].z = z;
    g_online_fail_cache[use_index].x = x;
    g_online_fail_cache[use_index].y = y;
    g_online_fail_cache[use_index].retry_after_ms = now_ms + ttl_ms;
}

static void online_fail_cache_clear_nolock(int z, int x, int y)
{
    for (int i = 0; i < ONLINE_FAIL_CACHE_SIZE; i++)
    {
        online_fail_cache_entry_t *e = &g_online_fail_cache[i];
        if (e->valid && e->z == z && e->x == x && e->y == y)
        {
            e->valid = false;
            return;
        }
    }
}

static bool online_tile_queue_insert_nolock(int z, int x, int y, int attempts, uint64_t retry_after_tick, bool count_as_enqueue)
{
    int free_index = -1;

    for (int i = 0; i < ONLINE_TILE_QUEUE_SIZE; i++)
    {
        if (g_online_tile_queue[i].valid)
        {
            if (g_online_tile_queue[i].z == z && g_online_tile_queue[i].x == x && g_online_tile_queue[i].y == y)
            {
                g_online_queue_dedup_skips++;
                return false;
            }
        }
        else if (free_index < 0)
        {
            free_index = i;
        }
    }

    if (free_index < 0)
    {
        g_online_queue_full_skips++;
        return false;
    }

    g_online_tile_queue[free_index].valid = true;
    g_online_tile_queue[free_index].z = z;
    g_online_tile_queue[free_index].x = x;
    g_online_tile_queue[free_index].y = y;
    g_online_tile_queue[free_index].attempts = attempts;
    g_online_tile_queue[free_index].retry_after_tick = retry_after_tick;
    g_online_tile_queue[free_index].enqueue_seq = g_online_queue_enqueue_seq++;
    g_online_tile_queue[free_index].enqueue_tick = g_online_tile_queue_tick;
    if (count_as_enqueue)
    {
        g_online_queue_enqueued++;
    }
    return true;
}

static uint64_t online_tile_retry_backoff_ticks(int z, int x, int y, int attempts)
{
    // Base backoff (30/60/90) + deterministic jitter ile retry spike azaltma
    uint64_t base = (uint64_t)attempts * 30ULL;
    uint32_t seed = (uint32_t)(z * 73856093) ^ (uint32_t)(x * 19349663) ^ (uint32_t)(y * 83492791) ^ (uint32_t)(attempts * 2654435761U);
    uint64_t jitter = (uint64_t)(seed % 19U); // 0..18 tick
    return base + jitter;
}

static int online_queue_pending_count_nolock(void)
{
    int pending = 0;
    for (int i = 0; i < ONLINE_TILE_QUEUE_SIZE; i++)
    {
        if (g_online_tile_queue[i].valid)
        {
            pending++;
        }
    }
    return pending;
}

static uint32_t online_worker_dynamic_interval_ms(int pending, int fail_streak)
{
    uint32_t interval = 180U;

    if (pending >= 64)
    {
        interval = 70U;
    }
    else if (pending >= 24)
    {
        interval = 95U;
    }
    else if (pending >= 8)
    {
        interval = 130U;
    }

    if (fail_streak > 0)
    {
        uint32_t penalty = (uint32_t)fail_streak * 14U;
        interval += penalty;
        if (interval > 320U)
        {
            interval = 320U;
        }
    }

    return interval;
}

static uint32_t online_worker_dynamic_idle_delay_ms(int pending, bool cooldown_active, int fail_streak)
{
    if (cooldown_active)
    {
        uint32_t delay = 90U + (uint32_t)fail_streak * 10U;
        if (delay > 220U)
        {
            delay = 220U;
        }
        return delay;
    }

    if (pending >= 48)
    {
        return 14U;
    }
    if (pending > 0)
    {
        return 30U;
    }
    return 120U;
}

static uint32_t online_tile_queue_apply_aging_bonus_nolock(uint32_t base_score, uint64_t enqueue_tick)
{
    // Worker processed-tick bazli aging: uzun sure bekleyen istekler ac kalmasin.
    const uint32_t aging_bonus_per_tick = 64U;
    const uint32_t max_aging_bonus = 20000U;

    uint64_t waited = 0;
    if (g_online_tile_queue_tick > enqueue_tick)
    {
        waited = g_online_tile_queue_tick - enqueue_tick;
    }

    uint64_t bonus64 = waited * (uint64_t)aging_bonus_per_tick;
    if (bonus64 > max_aging_bonus)
    {
        bonus64 = max_aging_bonus;
    }
    uint32_t bonus = (uint32_t)bonus64;

    if (bonus >= base_score)
    {
        return 0;
    }
    return base_score - bonus;
}

static uint32_t online_tile_queue_priority_score_nolock(int z, int x, int y)
{
    if (g_online_queue_focus_zoom < 0)
    {
        return UINT32_MAX / 4U;
    }

    int dz = abs(z - g_online_queue_focus_zoom);
    int dx = abs(x - g_online_queue_focus_x);
    int dy = abs(y - g_online_queue_focus_y);

    uint64_t score = (uint64_t)dz * 4096ULL + (uint64_t)dx * (uint64_t)dx + (uint64_t)dy * (uint64_t)dy;
    if (score > UINT32_MAX)
    {
        return UINT32_MAX;
    }
    return (uint32_t)score;
}

static void online_tile_queue_set_focus(int zoom, int x, int y)
{
    online_queue_lock();
    g_online_queue_focus_zoom = zoom;
    g_online_queue_focus_x = x;
    g_online_queue_focus_y = y;
    g_online_queue_focus_updates++;
    online_queue_unlock();
}

static bool online_tile_queue_push_if_missing(int z, int x, int y)
{
    online_queue_lock();

    if (online_fail_cache_contains_active_nolock(z, x, y))
    {
        g_online_queue_fail_cache_skips++;
        online_queue_unlock();
        return false;
    }

    bool ok = online_tile_queue_insert_nolock(z, x, y, 0, g_online_tile_queue_tick, true);
    online_queue_unlock();
    return ok;
}

static bool online_tile_queue_pop_ready_nolock(online_tile_job_t *out_job)
{
    if (!out_job)
    {
        return false;
    }

    int best_index = -1;
    uint32_t best_score = UINT32_MAX;
    uint64_t best_seq = UINT64_MAX;

    for (int i = 0; i < ONLINE_TILE_QUEUE_SIZE; i++)
    {
        if (!g_online_tile_queue[i].valid)
        {
            continue;
        }
        if (g_online_tile_queue[i].retry_after_tick > g_online_tile_queue_tick)
        {
            continue;
        }

        uint32_t base_score = online_tile_queue_priority_score_nolock(g_online_tile_queue[i].z,
                                           g_online_tile_queue[i].x,
                                           g_online_tile_queue[i].y);
        uint32_t score = online_tile_queue_apply_aging_bonus_nolock(base_score,
                                         g_online_tile_queue[i].enqueue_tick);
        uint64_t seq = g_online_tile_queue[i].enqueue_seq;

        if (best_index < 0 || score < best_score || (score == best_score && seq < best_seq))
        {
            best_index = i;
            best_score = score;
            best_seq = seq;
        }
    }

    if (best_index < 0)
    {
        return false;
    }

    out_job->z = g_online_tile_queue[best_index].z;
    out_job->x = g_online_tile_queue[best_index].x;
    out_job->y = g_online_tile_queue[best_index].y;
    out_job->attempts = g_online_tile_queue[best_index].attempts;

    uint32_t selected_base_score = online_tile_queue_priority_score_nolock(g_online_tile_queue[best_index].z,
                                                                            g_online_tile_queue[best_index].x,
                                                                            g_online_tile_queue[best_index].y);
    uint32_t selected_aged_score = online_tile_queue_apply_aging_bonus_nolock(selected_base_score,
                                                                                g_online_tile_queue[best_index].enqueue_tick);
    if (selected_aged_score < selected_base_score)
    {
        g_online_queue_aged_picks++;
    }

    g_online_tile_queue[best_index].valid = false;
    return true;
}

static int online_tile_worker_step(void)
{
    uint32_t now_ms = SDL_GetTicks();

    if (now_ms < g_online_network_cooldown_until_ms)
    {
        return 0;
    }

    int pending = 0;
    int fail_streak = 0;
    online_queue_lock();
    pending = online_queue_pending_count_nolock();
    fail_streak = g_online_download_fail_streak;
    online_queue_unlock();

    uint32_t dynamic_interval = online_worker_dynamic_interval_ms(pending, fail_streak);
    g_online_worker_interval_last_ms = dynamic_interval;

    if (g_online_last_process_ms != 0 && (now_ms - g_online_last_process_ms) < dynamic_interval)
    {
        return 0;
    }
    g_online_last_process_ms = now_ms;

    online_tile_job_t job = {};
    bool has_job = false;

    online_queue_lock();
    has_job = online_tile_queue_pop_ready_nolock(&job);
    online_queue_unlock();

    if (!has_job)
    {
        return 0;
    }

    online_fetch_status_t fetch_status = ONLINE_FETCH_OK;
    bool fetched = online_tile_fetch_and_cache("data/map_tiles", job.z, job.x, job.y, &fetch_status);

    online_queue_lock();
    g_online_tile_queue_tick++;
    g_online_worker_processed_total++;

    if (fetched)
    {
        g_online_fetch_success++;
        g_online_download_fail_streak = 0;
        g_online_network_cooldown_until_ms = 0;
        online_fail_cache_clear_nolock(job.z, job.x, job.y);
        (void)online_completion_enqueue_nolock(job.z, job.x, job.y);
    }
    else
    {
        g_tile_cache_online_attempts++;
        g_online_fetch_fail_total++;
        if (fetch_status == ONLINE_FETCH_FAIL_IO)
        {
            g_online_fetch_fail_io++;
        }
        else if (fetch_status == ONLINE_FETCH_FAIL_DOWNLOAD)
        {
            g_online_fetch_fail_download++;

            // Ardışık download hatalarında kısa adaptif cooldown uygula
            g_online_download_fail_streak++;
            if (g_online_download_fail_streak > 12)
            {
                g_online_download_fail_streak = 12;
            }

            uint32_t cooldown_ms = (uint32_t)(250U * (uint32_t)g_online_download_fail_streak);
            if (cooldown_ms > 3000U)
            {
                cooldown_ms = 3000U;
            }
            g_online_network_cooldown_until_ms = SDL_GetTicks() + cooldown_ms;
            g_online_net_cooldown_events++;
            g_online_net_cooldown_ms_total += (uint64_t)cooldown_ms;
        }
        else if (fetch_status == ONLINE_FETCH_FAIL_CONVERT)
        {
            g_online_fetch_fail_convert++;

            // Convert sorunu network sorunu olmayabilir; streak'i yumusat.
            if (g_online_download_fail_streak > 0)
            {
                g_online_download_fail_streak--;
            }
        }
        else if (g_online_download_fail_streak > 0)
        {
            g_online_download_fail_streak--;
        }

        int next_attempts = job.attempts + 1;
        if (next_attempts >= 3)
        {
            online_fail_cache_mark_nolock(job.z, job.x, job.y, ONLINE_FAIL_TTL_MS);
        }
        else
        {
            // Jitter'li backoff: 30/60/90 + deterministic jitter
            uint64_t delay_ticks = online_tile_retry_backoff_ticks(job.z, job.x, job.y, next_attempts);
            (void)online_tile_queue_insert_nolock(job.z,
                                                  job.x,
                                                  job.y,
                                                  next_attempts,
                                                  g_online_tile_queue_tick + delay_ticks,
                                                  false);
            g_online_queue_retry_requeues++;
        }
    }

    online_queue_unlock();
    return 1;
}

static int online_completion_drain_and_invalidate_cache(void)
{
    int drained = 0;
    online_tile_completion_t item;

    while (1)
    {
        online_queue_lock();
        bool has_item = online_completion_dequeue_nolock(&item);
        online_queue_unlock();

        if (!has_item)
        {
            break;
        }

        drained++;
        g_online_completion_drained++;

        for (int i = 0; i < TILE_TEXTURE_CACHE_SIZE; i++)
        {
            if (!g_tile_texture_cache[i].valid)
            {
                continue;
            }

            if (g_tile_texture_cache[i].z == item.z &&
                g_tile_texture_cache[i].x == item.x &&
                g_tile_texture_cache[i].y == item.y)
            {
                if (g_tile_texture_cache[i].texture_id != 0)
                {
                    glDeleteTextures(1, &g_tile_texture_cache[i].texture_id);
                }

                g_tile_texture_cache[i].valid = false;
                g_tile_texture_cache[i].missing = false;
                g_tile_texture_cache[i].texture_id = 0;
                g_tile_texture_cache[i].last_used_tick = 0;
            }
        }
    }

    return drained;
}

static int online_tile_worker_thread_fn(void *userdata)
{
    (void)userdata;

    while (SDL_AtomicGet(&g_online_worker_stop) == 0)
    {
        int did_work = online_tile_worker_step();
        if (did_work != 0)
        {
            SDL_Delay(6);
            continue;
        }

        int pending = 0;
        int fail_streak = 0;
        bool cooldown_active = false;
        uint32_t now_ms = SDL_GetTicks();
        online_queue_lock();
        pending = online_queue_pending_count_nolock();
        fail_streak = g_online_download_fail_streak;
        cooldown_active = (now_ms < g_online_network_cooldown_until_ms);
        online_queue_unlock();

        uint32_t idle_delay = online_worker_dynamic_idle_delay_ms(pending, cooldown_active, fail_streak);
        g_online_worker_idle_delay_last_ms = idle_delay;
        SDL_Delay(idle_delay);
    }

    return 0;
}

static void online_tile_queue_get_stats(int *pending_out,
                                        uint64_t *processed_total_out,
                                        uint64_t *enqueued_total_out,
                                        uint64_t *success_total_out,
                                        uint64_t *fail_total_out)
{
    int pending = 0;
    uint64_t processed_total = 0;
    uint64_t enqueued_total = 0;
    uint64_t success_total = 0;
    uint64_t fail_total = 0;

    online_queue_lock();
    for (int i = 0; i < ONLINE_TILE_QUEUE_SIZE; i++)
    {
        if (g_online_tile_queue[i].valid)
        {
            pending++;
        }
    }
    processed_total = g_online_worker_processed_total;
    enqueued_total = g_online_queue_enqueued;
    success_total = g_online_fetch_success;
    fail_total = g_online_fetch_fail_total;
    online_queue_unlock();

    if (pending_out)
    {
        *pending_out = pending;
    }
    if (processed_total_out)
    {
        *processed_total_out = processed_total;
    }
    if (enqueued_total_out)
    {
        *enqueued_total_out = enqueued_total;
    }
    if (success_total_out)
    {
        *success_total_out = success_total;
    }
    if (fail_total_out)
    {
        *fail_total_out = fail_total;
    }
}

static GLuint map_tile_texture_get_or_load(const char *base_dir,
                                           const char *mbtiles_path,
                                           bool prefer_mbtiles,
                                           bool offline_only,
                                           int z,
                                           int x,
                                           int y)
{
    int free_index = -1;
    int lru_index = 0;
    uint64_t lru_tick = UINT64_MAX;

    for (int i = 0; i < TILE_TEXTURE_CACHE_SIZE; i++)
    {
        if (g_tile_texture_cache[i].valid)
        {
            if (g_tile_texture_cache[i].z == z && g_tile_texture_cache[i].x == x && g_tile_texture_cache[i].y == y)
            {
                g_tile_texture_cache[i].last_used_tick = g_tile_texture_tick++;

                // Missing placeholder cache hit: online fetch retries should continue.
                // Otherwise a transient network failure can leave this tile stuck forever
                // until cache eviction happens.
                if (g_tile_texture_cache[i].missing && !offline_only)
                {
                    (void)online_tile_queue_push_if_missing(z, x, y);
                }

                g_tile_cache_hits++;
                return g_tile_texture_cache[i].texture_id;
            }

            if (g_tile_texture_cache[i].last_used_tick < lru_tick)
            {
                lru_tick = g_tile_texture_cache[i].last_used_tick;
                lru_index = i;
            }
        }
        else if (free_index < 0)
        {
            free_index = i;
        }
    }

    int width = 0, height = 0;
    unsigned char *pixels = NULL;

    if (prefer_mbtiles)
    {
        unsigned char *tile_blob = NULL;
        int tile_blob_size = 0;
        if (map_mbtiles_get_tile_blob(mbtiles_path, z, x, y, &tile_blob, &tile_blob_size))
        {
            g_tile_cache_mbtiles_reads++;
            if (map_tile_blob_is_bmp(tile_blob, tile_blob_size))
            {
                pixels = map_tile_decode_bmp_rgb_from_memory(tile_blob, tile_blob_size, &width, &height);
                if (pixels)
                {
                    g_tile_cache_mbtiles_decodes++;
                }
            }
            map_mbtiles_free_blob(tile_blob);
        }

        if (!pixels)
        {
            pixels = map_tile_load_bmp_rgb(base_dir, z, x, y, &width, &height);
        }
    }
    else
    {
        pixels = map_tile_load_bmp_rgb(base_dir, z, x, y, &width, &height);
        if (!pixels)
        {
            unsigned char *tile_blob = NULL;
            int tile_blob_size = 0;
            if (map_mbtiles_get_tile_blob(mbtiles_path, z, x, y, &tile_blob, &tile_blob_size))
            {
                g_tile_cache_mbtiles_reads++;
                if (map_tile_blob_is_bmp(tile_blob, tile_blob_size))
                {
                    pixels = map_tile_decode_bmp_rgb_from_memory(tile_blob, tile_blob_size, &width, &height);
                    if (pixels)
                    {
                        g_tile_cache_mbtiles_decodes++;
                    }
                }
                map_mbtiles_free_blob(tile_blob);
            }
        }
    }

    if (!pixels && !offline_only)
    {
        // Phase-5 scaffold: immediate network yerine istek kuyruguna eklenir.
        (void)online_tile_queue_push_if_missing(z, x, y);
    }

    g_tile_cache_misses++;
    if (!pixels || width <= 0 || height <= 0)
    {
        map_tile_free_pixels(pixels);

        int use_index = (free_index >= 0) ? free_index : lru_index;
        if (g_tile_texture_cache[use_index].valid && g_tile_texture_cache[use_index].texture_id != 0)
        {
            glDeleteTextures(1, &g_tile_texture_cache[use_index].texture_id);
        }
        g_tile_texture_cache[use_index].valid = true;
        g_tile_texture_cache[use_index].missing = true;
        g_tile_texture_cache[use_index].z = z;
        g_tile_texture_cache[use_index].x = x;
        g_tile_texture_cache[use_index].y = y;
        g_tile_texture_cache[use_index].texture_id = 0;
        g_tile_texture_cache[use_index].width = 0;
        g_tile_texture_cache[use_index].height = 0;
        g_tile_texture_cache[use_index].last_used_tick = g_tile_texture_tick++;
        g_tile_cache_failures++;
        return 0;
    }

    GLuint texture_id = 0;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    map_tile_free_pixels(pixels);

    int use_index = (free_index >= 0) ? free_index : lru_index;
    if (g_tile_texture_cache[use_index].valid && g_tile_texture_cache[use_index].texture_id != 0)
    {
        glDeleteTextures(1, &g_tile_texture_cache[use_index].texture_id);
    }

    g_tile_texture_cache[use_index].valid = true;
    g_tile_texture_cache[use_index].missing = false;
    g_tile_texture_cache[use_index].z = z;
    g_tile_texture_cache[use_index].x = x;
    g_tile_texture_cache[use_index].y = y;
    g_tile_texture_cache[use_index].texture_id = texture_id;
    g_tile_texture_cache[use_index].width = width;
    g_tile_texture_cache[use_index].height = height;
    g_tile_texture_cache[use_index].last_used_tick = g_tile_texture_tick++;

    return texture_id;
}

static void map_tile_texture_cache_cleanup(void)
{
    for (int i = 0; i < TILE_TEXTURE_CACHE_SIZE; i++)
    {
        if (g_tile_texture_cache[i].valid && g_tile_texture_cache[i].texture_id != 0)
        {
            glDeleteTextures(1, &g_tile_texture_cache[i].texture_id);
        }
        g_tile_texture_cache[i].valid = false;
        g_tile_texture_cache[i].missing = false;
        g_tile_texture_cache[i].texture_id = 0;
        g_tile_texture_cache[i].last_used_tick = 0;
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // Ensure data directory exists
    ensure_data_directory();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window *window = SDL_CreateWindow(
        "GPC — GPS Console",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window)
    {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    // ImGui başlat
    ImGuiContext *ctx = igCreateContext(NULL);
    igSetCurrentContext(ctx);
    gps_implot_create_context();
    ImGuiIO *io = cImGui_GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io->IniFilename = "imgui.ini";

    // cimgui backend'lerini başlat
    cImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    cImGui_ImplOpenGL3_Init("#version 150");

    // Initialize application state
    app_state_t app_state;
    memset(&app_state, 0, sizeof(app_state));
    gps_data_init(&app_state.gps_data);
    gps_serial_init(&app_state.gps_serial);
    map_system_init(&app_state.map_system);
    polar_view_init(&app_state.polar_view);
    compass_init(&app_state.compass);
    console_init(&app_state.console);
    app_state.show_demo_window = false;
    app_state.auto_scroll_log = true;
    app_state.active_tab = 0;
    app_state.show_gpx_export_dialog = false;
    app_state.show_gpx_import_dialog = false;
    app_state.show_csv_export_dialog = false;
    app_state.show_help_window = false;
    app_state.show_about_window = false;
    strcpy(app_state.connection_status_text, "Not Connected");
    app_state.last_error[0] = '\0';
    strcpy(app_state.gpx_filename, "data/gps_track.gpx");
    strcpy(app_state.gpx_import_filename, "data/gps_track.gpx");
    strcpy(app_state.csv_export_filename, "data/gps_track.csv");
    app_state.gpx_import_message[0] = '\0';
    app_state.csv_export_message[0] = '\0';

    // Initialize connection dialog state
    app_state.show_connection_dialog = false;
    app_state.auto_connect_enabled = true; // Default enabled
    strcpy(app_state.selected_port, "");
    app_state.selected_baud = 9600;
    app_state.available_port_count = 0;
    app_state.recent_connection_count = 0;
    app_state.selected_recent_index = -1;
    app_state.use_light_theme = false;

    gps_config_t app_config;
    bool has_config = gps_config_load("data/gpc_config.ini", &app_config);
    if (!has_config)
    {
        gps_config_init_defaults(&app_config);
    }
    app_state_apply_config(&app_state, &app_config);

    // Setup theme after config is loaded
    setup_imgui_style(app_state.use_light_theme);

    g_online_queue_mutex = SDL_CreateMutex();
    SDL_AtomicSet(&g_online_worker_stop, 0);
    if (g_online_queue_mutex)
    {
        g_online_worker_thread = SDL_CreateThread(online_tile_worker_thread_fn,
                                                  "gpc-online-tile-worker",
                                                  NULL);
    }

    bool done = false;
    ImVec4 clear_color = {0.11f, 0.11f, 0.11f, 1.00f}; // Dark background

    // Dynamic FPS control (v3.2): active ~60 FPS, idle ~15 FPS
    const uint32_t frame_target_active_ms = 16U;
    const uint32_t frame_target_idle_ms = 66U;

    while (!done)
    {
        uint32_t frame_begin_ms = SDL_GetTicks();

        SDL_Event event;
        bool had_input_event = false;
        while (SDL_PollEvent(&event))
        {
            had_input_event = true;
            cImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE)
                done = true;
        }

        // Update GPS data
        update_gps_data(&app_state);
        compass_update(&app_state.compass, &app_state.gps_data);

        cImGui_ImplOpenGL3_NewFrame();
        cImGui_ImplSDL2_NewFrame();
        igNewFrame();

        // Main dockspace
        ImGuiViewport *viewport = igGetMainViewport();
        igSetNextWindowPos(viewport->WorkPos, 0, (ImVec2){0, 0});
        igSetNextWindowSize(viewport->WorkSize, 0);
        igSetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        bool open = true;
        igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 0.0f);
        igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0.0f, 0.0f});

        igBegin("DockSpace Demo", &open, window_flags);
        igPopStyleVar(3);

        // Render header/menu bar
        render_header_bar(&app_state);

        // Submit the DockSpace
        ImGuiID dockspace_id = igGetID_Str("MyDockSpace");
        igDockSpace(dockspace_id, (ImVec2){0.0f, 0.0f}, ImGuiDockNodeFlags_None, NULL);

        // Render individual panels as separate dockable windows
        render_telemetry_window(&app_state);
        render_map_window(&app_state);
        render_satellite_window(&app_state);
        render_polar_window(&app_state);
        render_compass_window(&app_state);
        render_raw_data_window(&app_state);
        render_analysis_window(&app_state);

        render_status_bar(&app_state);
        render_connection_dialog(&app_state);
        render_gpx_export_dialog(&app_state);
        render_gpx_import_dialog(&app_state);
        render_csv_export_dialog(&app_state);

        // Help and About windows
        if (app_state.show_help_window)
        {
            render_help_window(&app_state);
        }

        if (app_state.show_about_window)
        {
            render_about_window(&app_state);
        }

        // Demo window for development
        if (app_state.show_demo_window)
        {
            igShowDemoWindow(&app_state.show_demo_window);
        }

        igEnd();

        igRender();
        glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        cImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        SDL_GL_SwapWindow(window);

        // Basit idle tespiti: etkileşim/bağlantı/iş yükü yoksa düşük FPS'e geç.
        int online_pending = online_tile_queue_pending_count();
        bool has_foreground_activity = had_input_event ||
                                       app_state.show_connection_dialog ||
                                       app_state.show_gpx_export_dialog ||
                                       app_state.show_gpx_import_dialog ||
                                       app_state.show_csv_export_dialog ||
                                       app_state.show_help_window ||
                                       app_state.show_about_window ||
                                       app_state.show_demo_window;
        bool has_background_activity = gps_serial_is_open(&app_state.gps_serial) ||
                                       app_state.gps_data.logging_enabled ||
                                       app_state.map_system.track.recording ||
                                       (online_pending > 0);

        uint32_t frame_target_ms = (has_foreground_activity || has_background_activity)
                                       ? frame_target_active_ms
                                       : frame_target_idle_ms;

        uint32_t frame_elapsed_ms = SDL_GetTicks() - frame_begin_ms;
        if (frame_elapsed_ms < frame_target_ms)
        {
            SDL_Delay(frame_target_ms - frame_elapsed_ms);
        }
    }

    // Cleanup
    gps_data_cleanup(&app_state.gps_data);
    gps_serial_cleanup(&app_state.gps_serial);
    map_system_cleanup(&app_state.map_system);
    polar_view_cleanup(&app_state.polar_view);
    compass_cleanup(&app_state.compass);
    console_cleanup(&app_state.console);
    map_tile_texture_cache_cleanup();
    SDL_AtomicSet(&g_online_worker_stop, 1);
    if (g_online_worker_thread)
    {
        SDL_WaitThread(g_online_worker_thread, NULL);
        g_online_worker_thread = NULL;
    }
    if (g_online_queue_mutex)
    {
        SDL_DestroyMutex(g_online_queue_mutex);
        g_online_queue_mutex = NULL;
    }

    // Persist configuration before subsystem shutdown mutates state.
    app_state_fill_config(&app_state, &app_config);
    (void)gps_config_save("data/gpc_config.ini", &app_config);

    map_mbtiles_shutdown();
    poi_db_shutdown();

    cImGui_ImplOpenGL3_Shutdown();
    cImGui_ImplSDL2_Shutdown();
    gps_implot_destroy_context();
    igDestroyContext(ctx);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void setup_imgui_style(bool use_light_theme)
{
    ImGuiStyle *style = igGetStyle();

    if (use_light_theme)
    {
        igStyleColorsLight(style);
        style->WindowRounding = 6.0f;
        style->ChildRounding = 4.0f;
        style->FrameRounding = 4.0f;
        style->PopupRounding = 4.0f;
        style->ScrollbarRounding = 6.0f;
        style->GrabRounding = 4.0f;
        style->TabRounding = 4.0f;
        style->WindowPadding = (ImVec2){10.0f, 10.0f};
        style->FramePadding = (ImVec2){8.0f, 4.0f};
        style->ItemSpacing = (ImVec2){8.0f, 6.0f};
        style->ItemInnerSpacing = (ImVec2){6.0f, 4.0f};
        return;
    }

    // Colors based on NOTE.md specifications
    style->Colors[ImGuiCol_WindowBg] = (ImVec4){0.17f, 0.17f, 0.17f, 1.00f}; // #2B2B2B
    style->Colors[ImGuiCol_ChildBg] = (ImVec4){0.12f, 0.12f, 0.12f, 1.00f};  // #1E1E1E
    style->Colors[ImGuiCol_PopupBg] = (ImVec4){0.17f, 0.17f, 0.17f, 1.00f};
    style->Colors[ImGuiCol_Border] = (ImVec4){0.30f, 0.30f, 0.30f, 1.00f};
    style->Colors[ImGuiCol_FrameBg] = (ImVec4){0.20f, 0.20f, 0.20f, 1.00f};
    style->Colors[ImGuiCol_FrameBgHovered] = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
    style->Colors[ImGuiCol_FrameBgActive] = (ImVec4){0.00f, 0.59f, 1.00f, 0.30f}; // #0096FF
    style->Colors[ImGuiCol_TitleBg] = (ImVec4){0.12f, 0.12f, 0.12f, 1.00f};
    style->Colors[ImGuiCol_TitleBgActive] = (ImVec4){0.00f, 0.59f, 1.00f, 1.00f}; // #0096FF
    style->Colors[ImGuiCol_Text] = (ImVec4){0.86f, 0.86f, 0.86f, 1.00f};          // #DADADA
    style->Colors[ImGuiCol_Button] = (ImVec4){0.20f, 0.20f, 0.20f, 1.00f};
    style->Colors[ImGuiCol_ButtonHovered] = (ImVec4){0.00f, 0.59f, 1.00f, 0.50f};
    style->Colors[ImGuiCol_ButtonActive] = (ImVec4){0.00f, 0.59f, 1.00f, 1.00f};

    // Rounded corners
    style->WindowRounding = 6.0f;
    style->ChildRounding = 4.0f;
    style->FrameRounding = 4.0f;
    style->PopupRounding = 4.0f;
    style->ScrollbarRounding = 6.0f;
    style->GrabRounding = 4.0f;
    style->TabRounding = 4.0f;

    // Spacing
    style->WindowPadding = (ImVec2){10.0f, 10.0f};
    style->FramePadding = (ImVec2){8.0f, 4.0f};
    style->ItemSpacing = (ImVec2){8.0f, 6.0f};
    style->ItemInnerSpacing = (ImVec2){6.0f, 4.0f};
}

void render_header_bar(app_state_t *app)
{
    if (igBeginMenuBar())
    {
        // Logo/Title
        igText("[GPC — GPS Console]");

        igSeparator();

        // Connection menu
        if (igBeginMenu("Connection", true))
        {
            if (igMenuItem_Bool("Connect...", NULL, false, !gps_serial_is_open(&app->gps_serial)))
            {
                app->show_connection_dialog = true;
                // Refresh available ports when opening dialog
                gps_serial_list_ports(app->available_ports, 16, &app->available_port_count);
            }
            if (igMenuItem_Bool("Disconnect", NULL, false, gps_serial_is_open(&app->gps_serial)))
            {
                gps_serial_close(&app->gps_serial);
                app->gps_data.status = GPS_STATUS_DISCONNECTED;
                strcpy(app->connection_status_text, "Disconnected");
                strcpy(app->gps_data.port_name, "");
                app->gps_data.baud_rate = 0;
            }
            igSeparator();
            bool auto_connect = app->auto_connect_enabled;
            if (igMenuItem_Bool("Auto-connect", NULL, auto_connect, true))
            {
                app->auto_connect_enabled = !app->auto_connect_enabled;
                if (app->auto_connect_enabled && !gps_serial_is_open(&app->gps_serial))
                {
                    // Try to auto-connect immediately
                    char ports[16][256];
                    int port_count;
                    if (gps_serial_list_ports(ports, 16, &port_count) && port_count > 0)
                    {
                        if (gps_serial_open(&app->gps_serial, ports[0], 9600))
                        {
                            app->gps_data.status = GPS_STATUS_CONNECTED;
                            strcpy(app->gps_data.port_name, ports[0]);
                            app->gps_data.baud_rate = 9600;
                            connection_history_add(app, ports[0], 9600);
                            snprintf(app->connection_status_text, sizeof(app->connection_status_text),
                                     "Auto-connected to %s", ports[0]);
                        }
                    }
                }
            }
            igEndMenu();
        }

        if (igBeginMenu("View", true))
        {
            if (igMenuItem_Bool("Dark Theme", NULL, !app->use_light_theme, true))
            {
                app->use_light_theme = false;
                setup_imgui_style(false);
            }
            if (igMenuItem_Bool("Light Theme", NULL, app->use_light_theme, true))
            {
                app->use_light_theme = true;
                setup_imgui_style(true);
            }
            igEndMenu();
        }

        // Tools menu
        if (igBeginMenu("Tools", true))
        {
            if (igMenuItem_Bool("Start Recording", NULL, false, !app->map_system.track.recording))
            {
                map_system_start_recording(&app->map_system);
            }
            if (igMenuItem_Bool("Stop Recording", NULL, false, app->map_system.track.recording))
            {
                map_system_stop_recording(&app->map_system);
            }
            igSeparator();
            if (igMenuItem_Bool("Clear Track", NULL, false, app->map_system.track.point_count > 0))
            {
                map_system_clear_track(&app->map_system);
            }
            if (igMenuItem_Bool("Export GPX...", NULL, false, app->map_system.track.point_count > 0))
            {
                app->show_gpx_export_dialog = true;
            }
            if (igMenuItem_Bool("Import GPX...", NULL, false, true))
            {
                app->show_gpx_import_dialog = true;
            }
            if (igMenuItem_Bool("Export CSV (Analysis)...", NULL, false, app->map_system.track.point_count > 0))
            {
                app->show_csv_export_dialog = true;
            }
            igSeparator();
            if (igMenuItem_Bool("Start Logging", NULL, false, !app->gps_data.logging_enabled))
            {
                char filename[256];
                time_t t = time(NULL);
                struct tm *tm_info = localtime(&t);
                strftime(filename, sizeof(filename), "data/gps_log_%Y%m%d_%H%M%S.nmea", tm_info);
                gps_data_start_logging(&app->gps_data, filename);
            }
            if (igMenuItem_Bool("Stop Logging", NULL, false, app->gps_data.logging_enabled))
            {
                gps_data_stop_logging(&app->gps_data);
            }
            igSeparator();
            bool show_demo = app->show_demo_window;
            if (igMenuItem_Bool("Show Demo Window", NULL, show_demo, true))
            {
                app->show_demo_window = !app->show_demo_window;
            }
            igEndMenu();
        }

        // Help menu
        if (igBeginMenu("Help", true))
        {
            // Show help/about /src/docs/gpc_help.md içeriğine bağlanabilir
            if (igMenuItem_Bool("Help", NULL, false, true))
            {
                app->show_help_window = true;
            }
            // About dialog Show /src/docs/gpc_about.md içeriğine bağlanabilir
            if (igMenuItem_Bool("About", NULL, false, true))
            {
                app->show_about_window = true;
            }

            igSeparator();
            if (igBeginMenu("Keyboard Shortcuts (Tips)", true))
            {
                igMenuItem_Bool("Open Connection Dialog", "Cmd/Ctrl+K", false, false);
                igMenuItem_Bool("Open Help", "F1", false, false);
                igMenuItem_Bool("Map Zoom", "Mouse Wheel", false, false);
                igMenuItem_Bool("Map Pan", "Middle Mouse + Drag", false, false);
                igMenuItem_Bool("Add Waypoint", "Right Click (Map)", false, false);
                igMenuItem_Bool("Send Console Command", "Enter", false, false);
                igEndMenu();
            }
            igEndMenu();
        }

        // Right side icons would go here

        igEndMenuBar();
    }
}

void render_telemetry_window(app_state_t *app)
{
    // Set default size and position for first use
    igSetNextWindowSize((ImVec2){LEFT_PANEL_WIDTH, 400}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){10, 30}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("Telemetry", NULL, ImGuiWindowFlags_None))
    {
        render_telemetry_panel(app);
    }
    igEnd();
}

void render_telemetry_panel(app_state_t *app)
{
    gps_data_t *gps = &app->gps_data;

    // Connection status
    igText("Status: %s", gps_status_to_string(gps->status));
    igSameLine(0, -1);
    if (gps->status == GPS_STATUS_CONNECTED)
    {
        igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "●");
    }
    else
    {
        igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "●");
    }

    igSeparator();

    // Position
    igText("Position:");
    if (gps->position_valid)
    {
        igText("  Lat: %.6f°", gps->latitude);
        igText("  Lon: %.6f°", gps->longitude);
        igText("  Alt: %.1f m", gps->altitude);
    }
    else
    {
        igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "  No position fix");
    }

    igSeparator();

    // Time
    igText("Time:");
    if (gps->time_valid)
    {
        igText("  UTC: %02d:%02d:%02d",
               gps->time.hours, gps->time.minutes, gps->time.seconds);
        igText("  Date: %02d/%02d/%04d",
               gps->date.day, gps->date.month,
               gps->date.year < 100 ? gps->date.year + 2000 : gps->date.year);
    }
    else
    {
        igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "  No time fix");
    }

    igSeparator();

    // Motion
    igText("Motion:");
    if (gps->motion_valid)
    {
        igText("  Speed: %.1f km/h", gps->speed_kmh);
        igText("  Course: %.1f°", gps->course);
    }
    else
    {
        igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "  No motion data");
    }

    igSeparator();

    // Fix quality
    igText("Fix Quality: %s", gps_fix_quality_to_string(gps->fix_quality));
    igText("Satellites: %d/%d", gps->satellites_used, gps->satellites_visible);
    igText("HDOP: %.2f", gps->hdop);
    igText("VDOP: %.2f", gps->vdop);
    igText("PDOP: %.2f", gps->pdop);

    igSeparator();

    // Statistics
    igText("Statistics:");
    igText("  Total: %lu", gps->total_sentences);
    igText("  Valid: %lu", gps->valid_sentences);
    igText("  Invalid: %lu", gps->invalid_sentences);
    if (gps->total_sentences > 0)
    {
        float success_rate = (float)gps->valid_sentences / gps->total_sentences * 100.0f;
        igText("  Success: %.1f%%", success_rate);
    }

    igSeparator();

    // Logging status
    igText("Logging: %s", gps->logging_enabled ? "ON" : "OFF");
    if (gps->logging_enabled)
    {
        igText("File: %s", gps->log_filename);
    }
}

void render_map_window(app_state_t *app)
{
    // Set default size and position for main map window
    igSetNextWindowSize((ImVec2){600, 400}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 20, 30}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("Map", NULL, ImGuiWindowFlags_None))
    {
        render_enhanced_map_panel(app);
    }
    igEnd();
}

void render_enhanced_map_panel(app_state_t *app)
{
    gps_data_t *gps = &app->gps_data;
    map_system_t *map = &app->map_system;
    bool has_track_points = (map->track.point_count > 0);
    static bool show_poi_markers = true;
    static char poi_filter_text[64] = "";
    static bool show_poi_detail_popup = false;
    static poi_item_t selected_poi = {};

    // Map controls
    igText("Map Controls:");
    igSameLine(0, 20);

    bool recording = map->track.recording;
    if (igCheckbox("Recording", &recording))
    {
        if (recording)
        {
            map_system_start_recording(map);
        }
        else
        {
            map_system_stop_recording(map);
        }
    }
    ui_show_tooltip("Track kaydini ac/kapat");

    igSameLine(0, 20);
    if (!has_track_points)
        igPushStyleVar_Float(ImGuiStyleVar_Alpha, 0.45f);
    if (igButton("Clear Track", (ImVec2){0, 0}) && has_track_points)
    {
        map_system_clear_track(map);
    }
    if (!has_track_points)
        igPopStyleVar(1);
    ui_show_tooltip(has_track_points ? "Kayitli izleri temizler" : "Temizlenecek track yok");

    igSameLine(0, 20);
    if (!has_track_points)
        igPushStyleVar_Float(ImGuiStyleVar_Alpha, 0.45f);
    if (igButton("Fit Track", (ImVec2){0, 0}) && has_track_points)
    {
        map_system_zoom_to_fit_track(map);
    }
    if (!has_track_points)
        igPopStyleVar(1);
    ui_show_tooltip(has_track_points ? "Tum track'i gorunecek sekilde zoom ayarlar" : "Zoom to fit icin track gerekli");

    igSameLine(0, 20);
    if (igCheckbox("Auto Center", &map->view.auto_center))
    {
        // Toggle handled by checkbox
    }
    ui_show_tooltip("GPS konumu geldiginde haritayi otomatik merkeze alir");

    igSameLine(0, 20);
    igCheckbox("Offline Only", &map->view.offline_only);
    ui_show_tooltip("Acilinca online tile denemelerini devre disi birakir");

    igSameLine(0, 20);
    igCheckbox("Prefer MBTiles", &map->view.prefer_mbtiles);
    ui_show_tooltip("Tile yuklemede MBTiles kaynagini once dener");

    // Zoom controls
    igText("Zoom: %.2fx", map->view.zoom_level);
    igSameLine(0, 20);
    igText("Center: %.5f, %.5f", map->view.center_lat, map->view.center_lon);
    igSameLine(0, 20);
    if (igButton("Zoom In", (ImVec2){0, 0}))
    {
        map_system_set_zoom(map, map->view.zoom_level * 1.5);
    }
    ui_show_tooltip("Haritayi yakinlastir");
    igSameLine(0, 10);
    if (igButton("Zoom Out", (ImVec2){0, 0}))
    {
        map_system_set_zoom(map, map->view.zoom_level / 1.5);
    }
    ui_show_tooltip("Haritayi uzaklastir");
    igSameLine(0, 10);
    if (igButton("Reset Zoom", (ImVec2){0, 0}))
    {
        map_system_set_zoom(map, 10.0); // 1.0 yerine 10.0
    }
    ui_show_tooltip("Zoom seviyesini varsayilana dondurur");

    // Track statistics
    if (map->track.point_count > 0)
    {
        igSeparator();
        igText("Track Info:");
        igText("  Points: %d", map->track.point_count);
        igText("  Distance: %.2f km", map->track.total_distance / 1000.0);
        igText("  Max Speed: %.1f km/h", map->track.max_speed);

        if (map->track.start_time > 0)
        {
            time_t duration = time(NULL) - map->track.start_time;
            igText("  Duration: %02ld:%02ld:%02ld",
                   duration / 3600, (duration % 3600) / 60, duration % 60);
        }
    }

    igSeparator();
    igText("Waypoints: %d", map->waypoints.waypoint_count);
    if (map->waypoints.waypoint_count > 0)
    {
        int delete_index = -1;
        for (int i = 0; i < map->waypoints.waypoint_count; i++)
        {
            waypoint_t *wp = &map->waypoints.waypoints[i];
            if (!wp->active)
                continue;

            igText("%d) %s", i + 1, wp->name);
            igSameLine(0, 8);

            char go_id[32];
            snprintf(go_id, sizeof(go_id), "Go##wp_go_%d", i);
            if (igButton(go_id, (ImVec2){40, 0}))
            {
                map_system_set_center(map, wp->latitude, wp->longitude);
                map->view.auto_center = false;
            }

            igSameLine(0, 6);
            char del_id[32];
            snprintf(del_id, sizeof(del_id), "Del##wp_del_%d", i);
            if (igButton(del_id, (ImVec2){40, 0}))
            {
                delete_index = i;
            }
        }

        if (delete_index >= 0)
        {
            map_system_remove_waypoint(map, delete_index);
        }
    }

    igSeparator();

    // Map canvas
    ImVec2 canvas_pos, canvas_size;
    igGetCursorScreenPos(&canvas_pos);
    igGetContentRegionAvail(&canvas_size);

    if (canvas_size.x < 50.0f)
        canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f)
        canvas_size.y = 50.0f;

    ImDrawList *draw_list = igGetWindowDrawList();

    // Background
    ImU32 bg_color = igGetColorU32_Col(ImGuiCol_ChildBg, 1.0f);
    ImDrawList_AddRectFilled(draw_list, canvas_pos,
                             (ImVec2){canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y},
                             bg_color, 4.0f, ImDrawFlags_RoundCornersAll);

    // Border
    ImU32 border_color = igGetColorU32_Col(ImGuiCol_Border, 1.0f);
    ImDrawList_AddRect(draw_list, canvas_pos,
                       (ImVec2){canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y},
                       border_color, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);

    // Tile-aware background (MVP): show local tile coverage using Web-Mercator tile math
    int tile_zoom = map_tiles_resolve_zoom_level(map->view.zoom_level);
    tile_range_t tile_range = map_view_visible_tile_range(&map->view, canvas_size.x, canvas_size.y, tile_zoom);

    int focus_tx = 0;
    int focus_ty = 0;
    map_lat_lon_to_tile(map->view.center_lat, map->view.center_lon, tile_zoom, &focus_tx, &focus_ty);
    online_tile_queue_set_focus(tile_zoom, focus_tx, focus_ty);

    bool rendered_tile_blocks = false;

    if (tile_range.valid)
    {
        int tile_count_x = tile_range.max_x - tile_range.min_x + 1;
        int tile_count_y = tile_range.max_y - tile_range.min_y + 1;
        int tile_count = tile_count_x * tile_count_y;

        // Safety cap for MVP rendering workload
        if (tile_count > 0 && tile_count <= 400)
        {
            for (int tx = tile_range.min_x; tx <= tile_range.max_x; tx++)
            {
                for (int ty = tile_range.min_y; ty <= tile_range.max_y; ty++)
                {
                    double lat_nw, lon_nw, lat_se, lon_se;
                    map_tile_to_lat_lon(tx, ty, tile_zoom, &lat_nw, &lon_nw);
                    map_tile_to_lat_lon(tx + 1, ty + 1, tile_zoom, &lat_se, &lon_se);

                    float sx1, sy1, sx2, sy2;
                    map_lat_lon_to_screen(&map->view, lat_nw, lon_nw, canvas_size.x, canvas_size.y, &sx1, &sy1);
                    map_lat_lon_to_screen(&map->view, lat_se, lon_se, canvas_size.x, canvas_size.y, &sx2, &sy2);

                    float left = canvas_pos.x + fminf(sx1, sx2);
                    float right = canvas_pos.x + fmaxf(sx1, sx2);
                    float top = canvas_pos.y + fminf(sy1, sy2);
                    float bottom = canvas_pos.y + fmaxf(sy1, sy2);

                    GLuint texture_id = map_tile_texture_get_or_load("data/map_tiles",
                                                                     "data/map_tiles.mbtiles",
                                                                     map->view.prefer_mbtiles,
                                                                     map->view.offline_only,
                                                                     tile_zoom,
                                                                     tx,
                                                                     ty);
                    if (texture_id != 0)
                    {
                        ImTextureRef tex_ref = {NULL, (ImTextureID)texture_id};
                        ImDrawList_AddImage(draw_list,
                                            tex_ref,
                                            (ImVec2){left, top},
                                            (ImVec2){right, bottom},
                                            (ImVec2){0.0f, 0.0f},
                                            (ImVec2){1.0f, 1.0f},
                                            igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}));
                    }
                    else
                    {
                        bool has_mbtiles_tile = map_mbtiles_has_tile("data/map_tiles.mbtiles", tile_zoom, tx, ty);
                        ImU32 tile_fill = has_mbtiles_tile
                                              ? igGetColorU32_Vec4((ImVec4){0.24f, 0.14f, 0.34f, 0.70f})
                                              : igGetColorU32_Vec4((ImVec4){0.18f, 0.18f, 0.18f, 0.65f});
                        ImU32 tile_border = has_mbtiles_tile
                                                ? igGetColorU32_Vec4((ImVec4){0.62f, 0.40f, 0.92f, 0.45f})
                                                : igGetColorU32_Vec4((ImVec4){0.35f, 0.35f, 0.35f, 0.35f});
                        ImDrawList_AddRectFilled(draw_list, (ImVec2){left, top}, (ImVec2){right, bottom},
                                                 tile_fill, 0.0f, ImDrawFlags_None);
                        ImDrawList_AddRect(draw_list, (ImVec2){left, top}, (ImVec2){right, bottom},
                                           tile_border, 0.0f, ImDrawFlags_None, 1.0f);
                    }
                    rendered_tile_blocks = true;
                }
            }
        }
    }

    int poi_in_view = 0;
    enum
    {
        POI_RENDER_MAX = 128
    };
    poi_item_t poi_items[POI_RENDER_MAX];
    int poi_render_count = 0;
    if (tile_range.valid)
    {
        double lat_nw, lon_nw, lat_se, lon_se;
        map_tile_to_lat_lon(tile_range.min_x, tile_range.min_y, tile_zoom, &lat_nw, &lon_nw);
        map_tile_to_lat_lon(tile_range.max_x + 1, tile_range.max_y + 1, tile_zoom, &lat_se, &lon_se);

        double min_lat = fmin(lat_nw, lat_se);
        double max_lat = fmax(lat_nw, lat_se);
        double min_lon = fmin(lon_nw, lon_se);
        double max_lon = fmax(lon_nw, lon_se);

        poi_in_view = poi_db_count_bbox("data/map_db.sqlite", min_lat, min_lon, max_lat, max_lon);
        if (show_poi_markers)
        {
            poi_render_count = poi_db_load_bbox("data/map_db.sqlite",
                                                min_lat,
                                                min_lon,
                                                max_lat,
                                                max_lon,
                                                poi_items,
                                                POI_RENDER_MAX);
        }
    }

    // POI Controls
    igText("POI Controls:");
    igSameLine(0, 12);
    igCheckbox("Show POI", &show_poi_markers);
    ui_show_tooltip("POI markerlarini goster/gizle");
    igSameLine(0, 12);
    igSetNextItemWidth(180.0f);
    igInputText("POI Filter", poi_filter_text, sizeof(poi_filter_text), ImGuiInputTextFlags_None, NULL, NULL);
    ui_show_tooltip("Isme gore filtre (case-insensitive)");

    // Fallback grid when no tile blocks are rendered
    if (!rendered_tile_blocks)
    {
        ImU32 grid_color = igGetColorU32_Vec4((ImVec4){0.3f, 0.3f, 0.3f, 0.5f});
        for (int i = 1; i < 4; i++)
        {
            float x = canvas_pos.x + (canvas_size.x * i / 4);
            float y = canvas_pos.y + (canvas_size.y * i / 4);

            // Vertical lines
            ImDrawList_AddLine(draw_list, (ImVec2){x, canvas_pos.y},
                               (ImVec2){x, canvas_pos.y + canvas_size.y}, grid_color, 1.0f);
            // Horizontal lines
            ImDrawList_AddLine(draw_list, (ImVec2){canvas_pos.x, y},
                               (ImVec2){canvas_pos.x + canvas_size.x, y}, grid_color, 1.0f);
        }
    }

    // Lightweight debug text for tile state
    char tile_debug[360];
    int completion_drained = online_completion_drain_and_invalidate_cache();
    int online_pending = 0;
    uint64_t online_processed_total = 0;
    uint64_t online_enqueued_total = 0;
    uint64_t online_success_total = 0;
    uint64_t online_fail_total = 0;
    online_tile_queue_get_stats(&online_pending,
                                &online_processed_total,
                                &online_enqueued_total,
                                &online_success_total,
                                &online_fail_total);

    snprintf(tile_debug, sizeof(tile_debug), "Tile Z:%d [%d..%d, %d..%d] H:%llu M:%llu F:%llu MB:%llu/%llu OA:%llu OQ:%llu OQP:%d OP:%llu OE:%llu OD:%llu OX:%llu OFC:%llu OFU:%llu OAG:%llu ORR:%llu ONC:%llu/%llu/%d OWI:%u OWD:%u OC:%llu/%llu/%d OS:%llu OF:%llu[%llu/%llu/%llu] POI:%d",
             tile_zoom,
             tile_range.min_x,
             tile_range.max_x,
             tile_range.min_y,
             tile_range.max_y,
             (unsigned long long)g_tile_cache_hits,
             (unsigned long long)g_tile_cache_misses,
             (unsigned long long)g_tile_cache_failures,
             (unsigned long long)g_tile_cache_mbtiles_reads,
             (unsigned long long)g_tile_cache_mbtiles_decodes,
             (unsigned long long)g_tile_cache_online_attempts,
             (unsigned long long)online_enqueued_total,
             online_pending,
             (unsigned long long)online_processed_total,
             (unsigned long long)g_online_queue_enqueued,
             (unsigned long long)g_online_queue_dedup_skips,
             (unsigned long long)g_online_queue_full_skips,
             (unsigned long long)g_online_queue_fail_cache_skips,
             (unsigned long long)g_online_queue_focus_updates,
             (unsigned long long)g_online_queue_aged_picks,
             (unsigned long long)g_online_queue_retry_requeues,
             (unsigned long long)g_online_net_cooldown_events,
             (unsigned long long)g_online_net_cooldown_ms_total,
             g_online_download_fail_streak,
             g_online_worker_interval_last_ms,
             g_online_worker_idle_delay_last_ms,
             (unsigned long long)g_online_completion_enqueued,
             (unsigned long long)g_online_completion_dropped,
             completion_drained,
             (unsigned long long)online_success_total,
             (unsigned long long)online_fail_total,
             (unsigned long long)g_online_fetch_fail_io,
             (unsigned long long)g_online_fetch_fail_download,
             (unsigned long long)g_online_fetch_fail_convert,
             poi_in_view);
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2){canvas_pos.x + 10, canvas_pos.y + canvas_size.y - 20},
                            igGetColorU32_Vec4((ImVec4){0.75f, 0.75f, 0.75f, 0.9f}),
                            tile_debug, NULL);

    // @onkanatTODO:  track çok küçük ise oluyor scale No screen space !!check src/include/gps_map.c  Draw track history
    if (map->track.point_count > 1 && map->view.show_track)
    {
        ImU32 track_color = igGetColorU32_Vec4((ImVec4){0.0f, 0.8f, 1.0f, 0.8f});
        int start_idx = map->track.point_count < MAX_TRACK_POINTS ? 0 : map->track.current_index;

        for (int i = 1; i < map->track.point_count; i++)
        {
            int prev_idx = (start_idx + i - 1) % MAX_TRACK_POINTS;
            int curr_idx = (start_idx + i) % MAX_TRACK_POINTS;

            const track_point_t *prev_point = &map->track.points[prev_idx];
            const track_point_t *curr_point = &map->track.points[curr_idx];
            if (!prev_point->valid || !curr_point->valid)
                continue;

            float prev_x, prev_y, curr_x, curr_y;
            map_lat_lon_to_screen(&map->view, prev_point->latitude, prev_point->longitude,
                                  canvas_size.x, canvas_size.y, &prev_x, &prev_y);
            map_lat_lon_to_screen(&map->view, curr_point->latitude, curr_point->longitude,
                                  canvas_size.x, canvas_size.y, &curr_x, &curr_y);

            prev_x += canvas_pos.x;
            prev_y += canvas_pos.y;
            curr_x += canvas_pos.x;
            curr_y += canvas_pos.y;

            ImDrawList_AddLine(draw_list, (ImVec2){prev_x, prev_y}, (ImVec2){curr_x, curr_y},
                               track_color, map->view.track_width);

            // Her 20 noktada bir küçük işaret bırak
            if ((i % 20) == 0)
            {
                ImDrawList_AddCircleFilled(draw_list, (ImVec2){curr_x, curr_y}, 2.0f, track_color, 8);
            }
        }
    }

    // Draw waypoints
    if (map->view.show_waypoints && map->waypoints.waypoint_count > 0)
    {
        for (int i = 0; i < map->waypoints.waypoint_count; i++)
        {
            const waypoint_t *wp = &map->waypoints.waypoints[i];
            if (!wp->active)
                continue;

            float wp_x, wp_y;
            map_lat_lon_to_screen(&map->view, wp->latitude, wp->longitude,
                                  canvas_size.x, canvas_size.y, &wp_x, &wp_y);

            wp_x += canvas_pos.x;
            wp_y += canvas_pos.y;

            ImU32 outer_color = igGetColorU32_Vec4((ImVec4){0.1f, 0.1f, 0.1f, 0.9f});
            ImU32 inner_color = igGetColorU32_Vec4((ImVec4){0.05f, 0.75f, 1.0f, 1.0f});
            ImDrawList_AddCircleFilled(draw_list, (ImVec2){wp_x, wp_y}, 6.0f, outer_color, 16);
            ImDrawList_AddCircleFilled(draw_list, (ImVec2){wp_x, wp_y}, 4.0f, inner_color, 16);

            ImDrawList_AddText_Vec2(draw_list,
                                    (ImVec2){wp_x + 8.0f, wp_y - 8.0f},
                                    igGetColorU32_Col(ImGuiCol_Text, 1.0f),
                                    wp->name, NULL);
        }
    }

    // Draw POI markers from map_db.sqlite (MVP)
    bool poi_right_click_consumed = false;
    if (show_poi_markers && poi_render_count > 0)
    {
        ImVec2 mouse_pos;
        igGetMousePos(&mouse_pos);
        bool mouse_in_canvas = (mouse_pos.x >= canvas_pos.x && mouse_pos.x <= canvas_pos.x + canvas_size.x &&
                                mouse_pos.y >= canvas_pos.y && mouse_pos.y <= canvas_pos.y + canvas_size.y);

        int hovered_poi_index = -1;
        float hovered_dist_sq = 999999.0f;

        for (int i = 0; i < poi_render_count; i++)
        {
            const poi_item_t *poi = &poi_items[i];

            float poi_x, poi_y;
            map_lat_lon_to_screen(&map->view, poi->latitude, poi->longitude,
                                  canvas_size.x, canvas_size.y, &poi_x, &poi_y);

            poi_x += canvas_pos.x;
            poi_y += canvas_pos.y;

            if (poi_x < canvas_pos.x - 8.0f || poi_x > canvas_pos.x + canvas_size.x + 8.0f ||
                poi_y < canvas_pos.y - 8.0f || poi_y > canvas_pos.y + canvas_size.y + 8.0f)
            {
                continue;
            }

            ImU32 poi_outer = igGetColorU32_Vec4((ImVec4){0.06f, 0.06f, 0.06f, 0.95f});
            ImU32 poi_inner = igGetColorU32_Vec4((ImVec4){0.95f, 0.35f, 0.15f, 1.0f});
            const char *poi_name = (poi->name[0] != '\0') ? poi->name : "(Unnamed POI)";
            if (!str_contains_case_insensitive(poi_name, poi_filter_text))
            {
                continue;
            }

            ImDrawList_AddCircleFilled(draw_list, (ImVec2){poi_x, poi_y}, 4.5f, poi_outer, 12);
            ImDrawList_AddCircleFilled(draw_list, (ImVec2){poi_x, poi_y}, 3.0f, poi_inner, 12);

            float dx = mouse_pos.x - poi_x;
            float dy = mouse_pos.y - poi_y;
            float d2 = dx * dx + dy * dy;
            if (d2 < 49.0f && d2 < hovered_dist_sq)
            {
                hovered_dist_sq = d2;
                hovered_poi_index = i;
            }
        }

        if (hovered_poi_index >= 0)
        {
            const poi_item_t *poi = &poi_items[hovered_poi_index];
            const char *poi_name = (poi->name[0] != '\0') ? poi->name : "(Unnamed POI)";
            igSetTooltip("POI: %s\nLat: %.6f\nLon: %.6f\nSol tik: merkeze al",
                         poi_name,
                         poi->latitude,
                         poi->longitude);

            if (mouse_in_canvas && igIsMouseClicked_Bool(ImGuiMouseButton_Left, false))
            {
                map_system_set_center(map, poi->latitude, poi->longitude);
                map->view.auto_center = false;
            }

            if (mouse_in_canvas && igIsMouseClicked_Bool(ImGuiMouseButton_Right, false))
            {
                selected_poi = *poi;
                show_poi_detail_popup = true;
                poi_right_click_consumed = true;
                igOpenPopup_Str("POI Details", ImGuiPopupFlags_None);
            }
        }
    }

    if (show_poi_detail_popup && igBeginPopupModal("POI Details", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const char *poi_name = (selected_poi.name[0] != '\0') ? selected_poi.name : "(Unnamed POI)";
        igText("POI: %s", poi_name);
        igText("Lat: %.6f", selected_poi.latitude);
        igText("Lon: %.6f", selected_poi.longitude);
        if (selected_poi.category[0] != '\0')
        {
            igText("Category: %s", selected_poi.category);
        }
        igSeparator();
        if (igButton("Center", (ImVec2){80, 0}))
        {
            map_system_set_center(map, selected_poi.latitude, selected_poi.longitude);
            map->view.auto_center = false;
        }
        igSameLine(0, 10);
        if (igButton("Close", (ImVec2){80, 0}))
        {
            show_poi_detail_popup = false;
            igCloseCurrentPopup();
        }
        igEndPopup();
    }

    // Draw current position
    if (gps->position_valid)
    {
        float pos_x, pos_y;
        map_lat_lon_to_screen(&map->view, gps->latitude, gps->longitude,
                              canvas_size.x, canvas_size.y, &pos_x, &pos_y);

        pos_x += canvas_pos.x;
        pos_y += canvas_pos.y;

        // Current position marker (larger circle)
        ImU32 pos_color = igGetColorU32_Vec4((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}); // Red
        ImDrawList_AddCircleFilled(draw_list, (ImVec2){pos_x, pos_y}, 8.0f, pos_color, 16);

        // Direction indicator
        if (gps->motion_valid && gps->speed_kmh > 1.0f)
        {
            float course_rad = gps->course * M_PI / 180.0f;
            float arrow_len = 15.0f;
            float end_x = pos_x + arrow_len * sinf(course_rad);
            float end_y = pos_y - arrow_len * cosf(course_rad);

            ImU32 arrow_color = igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 0.0f, 1.0f}); // Yellow
            ImDrawList_AddLine(draw_list, (ImVec2){pos_x, pos_y},
                               (ImVec2){end_x, end_y}, arrow_color, 3.0f);
        }

        // Position text overlay
        char pos_text[64];
        snprintf(pos_text, sizeof(pos_text), "%.6f, %.6f", gps->latitude, gps->longitude);
        ImDrawList_AddText_Vec2(draw_list,
                                (ImVec2){canvas_pos.x + 10, canvas_pos.y + 10},
                                igGetColorU32_Col(ImGuiCol_Text, 1.0f),
                                pos_text, NULL);

        // Speed and course overlay
        if (gps->motion_valid)
        {
            char motion_text[64];
            snprintf(motion_text, sizeof(motion_text), "%.1f km/h, %.0f°",
                     gps->speed_kmh, gps->course);
            ImDrawList_AddText_Vec2(draw_list,
                                    (ImVec2){canvas_pos.x + 10, canvas_pos.y + 30},
                                    igGetColorU32_Col(ImGuiCol_Text, 1.0f),
                                    motion_text, NULL);
        }
    }
    else
    {
        // No GPS fix message
        const char *no_fix_text = "No GPS fix available";
        ImVec2 text_size;
        igCalcTextSize(&text_size, no_fix_text, NULL, false, -1.0f);
        ImVec2 text_pos = {
            canvas_pos.x + (canvas_size.x - text_size.x) * 0.5f,
            canvas_pos.y + (canvas_size.y - text_size.y) * 0.5f};
        ImDrawList_AddText_Vec2(draw_list, text_pos,
                                igGetColorU32_Vec4((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}),
                                no_fix_text, NULL);
    }

    // Handle mouse interaction
    if (igInvisibleButton("map_canvas", canvas_size, ImGuiButtonFlags_None))
    {
        // Handle click events (future: add waypoints, etc.)
    }

    if (igIsItemHovered(ImGuiHoveredFlags_None))
    {
        ImVec2 mouse_pos;
        igGetMousePos(&mouse_pos);

        float local_x = mouse_pos.x - canvas_pos.x;
        float local_y = mouse_pos.y - canvas_pos.y;

        double hover_lat, hover_lon;
        map_screen_to_lat_lon(&map->view, local_x, local_y,
                              canvas_size.x, canvas_size.y,
                              &hover_lat, &hover_lon);

        char hover_text[96];
        snprintf(hover_text, sizeof(hover_text), "Mouse: %.6f, %.6f", hover_lat, hover_lon);
        ImVec2 hover_size;
        igCalcTextSize(&hover_size, hover_text, NULL, false, -1.0f);

        ImVec2 text_pos = {canvas_pos.x + canvas_size.x - hover_size.x - 14.0f, canvas_pos.y + 10.0f};
        ImU32 hover_bg = igGetColorU32_Vec4((ImVec4){0.0f, 0.0f, 0.0f, 0.55f});
        ImDrawList_AddRectFilled(draw_list,
                                 (ImVec2){text_pos.x - 4.0f, text_pos.y - 2.0f},
                                 (ImVec2){text_pos.x + hover_size.x + 4.0f, text_pos.y + hover_size.y + 2.0f},
                                 hover_bg, 3.0f, ImDrawFlags_RoundCornersAll);
        ImDrawList_AddText_Vec2(draw_list, text_pos,
                                igGetColorU32_Vec4((ImVec4){0.9f, 0.95f, 1.0f, 1.0f}),
                                hover_text, NULL);

        igSetTooltip("Sag tik: Waypoint ekle | Orta tus + surukle: Pan | Mouse wheel: Zoom");
    }

    static double pending_wp_lat = 0.0;
    static double pending_wp_lon = 0.0;
    static char pending_wp_name[64] = "";

    // Right-click to open waypoint add popup
    if (igIsItemHovered(ImGuiHoveredFlags_None) &&
        igIsMouseClicked_Bool(ImGuiMouseButton_Right, false) &&
        !poi_right_click_consumed)
    {
        ImVec2 mouse_pos;
        igGetMousePos(&mouse_pos);

        float local_x = mouse_pos.x - canvas_pos.x;
        float local_y = mouse_pos.y - canvas_pos.y;

        double wp_lat, wp_lon;
        map_screen_to_lat_lon(&map->view, local_x, local_y,
                              canvas_size.x, canvas_size.y,
                              &wp_lat, &wp_lon);

        pending_wp_lat = wp_lat;
        pending_wp_lon = wp_lon;
        snprintf(pending_wp_name, sizeof(pending_wp_name), "WP-%d", map->waypoints.waypoint_count + 1);
        igOpenPopup_Str("Add Waypoint", ImGuiPopupFlags_None);
    }

    if (igBeginPopupModal("Add Waypoint", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        igText("Waypoint coordinates:");
        igText("Lat: %.6f", pending_wp_lat);
        igText("Lon: %.6f", pending_wp_lon);
        igSeparator();

        igText("Name:");
        igInputText("##wp_name", pending_wp_name, sizeof(pending_wp_name),
                    ImGuiInputTextFlags_None, NULL, NULL);

        igSeparator();
        if (igButton("Add", (ImVec2){80, 0}))
        {
            map_system_add_waypoint(map, pending_wp_lat, pending_wp_lon, pending_wp_name);
            igCloseCurrentPopup();
        }

        igSameLine(0, 10);
        if (igButton("Cancel", (ImVec2){80, 0}))
        {
            igCloseCurrentPopup();
        }

        igEndPopup();
    }

    // Mouse wheel zoom
    ImGuiIO *io = cImGui_GetIO();
    if (igIsItemHovered(ImGuiHoveredFlags_None) && io->MouseWheel != 0.0f)
    {
        double zoom_factor = (io->MouseWheel > 0) ? 1.2 : (1.0 / 1.2);
        map_system_set_zoom(map, map->view.zoom_level * zoom_factor);
    }

    // Pan with middle mouse button
    if (igIsItemActive() && igIsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
    {
        ImVec2 delta;
        igGetMouseDragDelta(&delta, ImGuiMouseButton_Middle, 0.0f);

        // Ekran deltası -> lat/lon farkı (ters dönüşüm)
        double lat0, lon0, lat1, lon1;
        map_screen_to_lat_lon(&map->view, 0.0f, 0.0f, canvas_size.x, canvas_size.y, &lat0, &lon0);
        map_screen_to_lat_lon(&map->view, -delta.x, -delta.y, canvas_size.x, canvas_size.y, &lat1, &lon1);

        map_system_set_center(map,
                              map->view.center_lat + (lat1 - lat0),
                              map->view.center_lon + (lon1 - lon0));
    }
}

void render_polar_window(app_state_t *app)
{
    // Set default size for sky plot window
    igSetNextWindowSize((ImVec2){350, 350}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 630, 30}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("Sky Plot", NULL, ImGuiWindowFlags_None))
    {
        render_polar_view_panel(app);
    }
    igEnd();
}

void render_polar_view_panel(app_state_t *app)
{
    polar_view_t *polar = &app->polar_view;
    gps_data_t *gps = &app->gps_data;

    // Update polar view with current GPS data
    polar_view_update(polar, gps);

    // Statistics
    igText("Sky Plot - Satellite Positions");
    igText("Visible: %d | Used: %d", polar->satellites_visible, polar->satellites_used);
    if (polar->satellites_visible > 0)
    {
        igText("Average SNR: %.1f dB | Max SNR: %.1f dB",
               polar->average_snr, polar->max_snr);
    }

    igSeparator();

    // Polar plot canvas
    ImVec2 canvas_pos, canvas_size;
    igGetCursorScreenPos(&canvas_pos);
    igGetContentRegionAvail(&canvas_size);

    // Make it square
    float size = fminf(canvas_size.x, canvas_size.y) - 20.0f;
    if (size < 200.0f)
        size = 200.0f;

    float center_x = canvas_pos.x + size * 0.5f;
    float center_y = canvas_pos.y + size * 0.5f;
    float radius = size * 0.45f;

    polar_view_set_size(polar, center_x, center_y, radius);

    ImDrawList *draw_list = igGetWindowDrawList();

    // Background circle
    ImU32 bg_color = igGetColorU32_Col(ImGuiCol_ChildBg, 1.00f);
    ImDrawList_AddCircleFilled(draw_list, (ImVec2){center_x, center_y}, radius, bg_color, 64);

    // Grid circles (elevation)
    ImU32 grid_color = igGetColorU32_Vec4((ImVec4){0.4f, 0.4f, 0.4f, 0.6f});
    for (int elev = 30; elev <= 90; elev += 30)
    {
        float r = radius * (1.0f - elev / 90.0f);
        ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, r, grid_color, 32, 1.0f);
    }

    // Grid lines (azimuth)
    for (int az = 0; az < 360; az += 30)
    {
        float angle_rad = (az - 90.0f) * M_PI / 180.0f;
        float end_x = center_x + radius * cosf(angle_rad);
        float end_y = center_y + radius * sinf(angle_rad);
        ImDrawList_AddLine(draw_list, (ImVec2){center_x, center_y},
                           (ImVec2){end_x, end_y}, grid_color, 1.0f);
    }

    // Compass labels
    ImU32 text_color = igGetColorU32_Col(ImGuiCol_Text, 1.0f);
    float label_radius = radius + 15.0f;

    const char *compass_labels[] = {"N", "E", "S", "W"};
    for (int i = 0; i < 4; i++)
    {
        float angle_rad = (i * 90.0f - 90.0f) * M_PI / 180.0f;
        float label_x = center_x + label_radius * cosf(angle_rad) - 6.0f;
        float label_y = center_y + label_radius * sinf(angle_rad) - 6.0f;
        ImDrawList_AddText_Vec2(draw_list, (ImVec2){label_x, label_y},
                                text_color, compass_labels[i], NULL);
    }

    // Draw satellites
    for (int i = 0; i < polar->satellite_count; i++)
    {
        const satellite_visual_t *sat = &polar->satellites[i];

        if (!sat->visible)
            continue;

        // Color based on SNR
        ImVec4 sat_color;
        if (sat->snr >= 35)
        {
            sat_color = (ImVec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green
        }
        else if (sat->snr >= 20)
        {
            sat_color = (ImVec4){1.0f, 1.0f, 0.0f, 1.0f}; // Yellow
        }
        else if (sat->snr > 0)
        {
            sat_color = (ImVec4){1.0f, 0.5f, 0.0f, 1.0f}; // Orange
        }
        else
        {
            sat_color = (ImVec4){0.5f, 0.5f, 0.5f, 1.0f}; // Gray
        }

        // Highlight if used in fix
        if (sat->used_in_fix)
        {
            sat_color.w = 1.0f;
            // Draw outer ring for used satellites
            ImU32 ring_color = igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 1.0f, 0.8f});
            ImDrawList_AddCircle(draw_list, (ImVec2){sat->screen_x, sat->screen_y},
                                 12.0f, ring_color, 16, 2.0f);
        }

        // Satellite circle
        ImU32 sat_color_u32 = igGetColorU32_Vec4(sat_color);
        float sat_radius = sat->used_in_fix ? 8.0f : 6.0f;
        ImDrawList_AddCircleFilled(draw_list, (ImVec2){sat->screen_x, sat->screen_y},
                                   sat_radius, sat_color_u32, 16);

        // PRN label
        char prn_text[8];
        snprintf(prn_text, sizeof(prn_text), "%d", sat->prn);

        ImVec2 text_size;
        igCalcTextSize(&text_size, prn_text, NULL, false, -1.0f);

        float text_x = sat->screen_x - text_size.x * 0.5f;
        float text_y = sat->screen_y - text_size.y * 0.5f;

        // Text background for readability
        ImU32 text_bg = igGetColorU32_Vec4((ImVec4){0.0f, 0.0f, 0.0f, 0.7f});
        ImDrawList_AddRectFilled(draw_list,
                                 (ImVec2){text_x - 2, text_y - 1},
                                 (ImVec2){text_x + text_size.x + 2, text_y + text_size.y + 1},
                                 text_bg, 2.0f, ImDrawFlags_RoundCornersAll);

        ImDrawList_AddText_Vec2(draw_list, (ImVec2){text_x, text_y},
                                igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}),
                                prn_text, NULL);
    }

    // Outer border
    ImU32 border_color = igGetColorU32_Col(ImGuiCol_Border, 1.0f);
    ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, radius, border_color, 64, 2.0f);

    // Handle mouse interaction
    ImVec2 mouse_pos;
    igGetMousePos(&mouse_pos);
    bool mouse_in_canvas = (mouse_pos.x >= canvas_pos.x && mouse_pos.x <= canvas_pos.x + size &&
                            mouse_pos.y >= canvas_pos.y && mouse_pos.y <= canvas_pos.y + size);

    if (mouse_in_canvas)
    {
        bool clicked = igIsMouseClicked_Bool(ImGuiMouseButton_Left, false);
        polar_view_handle_mouse(polar, mouse_pos.x, mouse_pos.y, clicked);
    }

    // Selected satellite info
    if (polar->selected_satellite >= 0 && polar->selected_satellite < polar->satellite_count)
    {
        const satellite_visual_t *sel_sat = &polar->satellites[polar->selected_satellite];
        igSeparator();
        igText("Selected Satellite: PRN %d", sel_sat->prn);
        igText("  Elevation: %.0f°", sel_sat->elevation);
        igText("  Azimuth: %.0f°", sel_sat->azimuth);
        igText("  SNR: %.0f dB", sel_sat->snr);
        igText("  Used in fix: %s", sel_sat->used_in_fix ? "Yes" : "No");
    }

    // Legend
    igSeparator();
    igText("Legend:");
    igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "●");
    igSameLine(0, 5);
    igText("Strong signal (≥35 dB)");

    igTextColored((ImVec4){1.0f, 1.0f, 0.0f, 1.0f}, "●");
    igSameLine(0, 5);
    igText("Good signal (≥20 dB)");

    igTextColored((ImVec4){1.0f, 0.5f, 0.0f, 1.0f}, "●");
    igSameLine(0, 5);
    igText("Weak signal (<20 dB)");

    igTextColored((ImVec4){0.5f, 0.5f, 0.5f, 1.0f}, "●");
    igSameLine(0, 5);
    igText("No signal");

    igText("○ White ring = Used in position fix");

    // Reserve space for the canvas
    igInvisibleButton("polar_canvas", (ImVec2){size, size}, ImGuiButtonFlags_None);
}

void render_compass_window(app_state_t *app)
{
    // Set default size for compass window
    igSetNextWindowSize((ImVec2){350, 400}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 320, 450}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("Compass", NULL, ImGuiWindowFlags_None))
    {
        render_compass_panel(app);
    }
    igEnd();
}

void render_compass_panel(app_state_t *app)
{
    gps_data_t *gps = &app->gps_data;
    compass_t *compass = &app->compass;

    // Controls
    igText("Compass & Direction");
    igSeparator();

    igCheckbox("Auto-rotate with GPS", &compass->auto_rotate);
    float declination = compass->declination;
    if (igSliderFloat("Magnetic Declination", &declination, -30.0f, 30.0f, "%.1f°", ImGuiSliderFlags_None))
    {
        compass_set_declination(compass, declination);
    }

    // Current heading display
    float current_heading = compass->heading;
    if (compass->auto_rotate && gps->motion_valid)
    {
        current_heading = gps->course;
    }

    igText("Current Heading: %.1f°", current_heading);
    igText("Magnetic Declination: %.1f°", compass->declination);
    igText("True Heading: %.1f°", compass_get_true_heading(compass));

    igSeparator();

    // Compass canvas
    ImVec2 canvas_pos, canvas_size;
    igGetCursorScreenPos(&canvas_pos);
    igGetContentRegionAvail(&canvas_size);

    // Make it square
    float size = fminf(canvas_size.x, canvas_size.y) - 20.0f;
    if (size < 200.0f)
        size = 200.0f;

    float center_x = canvas_pos.x + size * 0.5f;
    float center_y = canvas_pos.y + size * 0.5f;
    float radius = size * 0.45f;

    ImDrawList *draw_list = igGetWindowDrawList();

    // Background circle
    ImU32 bg_color = igGetColorU32_Col(ImGuiCol_ChildBg, 1.0f);
    ImDrawList_AddCircleFilled(draw_list, (ImVec2){center_x, center_y}, radius, bg_color, 64);

    // Outer ring
    ImU32 ring_color = igGetColorU32_Col(ImGuiCol_Border, 1.0f);
    ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, radius, ring_color, 64, 3.0f);

    // Inner rings for reference
    ImU32 grid_color = igGetColorU32_Vec4((ImVec4){0.4f, 0.4f, 0.4f, 0.4f});
    ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, radius * 0.7f, grid_color, 32, 1.0f);
    ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, radius * 0.4f, grid_color, 32, 1.0f);

    // Cardinal direction lines
    for (int angle = 0; angle < 360; angle += 30)
    {
        float line_thickness = (angle % 90 == 0) ? 2.0f : 1.0f;
        ImU32 line_color = (angle % 90 == 0) ? igGetColorU32_Vec4((ImVec4){0.8f, 0.8f, 0.8f, 1.0f}) : grid_color;

        float angle_rad = (angle - 90.0f) * M_PI / 180.0f;
        float start_r = (angle % 90 == 0) ? radius * 0.85f : radius * 0.9f;

        float start_x = center_x + start_r * cosf(angle_rad);
        float start_y = center_y + start_r * sinf(angle_rad);
        float end_x = center_x + radius * cosf(angle_rad);
        float end_y = center_y + radius * sinf(angle_rad);

        ImDrawList_AddLine(draw_list, (ImVec2){start_x, start_y},
                           (ImVec2){end_x, end_y}, line_color, line_thickness);
    }

    // Cardinal direction labels
    ImU32 text_color = igGetColorU32_Col(ImGuiCol_Text, 1.0f);
    float label_radius = radius + 15.0f;

    const char *compass_labels[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    for (int i = 0; i < 8; i++)
    {
        float angle_rad = (i * 45.0f - 90.0f) * M_PI / 180.0f;
        float label_x = center_x + label_radius * cosf(angle_rad) - 8.0f;
        float label_y = center_y + label_radius * sinf(angle_rad) - 8.0f;

        // Highlight N, E, S, W
        ImU32 label_color = (i % 2 == 0) ? igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 0.0f, 1.0f}) : text_color;

        ImDrawList_AddText_Vec2(draw_list, (ImVec2){label_x, label_y},
                                label_color, compass_labels[i], NULL);
    }

    // Heading arrow (North indicator)
    float heading_rad = (current_heading - 90.0f) * M_PI / 180.0f;
    float arrow_length = radius * 0.8f;

    // Main arrow
    float arrow_end_x = center_x + arrow_length * cosf(heading_rad);
    float arrow_end_y = center_y + arrow_length * sinf(heading_rad);

    ImU32 arrow_color = igGetColorU32_Vec4((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}); // Red
    ImDrawList_AddLine(draw_list, (ImVec2){center_x, center_y},
                       (ImVec2){arrow_end_x, arrow_end_y}, arrow_color, 4.0f);

    // Arrow head
    float arrow_angle1 = heading_rad + (150.0f * M_PI / 180.0f);
    float arrow_angle2 = heading_rad - (150.0f * M_PI / 180.0f);
    float head_length = 20.0f;

    float head1_x = arrow_end_x + head_length * cosf(arrow_angle1);
    float head1_y = arrow_end_y + head_length * sinf(arrow_angle1);
    float head2_x = arrow_end_x + head_length * cosf(arrow_angle2);
    float head2_y = arrow_end_y + head_length * sinf(arrow_angle2);

    ImDrawList_AddLine(draw_list, (ImVec2){arrow_end_x, arrow_end_y},
                       (ImVec2){head1_x, head1_y}, arrow_color, 3.0f);
    ImDrawList_AddLine(draw_list, (ImVec2){arrow_end_x, arrow_end_y},
                       (ImVec2){head2_x, head2_y}, arrow_color, 3.0f);

    // Center dot
    ImU32 center_color = igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 1.0f, 1.0f});
    ImDrawList_AddCircleFilled(draw_list, (ImVec2){center_x, center_y}, 5.0f, center_color, 16);

    // Speed and course info overlay
    if (gps->motion_valid)
    {
        char info_text[128];
        snprintf(info_text, sizeof(info_text), "Speed: %.1f km/h", gps->speed_kmh);
        ImDrawList_AddText_Vec2(draw_list,
                                (ImVec2){canvas_pos.x + 10, canvas_pos.y + 10},
                                text_color, info_text, NULL);

        snprintf(info_text, sizeof(info_text), "Course: %.1f°", gps->course);
        ImDrawList_AddText_Vec2(draw_list,
                                (ImVec2){canvas_pos.x + 10, canvas_pos.y + 30},
                                text_color, info_text, NULL);
    }

    // Reserve space for the canvas
    igInvisibleButton("compass_canvas", (ImVec2){size, size}, ImGuiButtonFlags_None);
}

void render_raw_data_window(app_state_t *app)
{
    // Set default size for raw data window
    igSetNextWindowSize((ImVec2){600, 300}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 20, 450}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("Raw Data Console", NULL, ImGuiWindowFlags_None))
    {
        render_raw_data_panel(app);
    }
    igEnd();
}

void render_analysis_window(app_state_t *app)
{
    igSetNextWindowSize((ImVec2){720, 560}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 640, 400}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("Analytics", NULL, ImGuiWindowFlags_None))
    {
        render_analysis_panel(app);
    }
    igEnd();
}

void render_analysis_panel(app_state_t *app)
{
    track_analysis_summary_t summary;
    static double speed_time_minutes[MAX_TRACK_POINTS];
    static double speed_kmh[MAX_TRACK_POINTS];
    static double distance_km[MAX_TRACK_POINTS];
    static double altitude_m[MAX_TRACK_POINTS];

    gps_analysis_compute_track_summary(&app->map_system, &summary);

    igText("Track Analyzer");
    igSeparator();

    if (!summary.has_track)
    {
        igTextColored((ImVec4){0.75f, 0.75f, 0.75f, 1.0f}, "No recorded track data yet.");
        igText("Start Recording from Tools menu to collect metrics and charts.");
        return;
    }

    igText("Points: %d", summary.point_count);
    igText("Distance: %.2f km", summary.total_distance_km);
    igText("Duration: %02d:%02d:%02d",
           (int)(summary.duration_seconds / 3600.0),
           (int)(((int)summary.duration_seconds % 3600) / 60),
           (int)((int)summary.duration_seconds % 60));
    igText("Average Speed: %.1f km/h", summary.average_speed_kmh);
    igText("Max Speed: %.1f km/h", summary.max_speed_kmh);
    igText("Altitude Range: %.1f m .. %.1f m", summary.min_altitude_m, summary.max_altitude_m);

    igSeparator();

    int speed_point_count = gps_analysis_build_speed_time_series(&app->map_system,
                                                                 speed_time_minutes,
                                                                 speed_kmh,
                                                                 MAX_TRACK_POINTS);
    int altitude_point_count = gps_analysis_build_altitude_distance_series(&app->map_system,
                                                                           distance_km,
                                                                           altitude_m,
                                                                           MAX_TRACK_POINTS);

    static bool reset_speed_axes = false;
    static bool reset_altitude_axes = false;

    if (igButton("Reset Speed Plot", (ImVec2){130, 0}))
    {
        reset_speed_axes = true;
    }
    igSameLine(0, 10);
    if (igButton("Reset Altitude Plot", (ImVec2){150, 0}))
    {
        reset_altitude_axes = true;
    }
    igSameLine(0, 10);
    igTextDisabled("Tip: mouse wheel=zoom, drag=pan");

    igSeparator();

    if (speed_point_count > 1)
    {
        if (reset_speed_axes)
        {
            gps_implot_set_next_axes_to_fit();
            reset_speed_axes = false;
        }
        if (gps_implot_begin_plot("Speed vs Time",
                                  -1.0f,
                                  190.0f,
                                  "Elapsed Time (min)",
                                  "Speed (km/h)",
                                  GPS_IMPLOT_FLAG_NO_LEGEND,
                                  GPS_IMPLOT_AXIS_FLAG_AUTO_FIT,
                                  GPS_IMPLOT_AXIS_FLAG_AUTO_FIT))
        {
            gps_implot_plot_line("Speed", speed_time_minutes, speed_kmh, speed_point_count);
            if (gps_implot_is_plot_hovered())
            {
                double mx = 0.0;
                double my = 0.0;
                if (gps_implot_get_plot_mouse_pos(&mx, &my))
                {
                    igText("Cursor: t=%.2f min, v=%.2f km/h", mx, my);
                }
            }
            gps_implot_end_plot();
        }
    }
    else
    {
        igTextColored((ImVec4){0.75f, 0.75f, 0.75f, 1.0f}, "Speed graph will appear after at least two recorded samples.");
    }

    if (altitude_point_count > 1)
    {
        if (reset_altitude_axes)
        {
            gps_implot_set_next_axes_to_fit();
            reset_altitude_axes = false;
        }
        if (gps_implot_begin_plot("Altitude vs Distance",
                                  -1.0f,
                                  190.0f,
                                  "Distance (km)",
                                  "Altitude (m)",
                                  GPS_IMPLOT_FLAG_NO_LEGEND,
                                  GPS_IMPLOT_AXIS_FLAG_AUTO_FIT,
                                  GPS_IMPLOT_AXIS_FLAG_AUTO_FIT))
        {
            gps_implot_plot_line("Altitude", distance_km, altitude_m, altitude_point_count);
            if (gps_implot_is_plot_hovered())
            {
                double mx = 0.0;
                double my = 0.0;
                if (gps_implot_get_plot_mouse_pos(&mx, &my))
                {
                    igText("Cursor: d=%.2f km, h=%.2f m", mx, my);
                }
            }
            gps_implot_end_plot();
        }
    }
    else
    {
        igTextColored((ImVec4){0.75f, 0.75f, 0.75f, 1.0f}, "Altitude graph will appear after at least two recorded samples.");
    }

    igSeparator();
    igTextWrapped("Charts are rendered with vendored ImPlot and automatically follow the current Dear ImGui theme.");
}

void render_raw_data_panel(app_state_t *app)
{
    gps_data_t *gps = &app->gps_data;
    console_t *console = &app->console;

    igText("GPS Device Configuration & Raw Data Monitor");
    igSeparator();

    // Connection info
    igText("Port: %s", gps->port_name);
    igSameLine(0, 20);
    igText("Baud: %d", gps->baud_rate);
    igSameLine(0, 20);
    igText("Status: %s", gps_status_to_string(gps->status));

    igSeparator();

    // Controls
    igCheckbox("Auto-scroll console", &console->auto_scroll);
    igSameLine(0, 20);

    if (igButton("Clear Console", (ImVec2){0, 0}))
    {
        console_clear(console);
    }

    igSameLine(0, 20);
    bool can_send_command = gps_serial_is_open(&app->gps_serial);
    if (!can_send_command)
        igPushStyleVar_Float(ImGuiStyleVar_Alpha, 0.45f);
    if (igButton("Request Version", (ImVec2){0, 0}))
    {
        if (can_send_command && gps_serial_send_command(&app->gps_serial, "$PMTK605*31"))
        {
            console_add_line(console, ">>> $PMTK605*31 (Request firmware version)");
        }
        else
        {
            console_add_line(console, "Error: Failed to send command");
        }
    }
    if (!can_send_command)
        igPopStyleVar(1);
    ui_show_tooltip(can_send_command ? "GPS firmware bilgisini ister" : "Komut icin once GPS baglantisi gerekli");

    igSameLine(0, 20);
    if (!can_send_command)
        igPushStyleVar_Float(ImGuiStyleVar_Alpha, 0.45f);
    if (igButton("Reset Config", (ImVec2){0, 0}))
    {
        if (can_send_command && gps_serial_send_command(&app->gps_serial, "$PMTK104*37"))
        {
            console_add_line(console, ">>> $PMTK104*37 (Cold restart)");
        }
        else
        {
            console_add_line(console, "Error: Failed to send command");
        }
    }
    if (!can_send_command)
        igPopStyleVar(1);
    ui_show_tooltip(can_send_command ? "GPS aliciyi cold restart eder" : "Komut icin once GPS baglantisi gerekli");

    igSeparator();

    // Raw data console (5 lines max)
    igText("Raw NMEA Data (Last 5 sentences):");

    ImGuiWindowFlags console_flags = ImGuiWindowFlags_HorizontalScrollbar;
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){5.0f, 5.0f});

    if (igBeginChild_Str("RawConsole", (ImVec2){0, 120}, ImGuiChildFlags_Borders, console_flags))
    {
        // Remove font push since it's not supported in this CImGui version

        for (int i = 0; i < 5; i++)
        {
            const char *line = console_get_line(console, i);
            if (line)
            {
                // Color code different NMEA sentence types
                ImVec4 line_color = {0.8f, 0.8f, 0.8f, 1.0f}; // Default white

                if (strstr(line, "$GPRMC") || strstr(line, "$GNRMC"))
                {
                    line_color = (ImVec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green for RMC
                }
                else if (strstr(line, "$GPGGA") || strstr(line, "$GNGGA"))
                {
                    line_color = (ImVec4){0.0f, 0.8f, 1.0f, 1.0f}; // Cyan for GGA
                }
                else if (strstr(line, "$GPGSV") || strstr(line, "$GNGSV"))
                {
                    line_color = (ImVec4){1.0f, 0.8f, 0.0f, 1.0f}; // Orange for GSV
                }
                else if (strstr(line, "$PMTK"))
                {
                    line_color = (ImVec4){1.0f, 0.0f, 1.0f, 1.0f}; // Magenta for MTK responses
                }
                else if (strstr(line, ">>>"))
                {
                    line_color = (ImVec4){1.0f, 1.0f, 0.0f, 1.0f}; // Yellow for commands
                }

                igTextColored(line_color, "%s", line);
            }
        }

        // Auto-scroll to bottom only if auto-scroll is enabled AND there's new data
        if (console->auto_scroll && console_has_new_data(console))
        {
            igSetScrollHereY(1.0f);
            console_mark_data_consumed(console);
        }
    }
    igEndChild();
    igPopStyleVar(1);

    igSeparator();

    // Command input
    igText("Send Command to GPS:");
    igSetNextItemWidth(-100.0f);

    // Get mutable copy of command for input
    char command_buffer[256];
    strncpy(command_buffer, console_get_command(console), sizeof(command_buffer));
    command_buffer[255] = '\0';

    bool enter_pressed = igInputText("##command", command_buffer, sizeof(command_buffer),
                                     ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL);

    // Update console command buffer
    console_set_command(console, command_buffer);

    igSameLine(0, 5);
    bool send_clicked = igButton("Send", (ImVec2){80, 0});

    if (enter_pressed || send_clicked)
    {
        const char *cmd = console_get_command(console);
        if (strlen(cmd) > 0)
        {
            if (gps_serial_send_command(&app->gps_serial, cmd))
            {
                char cmd_line[300];
                snprintf(cmd_line, sizeof(cmd_line), ">>> %s", cmd);
                console_add_line(console, cmd_line);
            }
            else
            {
                console_add_line(console, "Error: Failed to send command");
            }
            console_clear_command(console);
        }
    }

    // Common GPS commands help
    igSeparator();
    igText("Common Commands:");
    igBulletText("$PMTK605*31 - Query firmware version");
    igBulletText("$PMTK104*37 - Cold restart");
    igBulletText("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28 - RMC+GGA only");
    igBulletText("$PMTK314,-1*04 - Restore default NMEA sentences");
    igBulletText("$PMTK220,1000*1F - Set update rate to 1Hz");
}

void render_gpx_export_dialog(app_state_t *app)
{
    if (!app->show_gpx_export_dialog)
        return;

    bool open = app->show_gpx_export_dialog;
    if (igBegin("Export GPX", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        igText("Export track to GPX file");
        igSeparator();

        igText("Filename:");
        igInputText("##filename", app->gpx_filename, sizeof(app->gpx_filename),
                    ImGuiInputTextFlags_None, NULL, NULL);

        igText("Track points: %d", app->map_system.track.point_count);
        igText("Distance: %.2f km", app->map_system.track.total_distance / 1000.0);

        igSeparator();

        if (igButton("Export", (ImVec2){80, 0}))
        {
            bool success = map_system_save_gpx(&app->map_system, app->gpx_filename);
            app->gpx_export_success = success;

            if (success)
            {
                snprintf(app->gpx_export_message, sizeof(app->gpx_export_message),
                         "GPX file exported successfully to %s", app->gpx_filename);
            }
            else
            {
                snprintf(app->gpx_export_message, sizeof(app->gpx_export_message),
                         "Failed to export GPX file: %s", app->gpx_filename);
            }

            app->show_gpx_export_dialog = false;
        }

        igSameLine(0, 10);
        if (igButton("Cancel", (ImVec2){80, 0}))
        {
            app->show_gpx_export_dialog = false;
        }

        // Show export result message
        if (strlen(app->gpx_export_message) > 0)
        {
            igSeparator();
            if (app->gpx_export_success)
            {
                igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "%s", app->gpx_export_message);
            }
            else
            {
                igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "%s", app->gpx_export_message);
            }
        }
    }

    if (!open)
    {
        app->show_gpx_export_dialog = false;
    }

    igEnd();
}

void render_gpx_import_dialog(app_state_t *app)
{
    if (!app->show_gpx_import_dialog)
        return;

    bool open = app->show_gpx_import_dialog;
    if (igBegin("Import GPX", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        igText("Import GPX track into current analysis context");
        igSeparator();

        igText("Filename:");
        igInputText("##import_filename", app->gpx_import_filename, sizeof(app->gpx_import_filename),
                    ImGuiInputTextFlags_None, NULL, NULL);

        igTextWrapped("Import operation clears current in-memory track and replaces it with GPX points.");

        igSeparator();
        if (igButton("Import", (ImVec2){80, 0}))
        {
            bool success = map_system_load_gpx(&app->map_system, app->gpx_import_filename);
            app->gpx_import_success = success;
            if (success)
            {
                snprintf(app->gpx_import_message, sizeof(app->gpx_import_message),
                         "GPX imported successfully from %s", app->gpx_import_filename);
            }
            else
            {
                snprintf(app->gpx_import_message, sizeof(app->gpx_import_message),
                         "Failed to import GPX file: %s", app->gpx_import_filename);
            }
            app->show_gpx_import_dialog = false;
        }

        igSameLine(0, 10);
        if (igButton("Cancel", (ImVec2){80, 0}))
        {
            app->show_gpx_import_dialog = false;
        }

        if (strlen(app->gpx_import_message) > 0)
        {
            igSeparator();
            if (app->gpx_import_success)
            {
                igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "%s", app->gpx_import_message);
            }
            else
            {
                igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "%s", app->gpx_import_message);
            }
        }
    }

    if (!open)
    {
        app->show_gpx_import_dialog = false;
    }

    igEnd();
}

void render_csv_export_dialog(app_state_t *app)
{
    if (!app->show_csv_export_dialog)
        return;

    bool open = app->show_csv_export_dialog;
    if (igBegin("Export CSV (Analysis)", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        igText("Export current track analysis data to CSV");
        igSeparator();

        igText("Filename:");
        igInputText("##csv_filename", app->csv_export_filename, sizeof(app->csv_export_filename),
                    ImGuiInputTextFlags_None, NULL, NULL);

        igText("Track points: %d", app->map_system.track.point_count);

        igSeparator();
        if (igButton("Export", (ImVec2){80, 0}))
        {
            bool success = map_system_export_track_csv(&app->map_system, app->csv_export_filename);
            app->csv_export_success = success;
            if (success)
            {
                snprintf(app->csv_export_message, sizeof(app->csv_export_message),
                         "CSV exported successfully to %s", app->csv_export_filename);
            }
            else
            {
                snprintf(app->csv_export_message, sizeof(app->csv_export_message),
                         "Failed to export CSV file: %s", app->csv_export_filename);
            }
            app->show_csv_export_dialog = false;
        }

        igSameLine(0, 10);
        if (igButton("Cancel", (ImVec2){80, 0}))
        {
            app->show_csv_export_dialog = false;
        }

        if (strlen(app->csv_export_message) > 0)
        {
            igSeparator();
            if (app->csv_export_success)
            {
                igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "%s", app->csv_export_message);
            }
            else
            {
                igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "%s", app->csv_export_message);
            }
        }
    }

    if (!open)
    {
        app->show_csv_export_dialog = false;
    }

    igEnd();
}

void render_satellite_window(app_state_t *app)
{
    // Set default size for satellite window
    igSetNextWindowSize((ImVec2){300, 250}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){10, 450}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("Satellites", NULL, ImGuiWindowFlags_None))
    {
        render_satellite_panel(app);
    }
    igEnd();
}
void render_satellite_panel(app_state_t *app)
{
    gps_data_t *gps = &app->gps_data;

    igText("Visible: %d | Used: %d", gps->satellites_visible, gps->satellites_used);
    igSeparator();

    if (gps->satellite_count > 0)
    {
        // Table header with wider SNR column for progress bar
        if (igBeginTable("satellites", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, (ImVec2){0, 0}, 0))
        {
            igTableSetupColumn("PRN", ImGuiTableColumnFlags_WidthFixed, 50.0f, 0);
            igTableSetupColumn("Elev", ImGuiTableColumnFlags_WidthFixed, 50.0f, 0);
            igTableSetupColumn("Azim", ImGuiTableColumnFlags_WidthFixed, 50.0f, 0);
            igTableSetupColumn("SNR", ImGuiTableColumnFlags_WidthFixed, 150.0f, 0);
            igTableHeadersRow();

            for (int i = 0; i < gps->satellite_count; i++)
            {
                satellite_info_t *sat = &gps->satellites[i];

                igTableNextRow(0, 0);

                igTableSetColumnIndex(0);
                igText("%d", sat->prn);

                igTableSetColumnIndex(1);
                igText("%d°", sat->elevation);

                igTableSetColumnIndex(2);
                igText("%d°", sat->azimuth);

                igTableSetColumnIndex(3);
                if (sat->snr > 0)
                {
                    // Color code SNR: Green > 35, Yellow 20-35, Red < 20
                    ImVec4 color;
                    if (sat->snr >= 35)
                    {
                        color = (ImVec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green
                    }
                    else if (sat->snr >= 20)
                    {
                        color = (ImVec4){1.0f, 1.0f, 0.0f, 1.0f}; // Yellow
                    }
                    else
                    {
                        color = (ImVec4){1.0f, 0.0f, 0.0f, 1.0f}; // Red
                    }

                    // Draw horizontal progress bar
                    ImVec2 cursor_pos;
                    igGetCursorScreenPos(&cursor_pos);

                    // Progress bar dimensions
                    float bar_width = 100.0f;
                    float bar_height = 18.0f;
                    float max_snr = 50.0f; // Maximum SNR for bar scaling
                    float progress = (sat->snr / max_snr);
                    if (progress > 1.0f)
                        progress = 1.0f;

                    ImDrawList *draw_list = igGetWindowDrawList();

                    // Background bar
                    ImU32 bg_color = igGetColorU32_Vec4((ImVec4){0.2f, 0.2f, 0.2f, 1.0f});
                    ImDrawList_AddRectFilled(draw_list, cursor_pos,
                                             (ImVec2){cursor_pos.x + bar_width, cursor_pos.y + bar_height},
                                             bg_color, 2.0f, ImDrawFlags_RoundCornersAll);

                    // Foreground progress bar
                    ImU32 bar_color = igGetColorU32_Vec4(color);
                    if (progress > 0.01f)
                    {
                        ImDrawList_AddRectFilled(draw_list, cursor_pos,
                                                 (ImVec2){cursor_pos.x + (bar_width * progress), cursor_pos.y + bar_height},
                                                 bar_color, 2.0f, ImDrawFlags_RoundCornersAll);
                    }

                    // Border
                    ImU32 border_color = igGetColorU32_Vec4((ImVec4){0.4f, 0.4f, 0.4f, 1.0f});
                    ImDrawList_AddRect(draw_list, cursor_pos,
                                       (ImVec2){cursor_pos.x + bar_width, cursor_pos.y + bar_height},
                                       border_color, 2.0f, ImDrawFlags_RoundCornersAll, 1.0f);

                    // Text overlay - centered on bar
                    char snr_text[16];
                    snprintf(snr_text, sizeof(snr_text), "%d dB", sat->snr);
                    ImVec2 text_size;
                    igCalcTextSize(&text_size, snr_text, NULL, false, -1.0f);
                    ImVec2 text_pos = {
                        cursor_pos.x + (bar_width - text_size.x) * 0.5f,
                        cursor_pos.y + (bar_height - text_size.y) * 0.5f};

                    // Text shadow for better readability
                    ImDrawList_AddText_Vec2(draw_list,
                                            (ImVec2){text_pos.x + 1, text_pos.y + 1},
                                            igGetColorU32_Vec4((ImVec4){0.0f, 0.0f, 0.0f, 0.8f}),
                                            snr_text, NULL);
                    ImDrawList_AddText_Vec2(draw_list, text_pos,
                                            igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}),
                                            snr_text, NULL);

                    // Reserve space for the bar
                    igDummy((ImVec2){bar_width, bar_height});
                    if (igIsItemHovered(ImGuiHoveredFlags_None))
                    {
                        igSetTooltip("PRN %d | Elev %d° | Azim %d° | SNR %d dB | Used: %s",
                                     sat->prn,
                                     sat->elevation,
                                     sat->azimuth,
                                     sat->snr,
                                     sat->used_in_fix ? "Yes" : "No");
                    }
                }
                else
                {
                    igTextColored((ImVec4){0.5f, 0.5f, 0.5f, 1.0f}, "--");
                }
            }

            igEndTable();
        }
    }
    else
    {
        igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "No satellite data available");
    }
}

void render_status_bar(app_state_t *app)
{
    ImGuiViewport *viewport = igGetMainViewport();
    ImVec2 status_pos = {viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - STATUS_BAR_HEIGHT};
    ImVec2 status_size = {viewport->WorkSize.x, STATUS_BAR_HEIGHT};

    igSetNextWindowPos(status_pos, ImGuiCond_Always, (ImVec2){0, 0});
    igSetNextWindowSize(status_size, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoCollapse;

    if (igBegin("Status Bar", NULL, flags))
    {
        gps_data_t *gps = &app->gps_data;

        // Connection status
        const char *status_str = gps_status_to_string(gps->status);
        if (gps->status == GPS_STATUS_CONNECTED)
        {
            igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "[%s]", status_str);
        }
        else
        {
            igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "[%s]", status_str);
        }

        igSameLine(0, 20);
        igText("Port: %s", gps->port_name);

        igSameLine(0, 20);
        ImVec4 fix_color = (gps->fix_quality == GPS_FIX_NONE)
                       ? (ImVec4){1.0f, 0.35f, 0.35f, 1.0f}
                       : ((gps->fix_quality == GPS_FIX_DGPS || gps->fix_quality == GPS_FIX_RTK || gps->fix_quality == GPS_FIX_FLOAT_RTK)
                          ? (ImVec4){0.35f, 1.0f, 0.35f, 1.0f}
                          : (ImVec4){1.0f, 0.9f, 0.35f, 1.0f});
        igTextColored(fix_color, "Fix: %s", gps_fix_quality_to_string(gps->fix_quality));

        igSameLine(0, 20);
        igText("Sats: %d", gps->satellites_used);

        igSameLine(0, 20);
        igText("Log: %s", gps->logging_enabled ? "ON" : "OFF");

        // FPS on the right
        ImVec2 fps_size;
        char fps_text[32];
        snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", cImGui_GetIO()->Framerate);
        igCalcTextSize(&fps_size, fps_text, NULL, false, -1.0f);
        igSameLine(viewport->WorkSize.x - fps_size.x - 20, -1);
        igText("%s", fps_text);
    }
    igEnd();
}

void render_connection_dialog(app_state_t *app)
{
    // Show connection dialog when requested
    if (app->show_connection_dialog)
    {
        bool open = app->show_connection_dialog;
        igSetNextWindowSize((ImVec2){400, 300}, ImGuiCond_FirstUseEver);

        if (igBegin("GPS Connection", &open, ImGuiWindowFlags_AlwaysAutoResize))
        {
            igText("Connect to GPS Device");
            igSeparator();

            // Port selection
            igText("Available Ports:");
            if (app->available_port_count > 0)
            {
                for (int i = 0; i < app->available_port_count; i++)
                {
                    bool is_selected = (strcmp(app->selected_port, app->available_ports[i]) == 0);
                    if (igRadioButton_Bool(app->available_ports[i], is_selected))
                    {
                        strcpy(app->selected_port, app->available_ports[i]);
                    }
                }
            }
            else
            {
                igTextColored((ImVec4){1.0f, 0.7f, 0.0f, 1.0f}, "No GPS devices found!");
                igText("Make sure your GPS device is connected via USB.");
            }

            igSeparator();

            // Baud rate selection
            igText("Baud Rate:");
            int current_baud_index = 1; // Default 9600
            const char *baud_options[] = {"4800", "9600", "19200", "38400", "57600", "115200"};
            const int baud_values[] = {4800, 9600, 19200, 38400, 57600, 115200};

            // Find current baud index
            for (int i = 0; i < 6; i++)
            {
                if (baud_values[i] == app->selected_baud)
                {
                    current_baud_index = i;
                    break;
                }
            }

            if (igCombo_Str_arr("##baud", &current_baud_index, baud_options, 6, -1))
            {
                app->selected_baud = baud_values[current_baud_index];
            }

            if (app->recent_connection_count > 0)
            {
                igSpacing();
                igText("Recent Port/Baud:");

                char recent_labels[CONNECTION_HISTORY_SIZE][320];
                const char *recent_items[CONNECTION_HISTORY_SIZE];

                for (int i = 0; i < app->recent_connection_count; i++)
                {
                    snprintf(recent_labels[i], sizeof(recent_labels[i]), "%s @ %d", app->recent_connections[i].port,
                             app->recent_connections[i].baud);
                    recent_items[i] = recent_labels[i];
                }

                if (app->selected_recent_index < 0 || app->selected_recent_index >= app->recent_connection_count)
                {
                    app->selected_recent_index = 0;
                }

                int recent_index = app->selected_recent_index;
                if (igCombo_Str_arr("##recent_conn", &recent_index, recent_items, app->recent_connection_count, 5))
                {
                    app->selected_recent_index = recent_index;
                    strcpy(app->selected_port, app->recent_connections[recent_index].port);
                    app->selected_baud = app->recent_connections[recent_index].baud;
                }

                if (igButton("Use Recent", (ImVec2){110, 0}))
                {
                    int idx = app->selected_recent_index;
                    if (idx >= 0 && idx < app->recent_connection_count)
                    {
                        strcpy(app->selected_port, app->recent_connections[idx].port);
                        app->selected_baud = app->recent_connections[idx].baud;
                    }
                }
                ui_show_tooltip("Secili son baglanti port/baud bilgisini forma uygular");
            }

            igSeparator();

            // Auto-connect checkbox
            igCheckbox("Enable Auto-connect", &app->auto_connect_enabled);
            igText("Automatically connect to first available GPS device on startup");

            igSeparator();

            // Connection buttons
            bool can_connect = (strlen(app->selected_port) > 0 && !gps_serial_is_open(&app->gps_serial));

            if (!can_connect)
                igBeginDisabled(true);
            if (igButton("Connect", (ImVec2){80, 0}))
            {
                if (gps_serial_open(&app->gps_serial, app->selected_port, app->selected_baud))
                {
                    app->gps_data.status = GPS_STATUS_CONNECTED;
                    strcpy(app->gps_data.port_name, app->selected_port);
                    app->gps_data.baud_rate = app->selected_baud;
                    connection_history_add(app, app->selected_port, app->selected_baud);
                    snprintf(app->connection_status_text, sizeof(app->connection_status_text),
                             "Connected to %s at %d baud", app->selected_port, app->selected_baud);
                    app->show_connection_dialog = false;
                }
                else
                {
                    snprintf(app->last_error, sizeof(app->last_error),
                             "Failed to connect to %s", app->selected_port);
                }
            }
            if (!can_connect)
                igEndDisabled();

            igSameLine(0, 10);
            if (igButton("Refresh Ports", (ImVec2){100, 0}))
            {
                gps_serial_list_ports(app->available_ports, 16, &app->available_port_count);
                strcpy(app->selected_port, ""); // Clear selection
            }

            igSameLine(0, 10);
            if (igButton("Cancel", (ImVec2){80, 0}))
            {
                app->show_connection_dialog = false;
            }

            // Show error if any
            if (strlen(app->last_error) > 0)
            {
                igSeparator();
                igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "Error: %s", app->last_error);
            }
        }

        if (!open)
        {
            app->show_connection_dialog = false;
        }

        igEnd();
    }

    // Auto-connect logic (only runs once at startup)
    static bool tried_auto_connect = false;
    if (!tried_auto_connect && app->auto_connect_enabled && !gps_serial_is_open(&app->gps_serial))
    {
        tried_auto_connect = true;

        char ports[16][256];
        int port_count;
        if (gps_serial_list_ports(ports, 16, &port_count) && port_count > 0)
        {
            // Try to connect to the first available port
            if (gps_serial_open(&app->gps_serial, ports[0], 9600))
            {
                app->gps_data.status = GPS_STATUS_CONNECTED;
                strcpy(app->gps_data.port_name, ports[0]);
                app->gps_data.baud_rate = 9600;
                connection_history_add(app, ports[0], 9600);
                snprintf(app->connection_status_text, sizeof(app->connection_status_text),
                         "Auto-connected to %s", ports[0]);
            }
        }
    }
}

void update_gps_data(app_state_t *app)
{
    if (gps_serial_is_open(&app->gps_serial))
    {
        // Use the new function that passes console to get raw NMEA data
        int processed = gps_serial_read_data_with_console(&app->gps_serial, &app->gps_data, &app->console);
        if (processed < 0)
        {
            // Error reading data
            gps_serial_close(&app->gps_serial);
            app->gps_data.status = GPS_STATUS_ERROR;
            strcpy(app->connection_status_text, "Connection Error");
        }
        else if (processed > 0)
        {
            // Update map system with new GPS data
            map_system_update(&app->map_system, &app->gps_data);
        }
    }
}

void ensure_data_directory(void)
{
    struct stat st;

    // Check if data directory exists
    if (stat("data", &st) == -1)
    {
        // Create data directory if it doesn't exist
        if (mkdir("data", 0755) != 0)
        {
            printf("Warning: Could not create data directory\n");
        }
        else
        {

            printf("Created data directory\n");
        }
    }

    // Check if map tile cache directory exists
    if (stat("data/map_tiles", &st) == -1)
    {
        if (mkdir("data/map_tiles", 0755) != 0)
        {
            printf("Warning: Could not create data/map_tiles directory\n");
        }
        else
        {
            printf("Created data/map_tiles directory\n");
        }
    }
}

/**
 * Simple markdown renderer for ImGui
 * This is a basic implementation that handles:
 * - Headers (# ## ###)
 * - Bold (**text**)
 * - Italic (*text*)
 * - Code blocks (```)
 * - Inline code (`code`)
 * - Lists (- item)
 * - Horizontal rules (---)
 */
void render_simple_markdown(const char *markdown_text)
{
    if (!markdown_text)
    {
        igText("No content available");
        return;
    }

    char line[1024];
    const char *current = markdown_text;
    bool in_code_block = false;

    while (*current)
    {
        // Find end of line
        const char *line_end = current;
        while (*line_end && *line_end != '\n' && *line_end != '\r')
        {
            line_end++;
        }

        // Copy line to buffer
        size_t line_length = line_end - current;
        if (line_length >= 1024)
        {
            line_length = 1023;
        }
        strncpy(line, current, line_length);
        line[line_length] = '\0';

        // Trim whitespace from the beginning
        char *trimmed_line = line;
        while (*trimmed_line == ' ' || *trimmed_line == '\t')
        {
            trimmed_line++;
        }

        // Handle different markdown elements
        if (strncmp(trimmed_line, "```", 3) == 0)
        {
            // Code block toggle
            in_code_block = !in_code_block;
            if (in_code_block)
            {
                igSeparator();
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.8f, 0.8f, 0.8f, 1.0f});
            }
            else
            {
                igPopStyleColor(1);
                igSeparator();
            }
        }
        else if (in_code_block)
        {
            // Inside code block - render as-is with monospace
            igText("%s", trimmed_line);
        }
        else if (strncmp(trimmed_line, "### ", 4) == 0)
        {
            // H3 header
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.4f, 0.8f, 1.0f, 1.0f});
            igText("%s", trimmed_line + 4);
            igPopStyleColor(1);
            igSeparator();
        }
        else if (strncmp(trimmed_line, "## ", 3) == 0)
        {
            // H2 header
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.3f, 0.7f, 1.0f, 1.0f});
            igText("%s", trimmed_line + 3);
            igPopStyleColor(1);
            igSeparator();
        }
        else if (strncmp(trimmed_line, "# ", 2) == 0)
        {
            // H1 header
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.2f, 0.6f, 1.0f, 1.0f});
            igText("%s", trimmed_line + 2);
            igPopStyleColor(1);
            igSeparator();
        }
        else if (strncmp(trimmed_line, "---", 3) == 0 || strncmp(trimmed_line, "===", 3) == 0)
        {
            // Horizontal rule
            igSeparator();
        }
        else if (strncmp(trimmed_line, "- ", 2) == 0 || strncmp(trimmed_line, "* ", 2) == 0)
        {
            // List item
            igBulletText("%s", trimmed_line + 2);
        }
        else if (strlen(trimmed_line) == 0)
        {
            // Empty line - add spacing
            igSpacing();
        }
        else
        {
            // Regular text - handle inline formatting
            // For now, just render as plain text
            // TODO: Handle **bold**, *italic*, `code` inline formatting
            igTextWrapped("%s", trimmed_line);
        }

        // Move to next line
        current = line_end;
        if (*current == '\r')
            current++;
        if (*current == '\n')
            current++;
    }
}

void render_help_window(app_state_t *app)
{
    if (!app->show_help_window)
    {
        return;
    }

    // Set window size and position
    igSetNextWindowSize((ImVec2){600, 500}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){200, 100}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("Help - GPC GPS Console", &app->show_help_window, ImGuiWindowFlags_None))
    {
        static markdown_content_t help_content = {NULL, 0, false, ""};
        static bool content_loaded = false;

        if (!content_loaded)
        {
            content_loaded = tools_load_markdown_file("src/docs/gpc_help.md", &help_content);
        }

        if (help_content.is_loaded && help_content.content)
        {
            // Create scrollable child window for content
            if (igBeginChild_Str("help_content", (ImVec2){0, -30}, true, ImGuiWindowFlags_None))
            {
                render_simple_markdown(help_content.content);
            }
            igEndChild();

            igSeparator();
            if (igButton("Close", (ImVec2){0, 0}))
            {
                app->show_help_window = false;
            }
        }
        else
        {
            igText("Unable to load help file: src/docs/gpc_help.md");
            igText("Please ensure the file exists and is readable.");

            igSeparator();
            if (igButton("Close", (ImVec2){0, 0}))
            {
                app->show_help_window = false;
            }
        }
    }
    igEnd();
}

void render_about_window(app_state_t *app)
{
    if (!app->show_about_window)
    {
        return;
    }

    // Set window size and position
    igSetNextWindowSize((ImVec2){500, 400}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){250, 150}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});

    if (igBegin("About - GPC GPS Console", &app->show_about_window, ImGuiWindowFlags_None))
    {
        static markdown_content_t about_content = {NULL, 0, false, ""};
        static bool content_loaded = false;

        if (!content_loaded)
        {
            content_loaded = tools_load_markdown_file("src/docs/gpc_about.md", &about_content);
        }

        if (about_content.is_loaded && about_content.content)
        {
            // Create scrollable child window for content
            if (igBeginChild_Str("about_content", (ImVec2){0, -30}, true, ImGuiWindowFlags_None))
            {
                render_simple_markdown(about_content.content);
            }
            igEndChild();

            igSeparator();
            if (igButton("Close", (ImVec2){0, 0}))
            {
                app->show_about_window = false;
            }
        }
        else
        {
            igText("Unable to load about file: src/docs/gpc_about.md");
            igText("Please ensure the file exists and is readable.");

            igSeparator();
            if (igButton("Close", (ImVec2){0, 0}))
            {
                app->show_about_window = false;
            }
        }
    }
    igEnd();
}