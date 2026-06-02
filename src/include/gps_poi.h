#ifndef GPS_POI_H
#define GPS_POI_H

#include <stdbool.h>

#if defined(__has_include)
#  if __has_include(<sqlite3.h>)
#    define GPC_POI_HAVE_SQLITE3 1
#  else
#    define GPC_POI_HAVE_SQLITE3 0
#  endif
#else
#  define GPC_POI_HAVE_SQLITE3 1
#endif

typedef struct {
    int id;
    double latitude;
    double longitude;
    char name[128];
    char category[64];
} poi_item_t;

bool poi_db_init(const char* db_path);
void poi_db_shutdown(void);
int poi_db_count_bbox(const char* db_path,
                      double min_lat,
                      double min_lon,
                      double max_lat,
                      double max_lon);
int poi_db_load_bbox(const char* db_path,
                     double min_lat,
                     double min_lon,
                     double max_lat,
                     double max_lon,
                     poi_item_t* out_items,
                     int max_items);

#endif // GPS_POI_H
