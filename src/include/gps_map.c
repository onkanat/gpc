//
// GPS map system implementation
//
#include "gps_map.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Earth radius in meters
#define EARTH_RADIUS 6371000.0
#define WEB_MERCATOR_MAX_LAT 85.05112878
#define TILE_SIZE_PX 256.0

static double clamp_lat_mercator(double lat) {
    if (lat > WEB_MERCATOR_MAX_LAT) return WEB_MERCATOR_MAX_LAT;
    if (lat < -WEB_MERCATOR_MAX_LAT) return -WEB_MERCATOR_MAX_LAT;
    return lat;
}

static double normalize_lon(double lon) {
    while (lon < -180.0) lon += 360.0;
    while (lon > 180.0) lon -= 360.0;
    return lon;
}

static int view_zoom_to_slippy_zoom(double view_zoom) {
    double normalized = (view_zoom <= 0.0) ? 0.1 : view_zoom;
    int zoom = (int)floor(log2(normalized) + 12.0);
    if (zoom < 0) zoom = 0;
    if (zoom > 19) zoom = 19;
    return zoom;
}

static double lon_to_world_x(double lon) {
    return (normalize_lon(lon) + 180.0) / 360.0;
}

static double lat_to_world_y(double lat) {
    double clamped_lat = clamp_lat_mercator(lat);
    double lat_rad = clamped_lat * M_PI / 180.0;
    return (1.0 - asinh(tan(lat_rad)) / M_PI) / 2.0;
}

static double world_x_to_lon(double x) {
    return normalize_lon(x * 360.0 - 180.0);
}

static double world_y_to_lat(double y) {
    double n = M_PI * (1.0 - 2.0 * y);
    return atan(sinh(n)) * 180.0 / M_PI;
}

static void map_system_mark_dirty(map_system_t* map) {
    if (!map) return;
    map->dirty = true;
    map->dirty_revision++;
}

static bool parse_gpx_attr_double(const char* line, const char* attr_name, double* out_value)
{
    if (!line || !attr_name || !out_value)
    {
        return false;
    }

    char pattern[32];
    snprintf(pattern, sizeof(pattern), "%s=\"", attr_name);
    const char* pos = strstr(line, pattern);
    if (!pos)
    {
        return false;
    }

    pos += strlen(pattern);
    char* endptr = NULL;
    double value = strtod(pos, &endptr);
    if (endptr == pos)
    {
        return false;
    }

    *out_value = value;
    return true;
}

static time_t parse_gpx_time_utc(const char* text)
{
    if (!text)
    {
        return 0;
    }

    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (sscanf(text, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
    {
        return 0;
    }

    struct tm tm_utc;
    memset(&tm_utc, 0, sizeof(tm_utc));
    tm_utc.tm_year = year - 1900;
    tm_utc.tm_mon = month - 1;
    tm_utc.tm_mday = day;
    tm_utc.tm_hour = hour;
    tm_utc.tm_min = minute;
    tm_utc.tm_sec = second;

#if defined(_WIN32)
    return _mkgmtime(&tm_utc);
#else
    return timegm(&tm_utc);
#endif
}

void map_system_init(map_system_t* map) {
    if (!map) return;
    
    memset(map, 0, sizeof(map_system_t));
    
    // Initialize map view
    map->view.center_lat = 39.0;  // Default center (approximate world center)
    map->view.center_lon = -98.0;
    map->view.zoom_level = 10.0; // 1.0'dan 10.0'a yükseltildi
    map->view.auto_center = true;
    map->view.show_track = true;
    map->view.show_waypoints = true;
    map->view.track_width = 2.0f;
    map->view.follow_mode = true;
    map->view.offline_only = true;
    map->view.prefer_mbtiles = false;
    
    // Initialize track
    map->track.point_count = 0;
    map->track.current_index = 0;
    map->track.recording = false;
    map->track.start_time = 0;
    map->track.last_point_time = 0;
    map->track.total_distance = 0.0;
    map->track.min_point_distance = 5.0f; // 5 meters minimum
    map->track.max_speed = 0.0f;
    
    // Initialize waypoints
    map->waypoints.waypoint_count = 0;
    map->waypoints.selected_waypoint = -1;
    
    // Initialize UI state
    map->pan_active = false;
    map->zoom_changed = false;
    
    // Initialize bounds
    map->track_bounds_min_lat = 90.0;
    map->track_bounds_max_lat = -90.0;
    map->track_bounds_min_lon = 180.0;
    map->track_bounds_max_lon = -180.0;
    map->dirty = true;
    map->dirty_revision = 1;
}

void map_system_cleanup(map_system_t* map) {
    // Nothing to cleanup currently
    (void)map;
}

void map_system_update(map_system_t* map, const gps_data_t* gps_data) {
    if (!map || !gps_data) return;
    
    // Auto-center on GPS position if enabled
    if (map->view.auto_center && gps_data->position_valid) {
        if (map->view.center_lat != gps_data->latitude || map->view.center_lon != gps_data->longitude) {
            map->view.center_lat = gps_data->latitude;
            map->view.center_lon = gps_data->longitude;
            map_system_mark_dirty(map);
        }
    }
    
    // Add track point if recording and position is valid
    if (map->track.recording && gps_data->position_valid) {
        map_system_add_track_point(map, gps_data);
    }
}

void map_system_add_track_point(map_system_t* map, const gps_data_t* gps_data) {
    if (!map || !gps_data || !gps_data->position_valid) return;
    
    time_t now = time(NULL);
    
    // Check if enough time/distance has passed since last point
    if (map->track.point_count > 0) {
        track_point_t* last_point = &map->track.points[(map->track.current_index - 1 + MAX_TRACK_POINTS) % MAX_TRACK_POINTS];
        
        double distance = map_system_calculate_distance(
            last_point->latitude, last_point->longitude,
            gps_data->latitude, gps_data->longitude
        );
        
        // Skip if too close or too recent
        if (distance < map->track.min_point_distance && 
            (now - map->track.last_point_time) < 1) {
            return;
        }
        
        map->track.total_distance += distance;
    }
    
    // Add new point
    track_point_t* point = &map->track.points[map->track.current_index];
    point->latitude = gps_data->latitude;
    point->longitude = gps_data->longitude;
    point->altitude = gps_data->altitude;
    point->timestamp = now;
    point->speed = gps_data->speed_kmh;
    point->course = gps_data->course;
    point->valid = true;
    
    // Update bounds
    if (gps_data->latitude < map->track_bounds_min_lat) 
        map->track_bounds_min_lat = gps_data->latitude;
    if (gps_data->latitude > map->track_bounds_max_lat) 
        map->track_bounds_max_lat = gps_data->latitude;
    if (gps_data->longitude < map->track_bounds_min_lon) 
        map->track_bounds_min_lon = gps_data->longitude;
    if (gps_data->longitude > map->track_bounds_max_lon) 
        map->track_bounds_max_lon = gps_data->longitude;
    
    // Update statistics
    if (gps_data->speed_kmh > map->track.max_speed) {
        map->track.max_speed = gps_data->speed_kmh;
    }
    
    // Advance circular buffer
    map->track.current_index = (map->track.current_index + 1) % MAX_TRACK_POINTS;
    if (map->track.point_count < MAX_TRACK_POINTS) {
        map->track.point_count++;
    }
    
    map->track.last_point_time = now;
    map_system_mark_dirty(map);
}

void map_system_start_recording(map_system_t* map) {
    if (!map) return;
    
    map->track.recording = true;
    map->track.start_time = time(NULL);
    map->track.last_point_time = 0;
    map_system_mark_dirty(map);
}

void map_system_stop_recording(map_system_t* map) {
    if (!map) return;
    
    map->track.recording = false;
    map_system_mark_dirty(map);
}

void map_system_clear_track(map_system_t* map) {
    if (!map) return;
    
    map->track.point_count = 0;
    map->track.current_index = 0;
    map->track.total_distance = 0.0;
    map->track.max_speed = 0.0f;
    
    // Reset bounds
    map->track_bounds_min_lat = 90.0;
    map->track_bounds_max_lat = -90.0;
    map->track_bounds_min_lon = 180.0;
    map->track_bounds_max_lon = -180.0;
    map_system_mark_dirty(map);
}

void map_system_set_zoom(map_system_t* map, double zoom) {
    if (!map) return;
    
    // Clamp zoom level
    if (zoom < 0.1) zoom = 0.1;
    if (zoom > 100.0) zoom = 100.0;
    
    if (map->view.zoom_level != zoom) {
        map->view.zoom_level = zoom;
        map->zoom_changed = true;
        map_system_mark_dirty(map);
    }
}

void map_system_set_center(map_system_t* map, double lat, double lon) {
    if (!map) return;
    
    if (map->view.center_lat != lat || map->view.center_lon != lon || map->view.auto_center) {
        map->view.center_lat = lat;
        map->view.center_lon = lon;
        map->view.auto_center = false; // Disable auto-center when manually setting
        map_system_mark_dirty(map);
    }
}

void map_system_zoom_to_fit_track(map_system_t* map) {
    if (!map || map->track.point_count == 0) return;
    
    // Calculate center
    double center_lat = (map->track_bounds_min_lat + map->track_bounds_max_lat) / 2.0;
    double center_lon = (map->track_bounds_min_lon + map->track_bounds_max_lon) / 2.0;
    
    // Calculate required zoom to fit all points
    double lat_span = map->track_bounds_max_lat - map->track_bounds_min_lat;
    double lon_span = map->track_bounds_max_lon - map->track_bounds_min_lon;
    
    // Simple zoom calculation (can be improved)
    double zoom = 1.0 / fmax(lat_span / 0.1, lon_span / 0.1);
    if (zoom < 0.1) zoom = 0.1;
    if (zoom > 10.0) zoom = 10.0;
    
    map->view.center_lat = center_lat;
    map->view.center_lon = center_lon;
    map->view.zoom_level = zoom;
    map->view.auto_center = false;
    map_system_mark_dirty(map);
}

int map_system_add_waypoint(map_system_t* map, double lat, double lon, const char* name) {
    if (!map || map->waypoints.waypoint_count >= MAX_WAYPOINTS) return -1;
    
    waypoint_t* wp = &map->waypoints.waypoints[map->waypoints.waypoint_count];
    wp->latitude = lat;
    wp->longitude = lon;
    strncpy(wp->name, name ? name : "Waypoint", sizeof(wp->name) - 1);
    wp->name[sizeof(wp->name) - 1] = '\0';
    snprintf(wp->description, sizeof(wp->description), "Added at %.6f, %.6f", lat, lon);
    wp->created = time(NULL);
    wp->active = true;
    
    map_system_mark_dirty(map);
    return map->waypoints.waypoint_count++;
}

void map_system_remove_waypoint(map_system_t* map, int index) {
    if (!map || index < 0 || index >= map->waypoints.waypoint_count) return;
    
    // Shift remaining waypoints
    for (int i = index; i < map->waypoints.waypoint_count - 1; i++) {
        map->waypoints.waypoints[i] = map->waypoints.waypoints[i + 1];
    }
    
    map->waypoints.waypoint_count--;
    
    if (map->waypoints.selected_waypoint >= map->waypoints.waypoint_count) {
        map->waypoints.selected_waypoint = -1;
    }
    map_system_mark_dirty(map);
}

bool map_system_save_gpx(const map_system_t* map, const char* filename) {
    if (!map || !filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) return false;
    
    // GPX header
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(file, "<gpx version=\"1.1\" creator=\"GPC GPS Console\" ");
    fprintf(file, "xmlns=\"http://www.topografix.com/GPX/1/1\">\n");
    
    // Metadata
    fprintf(file, "  <metadata>\n");
    fprintf(file, "    <name>GPS Track</name>\n");
    fprintf(file, "    <desc>Track recorded by GPC GPS Console</desc>\n");
    
    if (map->track.start_time > 0) {
        char time_str[32];
        struct tm* tm_info = gmtime(&map->track.start_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tm_info);
        fprintf(file, "    <time>%s</time>\n", time_str);
    }
    
    fprintf(file, "  </metadata>\n");
    
    // Waypoints
    for (int i = 0; i < map->waypoints.waypoint_count; i++) {
        const waypoint_t* wp = &map->waypoints.waypoints[i];
        if (!wp->active) continue;
        
        fprintf(file, "  <wpt lat=\"%.8f\" lon=\"%.8f\">\n", wp->latitude, wp->longitude);
        fprintf(file, "    <name>%s</name>\n", wp->name);
        fprintf(file, "    <desc>%s</desc>\n", wp->description);
        fprintf(file, "  </wpt>\n");
    }
    
    // Track
    if (map->track.point_count > 0) {
        fprintf(file, "  <trk>\n");
        fprintf(file, "    <name>GPS Track</name>\n");
        fprintf(file, "    <trkseg>\n");
        
        // Output points in chronological order
        int start_idx = map->track.point_count < MAX_TRACK_POINTS ? 0 : map->track.current_index;
        for (int i = 0; i < map->track.point_count; i++) {
            int idx = (start_idx + i) % MAX_TRACK_POINTS;
            const track_point_t* point = &map->track.points[idx];
            
            if (!point->valid) continue;
            
            fprintf(file, "      <trkpt lat=\"%.8f\" lon=\"%.8f\">\n", 
                   point->latitude, point->longitude);
            fprintf(file, "        <ele>%.1f</ele>\n", point->altitude);
            
            // Convert timestamp to ISO format
            char time_str[32];
            struct tm* tm_info = gmtime(&point->timestamp);
            strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tm_info);
            fprintf(file, "        <time>%s</time>\n", time_str);
            
            fprintf(file, "      </trkpt>\n");
        }
        
        fprintf(file, "    </trkseg>\n");
        fprintf(file, "  </trk>\n");
    }
    
    fprintf(file, "</gpx>\n");
    fclose(file);
    
    return true;
}

bool map_system_load_gpx(map_system_t* map, const char* filename)
{
    if (!map || !filename)
    {
        return false;
    }

    FILE* file = fopen(filename, "r");
    if (!file)
    {
        return false;
    }

    map_system_clear_track(map);

    char line[1024];
    bool in_trkpt = false;
    double current_lat = 0.0;
    double current_lon = 0.0;
    double current_ele = 0.0;
    time_t current_time = 0;
    bool has_lat_lon = false;
    bool has_ele = false;
    bool has_time = false;
    int imported_count = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (strstr(line, "<trkpt") != NULL)
        {
            in_trkpt = true;
            has_lat_lon = parse_gpx_attr_double(line, "lat", &current_lat) &&
                          parse_gpx_attr_double(line, "lon", &current_lon);
            has_ele = false;
            has_time = false;
            current_ele = 0.0;
            current_time = 0;
            continue;
        }

        if (in_trkpt && strstr(line, "<ele>") != NULL)
        {
            char* begin = strstr(line, "<ele>");
            char* end = strstr(line, "</ele>");
            if (begin && end && end > begin)
            {
                begin += 5;
                current_ele = strtod(begin, NULL);
                has_ele = true;
            }
            continue;
        }

        if (in_trkpt && strstr(line, "<time>") != NULL)
        {
            char* begin = strstr(line, "<time>");
            char* end = strstr(line, "</time>");
            if (begin && end && end > begin)
            {
                begin += 6;
                char time_buf[64];
                size_t len = (size_t)(end - begin);
                if (len >= sizeof(time_buf))
                {
                    len = sizeof(time_buf) - 1;
                }
                memcpy(time_buf, begin, len);
                time_buf[len] = '\0';
                current_time = parse_gpx_time_utc(time_buf);
                has_time = (current_time != 0);
            }
            continue;
        }

        if (in_trkpt && strstr(line, "</trkpt>") != NULL)
        {
            if (has_lat_lon && map->track.point_count < MAX_TRACK_POINTS)
            {
                track_point_t* point = &map->track.points[map->track.current_index];
                point->latitude = current_lat;
                point->longitude = current_lon;
                point->altitude = has_ele ? (float)current_ele : 0.0f;
                point->timestamp = has_time ? current_time : time(NULL);
                point->speed = 0.0f;
                point->course = 0.0f;
                point->valid = true;

                if (map->track.point_count > 0)
                {
                    int prev_idx = (map->track.current_index - 1 + MAX_TRACK_POINTS) % MAX_TRACK_POINTS;
                    track_point_t* prev = &map->track.points[prev_idx];
                    map->track.total_distance += map_system_calculate_distance(prev->latitude,
                                                                               prev->longitude,
                                                                               point->latitude,
                                                                               point->longitude);

                    if (point->timestamp > prev->timestamp)
                    {
                        double dt = difftime(point->timestamp, prev->timestamp);
                        if (dt > 0.0)
                        {
                            double d = map_system_calculate_distance(prev->latitude,
                                                                     prev->longitude,
                                                                     point->latitude,
                                                                     point->longitude);
                            point->speed = (float)((d / dt) * 3.6);
                            if (point->speed > map->track.max_speed)
                            {
                                map->track.max_speed = point->speed;
                            }
                        }
                    }
                }

                if (point->latitude < map->track_bounds_min_lat) map->track_bounds_min_lat = point->latitude;
                if (point->latitude > map->track_bounds_max_lat) map->track_bounds_max_lat = point->latitude;
                if (point->longitude < map->track_bounds_min_lon) map->track_bounds_min_lon = point->longitude;
                if (point->longitude > map->track_bounds_max_lon) map->track_bounds_max_lon = point->longitude;

                map->track.current_index = (map->track.current_index + 1) % MAX_TRACK_POINTS;
                map->track.point_count++;
                imported_count++;
            }

            in_trkpt = false;
        }
    }

    fclose(file);

    if (imported_count <= 0)
    {
        return false;
    }

    map->track.recording = false;
    map->track.start_time = map->track.points[0].timestamp;
    map->track.last_point_time = map->track.points[(map->track.current_index - 1 + MAX_TRACK_POINTS) % MAX_TRACK_POINTS].timestamp;
    map_system_mark_dirty(map);
    return true;
}

bool map_system_export_track_csv(const map_system_t* map, const char* filename)
{
    if (!map || !filename || map->track.point_count <= 0)
    {
        return false;
    }

    FILE* file = fopen(filename, "w");
    if (!file)
    {
        return false;
    }

    fprintf(file, "index,timestamp_iso,elapsed_seconds,latitude,longitude,altitude_m,speed_kmh,course_deg,distance_m\\n");

    int start_idx = map->track.point_count < MAX_TRACK_POINTS ? 0 : map->track.current_index;
    time_t base_ts = 0;
    double cumulative_distance = 0.0;
    bool has_prev = false;
    double prev_lat = 0.0;
    double prev_lon = 0.0;

    for (int i = 0; i < map->track.point_count; i++)
    {
        int idx = (start_idx + i) % MAX_TRACK_POINTS;
        const track_point_t* p = &map->track.points[idx];
        if (!p->valid)
        {
            continue;
        }

        if (base_ts == 0)
        {
            base_ts = p->timestamp;
        }

        if (has_prev)
        {
            cumulative_distance += map_system_calculate_distance(prev_lat, prev_lon, p->latitude, p->longitude);
        }

        char time_str[32] = "";
        struct tm* tm_info = gmtime(&p->timestamp);
        if (tm_info)
        {
            strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tm_info);
        }

        double elapsed = 0.0;
        if (base_ts > 0 && p->timestamp >= base_ts)
        {
            elapsed = difftime(p->timestamp, base_ts);
        }

        fprintf(file,
                "%d,%s,%.0f,%.8f,%.8f,%.2f,%.2f,%.2f,%.2f\\n",
                i,
                time_str,
                elapsed,
                p->latitude,
                p->longitude,
                p->altitude,
                p->speed,
                p->course,
                cumulative_distance);

        prev_lat = p->latitude;
        prev_lon = p->longitude;
        has_prev = true;
    }

    fclose(file);
    return true;
}

double map_system_calculate_distance(double lat1, double lon1, double lat2, double lon2) {
    // Haversine formula
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon/2) * sin(dLon/2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return EARTH_RADIUS * c;
}

bool map_system_is_dirty(const map_system_t* map) {
    return map ? map->dirty : false;
}

unsigned int map_system_get_dirty_revision(const map_system_t* map) {
    return map ? map->dirty_revision : 0U;
}

void map_system_clear_dirty(map_system_t* map) {
    if (!map) return;
    map->dirty = false;
}

void map_lat_lon_to_screen(const map_view_t* view, double lat, double lon,
                           float screen_width, float screen_height,
                           float* screen_x, float* screen_y)
{
    if (!view || !screen_x || !screen_y) return;

    int slippy_zoom = view_zoom_to_slippy_zoom(view->zoom_level);
    double world_px = TILE_SIZE_PX * (double)(1 << slippy_zoom);

    double center_x = lon_to_world_x(view->center_lon);
    double center_y = lat_to_world_y(view->center_lat);
    double point_x = lon_to_world_x(lon);
    double point_y = lat_to_world_y(lat);

    double dx_world = point_x - center_x;
    if (dx_world > 0.5) dx_world -= 1.0;
    if (dx_world < -0.5) dx_world += 1.0;

    double dy_world = point_y - center_y;

    *screen_x = screen_width * 0.5f + (float)(dx_world * world_px);
    *screen_y = screen_height * 0.5f + (float)(dy_world * world_px);
}

void map_screen_to_lat_lon(const map_view_t* view, float screen_x, float screen_y,
                           float screen_width, float screen_height,
                           double* lat, double* lon)
{
    if (!view || !lat || !lon) return;

    int slippy_zoom = view_zoom_to_slippy_zoom(view->zoom_level);
    double world_px = TILE_SIZE_PX * (double)(1 << slippy_zoom);

    double center_x = lon_to_world_x(view->center_lon);
    double center_y = lat_to_world_y(view->center_lat);

    double dx_px = (double)screen_x - (double)screen_width * 0.5;
    double dy_px = (double)screen_y - (double)screen_height * 0.5;

    double x_world = center_x + (dx_px / world_px);
    double y_world = center_y + (dy_px / world_px);

    while (x_world < 0.0) x_world += 1.0;
    while (x_world > 1.0) x_world -= 1.0;
    if (y_world < 0.0) y_world = 0.0;
    if (y_world > 1.0) y_world = 1.0;

    *lat = world_y_to_lat(y_world);
    *lon = world_x_to_lon(x_world);
}