//
// GPS Compass module - Digital compass and direction display
//
#ifndef GPS_COMPASS_H
#define GPS_COMPASS_H

#include "gps_data.h"

typedef struct {
    float heading;           // Current heading in degrees (0-360)
    float declination;       // Magnetic declination
    bool auto_rotate;        // Auto-rotate with GPS heading
    float center_x;          // Display center X coordinate
    float center_y;          // Display center Y coordinate  
    float radius;            // Display radius
} compass_t;

// Initialize compass system
bool compass_init(compass_t* compass);

// Cleanup compass system
void compass_cleanup(compass_t* compass);

// Update compass data with GPS information
void compass_update(compass_t* compass, const gps_data_t* gps_data);

// Set display parameters for compass rendering
void compass_set_display(compass_t* compass, float center_x, float center_y, float radius);

// Get current true heading (GPS heading + magnetic declination)
float compass_get_true_heading(const compass_t* compass);

// Set magnetic declination manually
void compass_set_declination(compass_t* compass, float declination);

// Calculate magnetic declination based on position (future implementation)
float compass_calculate_declination(double latitude, double longitude);

#endif // GPS_COMPASS_H