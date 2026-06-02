#include "gps_analysis.h"

#include <string.h>

static int gps_analysis_track_start_index(const map_system_t* map)
{
    if (!map)
    {
        return 0;
    }

    return map->track.point_count < MAX_TRACK_POINTS ? 0 : map->track.current_index;
}

void gps_analysis_compute_track_summary(const map_system_t* map, track_analysis_summary_t* summary)
{
    if (!summary)
    {
        return;
    }

    memset(summary, 0, sizeof(*summary));
    summary->min_altitude_m = 0.0f;
    summary->max_altitude_m = 0.0f;

    if (!map || map->track.point_count <= 0)
    {
        return;
    }

    summary->has_track = true;
    summary->point_count = map->track.point_count;
    summary->total_distance_m = map->track.total_distance;
    summary->total_distance_km = map->track.total_distance / 1000.0;
    summary->max_speed_kmh = map->track.max_speed;

    int start_idx = gps_analysis_track_start_index(map);
    bool altitude_initialized = false;
    time_t first_ts = 0;
    time_t last_ts = 0;

    for (int i = 0; i < map->track.point_count; i++)
    {
        int idx = (start_idx + i) % MAX_TRACK_POINTS;
        const track_point_t* point = &map->track.points[idx];
        if (!point->valid)
        {
            continue;
        }

        if (!altitude_initialized)
        {
            summary->min_altitude_m = point->altitude;
            summary->max_altitude_m = point->altitude;
            altitude_initialized = true;
        }
        else
        {
            if (point->altitude < summary->min_altitude_m)
            {
                summary->min_altitude_m = point->altitude;
            }
            if (point->altitude > summary->max_altitude_m)
            {
                summary->max_altitude_m = point->altitude;
            }
        }

        if (first_ts == 0 || point->timestamp < first_ts)
        {
            first_ts = point->timestamp;
        }
        if (point->timestamp > last_ts)
        {
            last_ts = point->timestamp;
        }
    }

    if (first_ts > 0 && last_ts >= first_ts)
    {
        summary->duration_seconds = (double)(last_ts - first_ts);
        if (summary->duration_seconds > 0.0)
        {
            summary->average_speed_kmh = (summary->total_distance_m / summary->duration_seconds) * 3.6;
        }
    }
}

int gps_analysis_build_speed_time_series(const map_system_t* map, double* elapsed_time_minutes, double* speed_kmh, int max_points)
{
    if (!map || !elapsed_time_minutes || !speed_kmh || max_points <= 0 || map->track.point_count <= 0)
    {
        return 0;
    }

    int start_idx = gps_analysis_track_start_index(map);
    time_t base_ts = 0;
    int out_count = 0;

    for (int i = 0; i < map->track.point_count && out_count < max_points; i++)
    {
        int idx = (start_idx + i) % MAX_TRACK_POINTS;
        const track_point_t* point = &map->track.points[idx];
        if (!point->valid)
        {
            continue;
        }

        if (base_ts == 0)
        {
            base_ts = point->timestamp;
        }

        double elapsed_minutes = 0.0;
        if (base_ts != 0 && point->timestamp >= base_ts)
        {
            elapsed_minutes = (double)(point->timestamp - base_ts) / 60.0;
        }

        elapsed_time_minutes[out_count] = elapsed_minutes;
        speed_kmh[out_count] = (double)point->speed;
        out_count++;
    }

    return out_count;
}

int gps_analysis_build_altitude_distance_series(const map_system_t* map, double* distance_km, double* altitude_m, int max_points)
{
    if (!map || !distance_km || !altitude_m || max_points <= 0 || map->track.point_count <= 0)
    {
        return 0;
    }

    int start_idx = gps_analysis_track_start_index(map);
    int out_count = 0;
    double cumulative_distance_m = 0.0;
    bool has_previous = false;
    double previous_lat = 0.0;
    double previous_lon = 0.0;

    for (int i = 0; i < map->track.point_count && out_count < max_points; i++)
    {
        int idx = (start_idx + i) % MAX_TRACK_POINTS;
        const track_point_t* point = &map->track.points[idx];
        if (!point->valid)
        {
            continue;
        }

        if (has_previous)
        {
            cumulative_distance_m += map_system_calculate_distance(previous_lat,
                                                                   previous_lon,
                                                                   point->latitude,
                                                                   point->longitude);
        }

        distance_km[out_count] = cumulative_distance_m / 1000.0;
        altitude_m[out_count] = (double)point->altitude;
        out_count++;

        previous_lat = point->latitude;
        previous_lon = point->longitude;
        has_previous = true;
    }

    return out_count;
}
