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

void map_system_init(map_system_t* map) {
    if (!map) return;
    
    memset(map, 0, sizeof(map_system_t));
    
    // Initialize map view
    map->view.center_lat = 39.0;  // Default center (approximate world center)
    map->view.center_lon = -98.0;
    map->view.zoom_level = 1.0;
    map->view.auto_center = true;
    map->view.show_track = true;
    map->view.show_waypoints = true;
    map->view.track_width = 2.0f;
    map->view.follow_mode = true;
    
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
}

void map_system_cleanup(map_system_t* map) {
    // Nothing to cleanup currently
    (void)map;
}

void map_system_update(map_system_t* map, const gps_data_t* gps_data) {
    if (!map || !gps_data) return;
    
    // Auto-center on GPS position if enabled
    if (map->view.auto_center && gps_data->position_valid) {
        map->view.center_lat = gps_data->latitude;
        map->view.center_lon = gps_data->longitude;
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
}

void map_system_start_recording(map_system_t* map) {
    if (!map) return;
    
    map->track.recording = true;
    map->track.start_time = time(NULL);
    map->track.last_point_time = 0;
}

void map_system_stop_recording(map_system_t* map) {
    if (!map) return;
    
    map->track.recording = false;
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
}

void map_system_set_zoom(map_system_t* map, double zoom) {
    if (!map) return;
    
    // Clamp zoom level
    if (zoom < 0.1) zoom = 0.1;
    if (zoom > 100.0) zoom = 100.0;
    
    map->view.zoom_level = zoom;
    map->zoom_changed = true;
}

void map_system_set_center(map_system_t* map, double lat, double lon) {
    if (!map) return;
    
    map->view.center_lat = lat;
    map->view.center_lon = lon;
    map->view.auto_center = false; // Disable auto-center when manually setting
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

void map_lat_lon_to_screen(const map_view_t* view, double lat, double lon, 
                          float screen_width, float screen_height, 
                          float* screen_x, float* screen_y) {
    if (!view || !screen_x || !screen_y) return;
    
    // Simple projection (can be improved with proper map projection)
    double x_offset = (lon - view->center_lon) * view->zoom_level;
    double y_offset = (lat - view->center_lat) * view->zoom_level;
    
    *screen_x = screen_width * 0.5f + (float)(x_offset * screen_width * 0.01);
    *screen_y = screen_height * 0.5f - (float)(y_offset * screen_height * 0.01);
}

void map_screen_to_lat_lon(const map_view_t* view, float screen_x, float screen_y,
                          float screen_width, float screen_height,
                          double* lat, double* lon) {
    if (!view || !lat || !lon) return;
    
    // Reverse of the screen projection
    double x_offset = (screen_x - screen_width * 0.5f) / (screen_width * 0.01 * view->zoom_level);
    double y_offset = -(screen_y - screen_height * 0.5f) / (screen_height * 0.01 * view->zoom_level);
    
    *lon = view->center_lon + x_offset;
    *lat = view->center_lat + y_offset;
}