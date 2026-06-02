#ifndef GPS_ANALYSIS_H
#define GPS_ANALYSIS_H

#include <stdbool.h>
#include "gps_map.h"

typedef struct {
    bool has_track;
    int point_count;
    double total_distance_m;
    double total_distance_km;
    double duration_seconds;
    double average_speed_kmh;
    float max_speed_kmh;
    float min_altitude_m;
    float max_altitude_m;
} track_analysis_summary_t;

void gps_analysis_compute_track_summary(const map_system_t* map, track_analysis_summary_t* summary);
int gps_analysis_build_speed_time_series(const map_system_t* map, double* elapsed_time_minutes, double* speed_kmh, int max_points);
int gps_analysis_build_altitude_distance_series(const map_system_t* map, double* distance_km, double* altitude_m, int max_points);

#endif // GPS_ANALYSIS_H
