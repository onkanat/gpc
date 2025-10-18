//
// GPS Compass module implementation
//
#include "gps_compass.h"
#include <math.h>
#include <stdio.h>

bool compass_init(compass_t* compass) {
    if (!compass) return false;
    
    compass->heading = 0.0f;
    compass->declination = 0.0f;
    compass->auto_rotate = true;
    compass->center_x = 0.0f;
    compass->center_y = 0.0f;
    compass->radius = 100.0f;
    
    return true;
}

void compass_cleanup(compass_t* compass) {
    if (!compass) return;
    // Currently no cleanup needed
}

void compass_update(compass_t* compass, const gps_data_t* gps_data) {
    if (!compass || !gps_data) return;
    
    // Update compass heading based on GPS course
    if (compass->auto_rotate && gps_data->motion_valid && gps_data->speed_kmh > 2.0f) {
        compass->heading = gps_data->course;
    }
    
    // TODO: Auto-calculate magnetic declination based on GPS position
    // For now, declination is set manually via compass_set_declination()
}

void compass_set_display(compass_t* compass, float center_x, float center_y, float radius) {
    if (!compass) return;
    
    compass->center_x = center_x;
    compass->center_y = center_y;
    compass->radius = radius;
}

float compass_get_true_heading(const compass_t* compass) {
    if (!compass) return 0.0f;
    
    float true_heading = compass->heading + compass->declination;
    
    // Normalize to 0-360 degrees
    while (true_heading < 0.0f) true_heading += 360.0f;
    while (true_heading >= 360.0f) true_heading -= 360.0f;
    
    return true_heading;
}

void compass_set_declination(compass_t* compass, float declination) {
    if (!compass) return;
    
    // Clamp declination to reasonable bounds (-180 to +180)
    if (declination < -180.0f) declination = -180.0f;
    if (declination > 180.0f) declination = 180.0f;
    
    compass->declination = declination;
}

float compass_calculate_declination(double latitude, double longitude) {
    // TODO: Implement magnetic declination calculation using WMM (World Magnetic Model)
    // This is a complex calculation that requires magnetic field coefficients
    // For now, return 0 and require manual setting
    
    (void)latitude;   // Suppress unused parameter warning
    (void)longitude;  // Suppress unused parameter warning
    
    return 0.0f;
}