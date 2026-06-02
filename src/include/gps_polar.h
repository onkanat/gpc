//
// Satellite polar view and sky plot for GPS GUI
//
#ifndef GPS_POLAR_H
#define GPS_POLAR_H

#include <stdbool.h>
#include "gps_data.h"

// Polar view configuration
typedef struct {
    float center_x, center_y;    // Center of polar plot
    float radius;                // Radius of outer circle
    bool show_grid;              // Show elevation/azimuth grid
    bool show_labels;            // Show satellite labels
    bool show_snr_colors;        // Color-code by SNR
    float label_font_size;       // Font size for labels
} polar_view_config_t;

// Satellite visualization data
typedef struct {
    int prn;
    float screen_x, screen_y;    // Screen coordinates
    float snr;
    bool used_in_fix;
    bool visible;
    float elevation;
    float azimuth;
} satellite_visual_t;

// Polar view system
typedef struct {
    polar_view_config_t config;
    satellite_visual_t satellites[32];
    int satellite_count;
    
    // Statistics
    int satellites_visible;
    int satellites_used;
    float average_snr;
    float max_snr;
    
    // UI state
    int selected_satellite;      // -1 if none
    bool hover_active;
    float hover_x, hover_y;

    // Dirty state
    bool data_dirty;
    bool layout_dirty;
    time_t last_gps_update;
    int last_input_satellite_count;
    unsigned int dirty_revision;
    
} polar_view_t;

// Function declarations
void polar_view_init(polar_view_t* polar);
void polar_view_cleanup(polar_view_t* polar);
void polar_view_update(polar_view_t* polar, const gps_data_t* gps_data);
void polar_view_set_size(polar_view_t* polar, float center_x, float center_y, float radius);
void polar_view_render(polar_view_t* polar, float canvas_x, float canvas_y, 
                      float canvas_width, float canvas_height);
bool polar_view_handle_mouse(polar_view_t* polar, float mouse_x, float mouse_y, bool clicked);
void polar_view_azimuth_elevation_to_screen(float azimuth, float elevation, 
                                           float center_x, float center_y, float radius,
                                           float* screen_x, float* screen_y);
int polar_view_find_satellite_at_position(const polar_view_t* polar, float x, float y);

#endif // GPS_POLAR_H