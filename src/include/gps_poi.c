#include "gps_poi.h"

#include <stdio.h>
#include <string.h>

#if defined(__has_include)
#  if __has_include(<sqlite3.h>)
#    include <sqlite3.h>
#  endif
#else
#  include <sqlite3.h>
#endif

#if GPC_POI_HAVE_SQLITE3
typedef struct {
    sqlite3* db;
    sqlite3_stmt* stmt_count_bbox;
    char db_path[512];
} poi_ctx_t;

static poi_ctx_t g_poi_ctx = {NULL, NULL, {0}};

static void poi_close_cached(void) {
    if (g_poi_ctx.stmt_count_bbox) {
        sqlite3_finalize(g_poi_ctx.stmt_count_bbox);
        g_poi_ctx.stmt_count_bbox = NULL;
    }
    if (g_poi_ctx.db) {
        sqlite3_close(g_poi_ctx.db);
        g_poi_ctx.db = NULL;
    }
    g_poi_ctx.db_path[0] = '\0';
}

bool poi_db_init(const char* db_path) {
    if (!db_path || db_path[0] == '\0') return false;

    if (g_poi_ctx.db && strcmp(g_poi_ctx.db_path, db_path) == 0 && g_poi_ctx.stmt_count_bbox) {
        return true;
    }

    poi_close_cached();

    if (sqlite3_open_v2(db_path, &g_poi_ctx.db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        poi_close_cached();
        return false;
    }

    const char* sql_count =
        "SELECT COUNT(*) FROM places "
        "WHERE latitude>=? AND latitude<=? AND longitude>=? AND longitude<=?;";

    if (sqlite3_prepare_v2(g_poi_ctx.db, sql_count, -1, &g_poi_ctx.stmt_count_bbox, NULL) != SQLITE_OK) {
        poi_close_cached();
        return false;
    }

    snprintf(g_poi_ctx.db_path, sizeof(g_poi_ctx.db_path), "%s", db_path);
    return true;
}

void poi_db_shutdown(void) {
    poi_close_cached();
}

int poi_db_count_bbox(const char* db_path,
                      double min_lat,
                      double min_lon,
                      double max_lat,
                      double max_lon) {
    if (!poi_db_init(db_path)) return 0;

    sqlite3_reset(g_poi_ctx.stmt_count_bbox);
    sqlite3_clear_bindings(g_poi_ctx.stmt_count_bbox);

    if (sqlite3_bind_double(g_poi_ctx.stmt_count_bbox, 1, min_lat) != SQLITE_OK) return 0;
    if (sqlite3_bind_double(g_poi_ctx.stmt_count_bbox, 2, max_lat) != SQLITE_OK) return 0;
    if (sqlite3_bind_double(g_poi_ctx.stmt_count_bbox, 3, min_lon) != SQLITE_OK) return 0;
    if (sqlite3_bind_double(g_poi_ctx.stmt_count_bbox, 4, max_lon) != SQLITE_OK) return 0;

    int count = 0;
    if (sqlite3_step(g_poi_ctx.stmt_count_bbox) == SQLITE_ROW) {
        count = sqlite3_column_int(g_poi_ctx.stmt_count_bbox, 0);
    }

    sqlite3_reset(g_poi_ctx.stmt_count_bbox);
    return count;
}

#else
bool poi_db_init(const char* db_path) {
    (void)db_path;
    return false;
}

void poi_db_shutdown(void) {
}

int poi_db_count_bbox(const char* db_path,
                      double min_lat,
                      double min_lon,
                      double max_lat,
                      double max_lon) {
    (void)db_path;
    (void)min_lat;
    (void)min_lon;
    (void)max_lat;
    (void)max_lon;
    return 0;
}
#endif
