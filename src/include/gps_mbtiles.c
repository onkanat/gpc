#include "gps_mbtiles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_include)
#  if __has_include(<sqlite3.h>)
#    include <sqlite3.h>
#    define GPC_HAVE_SQLITE3 1
#  else
#    define GPC_HAVE_SQLITE3 0
#  endif
#else
#  include <sqlite3.h>
#  define GPC_HAVE_SQLITE3 1
#endif

#if GPC_HAVE_SQLITE3
typedef struct {
    sqlite3* db;
    sqlite3_stmt* stmt_has_tile;
    sqlite3_stmt* stmt_get_blob;
    sqlite3_stmt* stmt_get_scheme;
    int row_scheme; // 0=unknown, 1=tms, 2=xyz
    bool row_scheme_from_metadata;
    char db_path[512];
} mbtiles_ctx_t;

static mbtiles_ctx_t g_mbtiles_ctx = {NULL, NULL, NULL, NULL, 0, false, {0}};

static int xyz_to_tms_y(int zoom, int y_xyz) {
    int max_index = (1 << zoom) - 1;
    return max_index - y_xyz;
}

static int xyz_to_mbtiles_row(int zoom, int y_xyz) {
    // MBTiles spec default is TMS; some datasets store XYZ rows.
    if (g_mbtiles_ctx.row_scheme == 2) {
        return y_xyz;
    }
    return xyz_to_tms_y(zoom, y_xyz);
}

static int xyz_to_mbtiles_row_with_scheme(int zoom, int y_xyz, int row_scheme) {
    if (row_scheme == 2) {
        return y_xyz;
    }
    return xyz_to_tms_y(zoom, y_xyz);
}

static void map_mbtiles_load_scheme_if_needed(void) {
    if (!g_mbtiles_ctx.db || !g_mbtiles_ctx.stmt_get_scheme) return;
    if (g_mbtiles_ctx.row_scheme != 0) return;

    g_mbtiles_ctx.row_scheme = 1; // default to tms
    g_mbtiles_ctx.row_scheme_from_metadata = false;

    sqlite3_reset(g_mbtiles_ctx.stmt_get_scheme);
    sqlite3_clear_bindings(g_mbtiles_ctx.stmt_get_scheme);

    if (sqlite3_step(g_mbtiles_ctx.stmt_get_scheme) == SQLITE_ROW) {
        const unsigned char* txt = sqlite3_column_text(g_mbtiles_ctx.stmt_get_scheme, 0);
        if (txt) {
            if (strcmp((const char*)txt, "xyz") == 0) {
                g_mbtiles_ctx.row_scheme = 2;
                g_mbtiles_ctx.row_scheme_from_metadata = true;
            }
            else if (strcmp((const char*)txt, "tms") == 0) {
                g_mbtiles_ctx.row_scheme = 1;
                g_mbtiles_ctx.row_scheme_from_metadata = true;
            }
        }
    }

    sqlite3_reset(g_mbtiles_ctx.stmt_get_scheme);
}

static void map_mbtiles_close_cached_db(void) {
    if (g_mbtiles_ctx.stmt_has_tile) {
        sqlite3_finalize(g_mbtiles_ctx.stmt_has_tile);
        g_mbtiles_ctx.stmt_has_tile = NULL;
    }
    if (g_mbtiles_ctx.stmt_get_blob) {
        sqlite3_finalize(g_mbtiles_ctx.stmt_get_blob);
        g_mbtiles_ctx.stmt_get_blob = NULL;
    }
    if (g_mbtiles_ctx.stmt_get_scheme) {
        sqlite3_finalize(g_mbtiles_ctx.stmt_get_scheme);
        g_mbtiles_ctx.stmt_get_scheme = NULL;
    }
    if (g_mbtiles_ctx.db) {
        sqlite3_close(g_mbtiles_ctx.db);
        g_mbtiles_ctx.db = NULL;
    }
    g_mbtiles_ctx.row_scheme = 0;
    g_mbtiles_ctx.row_scheme_from_metadata = false;
    g_mbtiles_ctx.db_path[0] = '\0';
}

static bool map_mbtiles_ensure_db_ready(const char* db_path) {
    if (!db_path || db_path[0] == '\0') return false;

    if (g_mbtiles_ctx.db && strcmp(g_mbtiles_ctx.db_path, db_path) == 0 &&
        g_mbtiles_ctx.stmt_has_tile && g_mbtiles_ctx.stmt_get_blob) {
        return true;
    }

    map_mbtiles_close_cached_db();

    if (sqlite3_open_v2(db_path, &g_mbtiles_ctx.db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        map_mbtiles_close_cached_db();
        return false;
    }

    const char* sql_has =
        "SELECT 1 FROM tiles "
        "WHERE zoom_level=? AND tile_column=? AND tile_row=? LIMIT 1;";

    const char* sql_blob =
        "SELECT tile_data FROM tiles "
        "WHERE zoom_level=? AND tile_column=? AND tile_row=? LIMIT 1;";

    const char* sql_scheme =
        "SELECT value FROM metadata WHERE name='scheme' LIMIT 1;";

    if (sqlite3_prepare_v2(g_mbtiles_ctx.db, sql_has, -1, &g_mbtiles_ctx.stmt_has_tile, NULL) != SQLITE_OK) {
        map_mbtiles_close_cached_db();
        return false;
    }

    if (sqlite3_prepare_v2(g_mbtiles_ctx.db, sql_blob, -1, &g_mbtiles_ctx.stmt_get_blob, NULL) != SQLITE_OK) {
        map_mbtiles_close_cached_db();
        return false;
    }

    if (sqlite3_prepare_v2(g_mbtiles_ctx.db, sql_scheme, -1, &g_mbtiles_ctx.stmt_get_scheme, NULL) != SQLITE_OK) {
        // metadata table may not exist; keep running with default TMS.
        g_mbtiles_ctx.stmt_get_scheme = NULL;
    }

    map_mbtiles_load_scheme_if_needed();

    snprintf(g_mbtiles_ctx.db_path, sizeof(g_mbtiles_ctx.db_path), "%s", db_path);
    return true;
}

static bool map_mbtiles_bind_xyz(sqlite3_stmt* stmt, int zoom, int tile_x, int tile_y) {
    if (!stmt) return false;

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    if (sqlite3_bind_int(stmt, 1, zoom) != SQLITE_OK) return false;
    if (sqlite3_bind_int(stmt, 2, tile_x) != SQLITE_OK) return false;
    if (sqlite3_bind_int(stmt, 3, xyz_to_mbtiles_row(zoom, tile_y)) != SQLITE_OK) return false;
    return true;
}

static bool map_mbtiles_bind_xyz_with_scheme(sqlite3_stmt* stmt, int zoom, int tile_x, int tile_y, int row_scheme) {
    if (!stmt) return false;

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    if (sqlite3_bind_int(stmt, 1, zoom) != SQLITE_OK) return false;
    if (sqlite3_bind_int(stmt, 2, tile_x) != SQLITE_OK) return false;
    if (sqlite3_bind_int(stmt, 3, xyz_to_mbtiles_row_with_scheme(zoom, tile_y, row_scheme)) != SQLITE_OK) return false;
    return true;
}

bool map_mbtiles_has_tile(const char* db_path, int zoom, int tile_x, int tile_y) {
    if (!map_mbtiles_ensure_db_ready(db_path)) return false;
    if (!map_mbtiles_bind_xyz(g_mbtiles_ctx.stmt_has_tile, zoom, tile_x, tile_y)) return false;

    int rc = sqlite3_step(g_mbtiles_ctx.stmt_has_tile);
    bool found = (rc == SQLITE_ROW);

    if (!found && !g_mbtiles_ctx.row_scheme_from_metadata) {
        int alt_scheme = (g_mbtiles_ctx.row_scheme == 2) ? 1 : 2;
        if (map_mbtiles_bind_xyz_with_scheme(g_mbtiles_ctx.stmt_has_tile, zoom, tile_x, tile_y, alt_scheme)) {
            rc = sqlite3_step(g_mbtiles_ctx.stmt_has_tile);
            found = (rc == SQLITE_ROW);
            if (found) {
                g_mbtiles_ctx.row_scheme = alt_scheme;
            }
        }
    }

    sqlite3_reset(g_mbtiles_ctx.stmt_has_tile);
    return found;
}

bool map_mbtiles_get_tile_blob(const char* db_path, int zoom, int tile_x, int tile_y, unsigned char** data, int* size) {
    if (!db_path || !data || !size) return false;
    *data = NULL;
    *size = 0;

    if (!map_mbtiles_ensure_db_ready(db_path)) return false;
    if (!map_mbtiles_bind_xyz(g_mbtiles_ctx.stmt_get_blob, zoom, tile_x, tile_y)) return false;

    int rc = sqlite3_step(g_mbtiles_ctx.stmt_get_blob);
    if (rc != SQLITE_ROW && !g_mbtiles_ctx.row_scheme_from_metadata) {
        int alt_scheme = (g_mbtiles_ctx.row_scheme == 2) ? 1 : 2;
        if (map_mbtiles_bind_xyz_with_scheme(g_mbtiles_ctx.stmt_get_blob, zoom, tile_x, tile_y, alt_scheme)) {
            rc = sqlite3_step(g_mbtiles_ctx.stmt_get_blob);
            if (rc == SQLITE_ROW) {
                g_mbtiles_ctx.row_scheme = alt_scheme;
            }
        }
    }

    if (rc == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(g_mbtiles_ctx.stmt_get_blob, 0);
        int blob_size = sqlite3_column_bytes(g_mbtiles_ctx.stmt_get_blob, 0);
        if (blob && blob_size > 0) {
            unsigned char* out = (unsigned char*)malloc((size_t)blob_size);
            if (out) {
                memcpy(out, blob, (size_t)blob_size);
                *data = out;
                *size = blob_size;
                sqlite3_reset(g_mbtiles_ctx.stmt_get_blob);
                return true;
            }
        }
    }

    sqlite3_reset(g_mbtiles_ctx.stmt_get_blob);
    return false;
}

void map_mbtiles_free_blob(unsigned char* data) {
    free(data);
}

void map_mbtiles_shutdown(void) {
    map_mbtiles_close_cached_db();
}

#else
bool map_mbtiles_has_tile(const char* db_path, int zoom, int tile_x, int tile_y) {
    (void)db_path;
    (void)zoom;
    (void)tile_x;
    (void)tile_y;
    return false;
}

bool map_mbtiles_get_tile_blob(const char* db_path, int zoom, int tile_x, int tile_y, unsigned char** data, int* size) {
    (void)db_path;
    (void)zoom;
    (void)tile_x;
    (void)tile_y;
    if (data) *data = NULL;
    if (size) *size = 0;
    return false;
}

void map_mbtiles_free_blob(unsigned char* data) {
    (void)data;
}

void map_mbtiles_shutdown(void) {
}
#endif
