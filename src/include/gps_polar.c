//
// Satellite polar view implementation
//
#include "gps_polar.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void polar_view_init(polar_view_t* polar) {
    if (!polar) return;
    
    memset(polar, 0, sizeof(polar_view_t));
    
    // Default configuration
    polar->config.center_x = 150.0f;
    polar->config.center_y = 150.0f;
    polar->config.radius = 140.0f;
    polar->config.show_grid = true;
    polar->config.show_labels = true;
    polar->config.show_snr_colors = true;
    polar->config.label_font_size = 12.0f;
    
    polar->satellite_count = 0;
    polar->satellites_visible = 0;
    polar->satellites_used = 0;
    polar->average_snr = 0.0f;
    polar->max_snr = 0.0f;
    polar->selected_satellite = -1;
    polar->hover_active = false;
}

void polar_view_cleanup(polar_view_t* polar) {
    // Nothing to cleanup currently
    (void)polar;
}

void polar_view_update(polar_view_t* polar, const gps_data_t* gps_data) {
    if (!polar || !gps_data) return;
    
    // Reset counters
    polar->satellite_count = 0;
    polar->satellites_visible = 0;
    polar->satellites_used = 0;
    polar->average_snr = 0.0f;
    polar->max_snr = 0.0f;
    
    // Process satellites from GPS data
    for (int i = 0; i < gps_data->satellite_count && polar->satellite_count < 32; i++) {
        const satellite_info_t* sat = &gps_data->satellites[i];
        
        if (sat->prn <= 0) continue;
        
        satellite_visual_t* vis_sat = &polar->satellites[polar->satellite_count];
        
        vis_sat->prn = sat->prn;
        vis_sat->elevation = sat->elevation;
        vis_sat->azimuth = sat->azimuth;
        vis_sat->snr = sat->snr;
        vis_sat->used_in_fix = sat->used_in_fix;
        vis_sat->visible = (sat->snr > 0 || sat->elevation > 0);
        
        // Convert to screen coordinates
        polar_view_azimuth_elevation_to_screen(
            sat->azimuth, sat->elevation,
            polar->config.center_x, polar->config.center_y, polar->config.radius,
            &vis_sat->screen_x, &vis_sat->screen_y
        );
        
        // Update statistics
        if (vis_sat->visible) {
            polar->satellites_visible++;
            
            if (vis_sat->snr > 0) {
                polar->average_snr += vis_sat->snr;
                if (vis_sat->snr > polar->max_snr) {
                    polar->max_snr = vis_sat->snr;
                }
            }
        }
        
        if (vis_sat->used_in_fix) {
            polar->satellites_used++;
        }
        
        polar->satellite_count++;
    }
    
    // Calculate average SNR
    if (polar->satellites_visible > 0) {
        polar->average_snr /= polar->satellites_visible;
    }
}

void polar_view_set_size(polar_view_t* polar, float center_x, float center_y, float radius) {
    if (!polar) return;
    
    polar->config.center_x = center_x;
    polar->config.center_y = center_y;
    polar->config.radius = radius;
    
    // Recalculate all satellite positions
    for (int i = 0; i < polar->satellite_count; i++) {
        satellite_visual_t* sat = &polar->satellites[i];
        polar_view_azimuth_elevation_to_screen(
            sat->azimuth, sat->elevation,
            center_x, center_y, radius,
            &sat->screen_x, &sat->screen_y
        );
    }
}

void polar_view_azimuth_elevation_to_screen(float azimuth, float elevation, 
                                           float center_x, float center_y, float radius,
                                           float* screen_x, float* screen_y) {
    if (!screen_x || !screen_y) return;
    
    // Convert elevation to radius (0° = outer edge, 90° = center)
    float r = radius * (1.0f - elevation / 90.0f);
    
    // Convert azimuth to angle (0° = north, clockwise)
    // Adjust for screen coordinates (0° = up, clockwise)
    float angle_rad = (azimuth - 90.0f) * M_PI / 180.0f;
    
    *screen_x = center_x + r * cosf(angle_rad);
    *screen_y = center_y + r * sinf(angle_rad);
}

int polar_view_find_satellite_at_position(const polar_view_t* polar, float x, float y) {
    if (!polar) return -1;
    
    const float click_radius = 15.0f; // Pixel tolerance for clicking
    
    for (int i = 0; i < polar->satellite_count; i++) {
        const satellite_visual_t* sat = &polar->satellites[i];
        
        if (!sat->visible) continue;
        
        float dx = x - sat->screen_x;
        float dy = y - sat->screen_y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        if (distance <= click_radius) {
            return i;
        }
    }
    
    return -1;
}

bool polar_view_handle_mouse(polar_view_t* polar, float mouse_x, float mouse_y, bool clicked) {
    if (!polar) return false;
    
    bool handled = false;
    
    // Check if mouse is within polar view area
    float dx = mouse_x - polar->config.center_x;
    float dy = mouse_y - polar->config.center_y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    if (distance <= polar->config.radius + 20.0f) { // Small margin
        polar->hover_active = true;
        polar->hover_x = mouse_x;
        polar->hover_y = mouse_y;
        handled = true;
        
        if (clicked) {
            // Find satellite at click position
            int sat_index = polar_view_find_satellite_at_position(polar, mouse_x, mouse_y);
            if (sat_index >= 0) {
                polar->selected_satellite = (polar->selected_satellite == sat_index) ? -1 : sat_index;
            } else {
                polar->selected_satellite = -1;
            }
        }
    } else {
        polar->hover_active = false;
        if (clicked) {
            polar->selected_satellite = -1;
        }
    }
    
    return handled;
}