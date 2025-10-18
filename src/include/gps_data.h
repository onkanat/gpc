//
// GPS data structures and utilities for GUI application
//
#ifndef GPS_DATA_H
#define GPS_DATA_H

#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include "minmea.h"

// GPS Fix Quality enumeration
typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_GPS = 1,
    GPS_FIX_DGPS = 2,
    GPS_FIX_PPS = 3,
    GPS_FIX_RTK = 4,
    GPS_FIX_FLOAT_RTK = 5,
    GPS_FIX_ESTIMATED = 6,
    GPS_FIX_MANUAL = 7,
    GPS_FIX_SIMULATION = 8
} gps_fix_quality_t;

// GPS Connection Status
typedef enum {
    GPS_STATUS_DISCONNECTED = 0,
    GPS_STATUS_CONNECTING,
    GPS_STATUS_CONNECTED,
    GPS_STATUS_ERROR
} gps_connection_status_t;

// Individual satellite information
typedef struct {
    int prn;              // Satellite PRN number
    int elevation;        // Elevation in degrees
    int azimuth;          // Azimuth in degrees
    int snr;              // Signal-to-noise ratio
    bool used_in_fix;     // Whether satellite is used in fix
} satellite_info_t;

// Main GPS data structure
typedef struct {
    // Connection info
    gps_connection_status_t status;
    char port_name[256];
    int baud_rate;
    
    // Position data
    double latitude;
    double longitude;
    float altitude;
    bool position_valid;
    
    // Time data
    struct minmea_time time;
    struct minmea_date date;
    bool time_valid;
    
    // Motion data
    float speed_knots;
    float speed_kmh;
    float course;
    bool motion_valid;
    
    // Fix quality
    gps_fix_quality_t fix_quality;
    int satellites_used;
    int satellites_visible;
    float hdop;
    float vdop;
    float pdop;
    
    // Satellite data
    satellite_info_t satellites[32];
    int satellite_count;
    
    // Statistics
    unsigned long total_sentences;
    unsigned long valid_sentences;
    unsigned long invalid_sentences;
    time_t last_update;
    
    // Logging
    bool logging_enabled;
    char log_filename[256];
    FILE* log_file;
    
} gps_data_t;

// Function declarations
void gps_data_init(gps_data_t* data);
void gps_data_cleanup(gps_data_t* data);
void gps_data_update_from_rmc(gps_data_t* data, const struct minmea_sentence_rmc* rmc);
void gps_data_update_from_gga(gps_data_t* data, const struct minmea_sentence_gga* gga);
void gps_data_update_from_gsa(gps_data_t* data, const struct minmea_sentence_gsa* gsa);
void gps_data_update_from_gsv(gps_data_t* data, const struct minmea_sentence_gsv* gsv);
const char* gps_fix_quality_to_string(gps_fix_quality_t quality);
const char* gps_status_to_string(gps_connection_status_t status);
bool gps_data_start_logging(gps_data_t* data, const char* filename);
void gps_data_stop_logging(gps_data_t* data);

#endif // GPS_DATA_H