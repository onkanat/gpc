//
// Advanced map rendering and track management for GPS GUI
//
#ifndef GPS_MAP_H
#define GPS_MAP_H

#include <stdbool.h>
#include <time.h>
#include "gps_data.h"

#define MAX_TRACK_POINTS 10000
#define MAX_WAYPOINTS 1000

// Track point for route history
typedef struct {
    double latitude;
    double longitude;
    float altitude;
    time_t timestamp;
    float speed;
    float course;
    bool valid;
} track_point_t;

// Waypoint for navigation
typedef struct {
    double latitude;
    double longitude;
    char name[64];
    char description[256];
    time_t created;
    bool active;
} waypoint_t;

// Map view settings
typedef struct {
    double center_lat;
    double center_lon;
    double zoom_level;        // 1.0 = default, > 1.0 = zoomed in
    bool auto_center;         // Auto-center on GPS position
    bool show_track;          // Show track history
    bool show_waypoints;      // Show waypoints
    float track_width;        // Track line width
    bool follow_mode;         // Auto-follow GPS position
    bool offline_only;        // Disable online tile attempts
    bool prefer_mbtiles;      // Read MBTiles before disk tiles
} map_view_t;

// Track data management
typedef struct {
    track_point_t points[MAX_TRACK_POINTS];
    int point_count;
    int current_index;        // Circular buffer index
    bool recording;
    time_t start_time;
    time_t last_point_time;
    double total_distance;    // Total track distance in meters
    float min_point_distance; // Minimum distance between points (meters)
    float max_speed;          // Maximum recorded speed
} track_data_t;

// Waypoint management
typedef struct {
    waypoint_t waypoints[MAX_WAYPOINTS];
    int waypoint_count;
    int selected_waypoint;    // -1 if none selected
} waypoint_data_t;

// Complete map system
typedef struct {
    map_view_t view;
    track_data_t track;
    waypoint_data_t waypoints;
    
    // UI state
    bool pan_active;
    float last_mouse_x, last_mouse_y;
    bool zoom_changed;
    
    // Statistics
    double track_bounds_min_lat, track_bounds_max_lat;
    double track_bounds_min_lon, track_bounds_max_lon;

    // Dirty flag state
    bool dirty;
    unsigned int dirty_revision;
} map_system_t;

// Function declarations
void map_system_init(map_system_t* map);
void map_system_cleanup(map_system_t* map);
void map_system_update(map_system_t* map, const gps_data_t* gps_data);
void map_system_add_track_point(map_system_t* map, const gps_data_t* gps_data);
void map_system_start_recording(map_system_t* map);
void map_system_stop_recording(map_system_t* map);
void map_system_clear_track(map_system_t* map);
void map_system_set_zoom(map_system_t* map, double zoom);
void map_system_set_center(map_system_t* map, double lat, double lon);
void map_system_zoom_to_fit_track(map_system_t* map);
int map_system_add_waypoint(map_system_t* map, double lat, double lon, const char* name);
void map_system_remove_waypoint(map_system_t* map, int index);
bool map_system_save_gpx(const map_system_t* map, const char* filename);
bool map_system_load_gpx(map_system_t* map, const char* filename);
bool map_system_export_track_csv(const map_system_t* map, const char* filename);
double map_system_calculate_distance(double lat1, double lon1, double lat2, double lon2);
bool map_system_is_dirty(const map_system_t* map);
unsigned int map_system_get_dirty_revision(const map_system_t* map);
void map_system_clear_dirty(map_system_t* map);

// Coordinate conversion functions
void map_lat_lon_to_screen(const map_view_t* view, double lat, double lon, 
                          float screen_width, float screen_height, 
                          float* screen_x, float* screen_y);
void map_screen_to_lat_lon(const map_view_t* view, float screen_x, float screen_y,
                          float screen_width, float screen_height,
                          double* lat, double* lon);

#endif // GPS_MAP_H