//
// GPS data management implementation
//
#include "gps_data.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void gps_data_init(gps_data_t* data) {
    memset(data, 0, sizeof(gps_data_t));
    
    data->status = GPS_STATUS_DISCONNECTED;
    strcpy(data->port_name, "/dev/tty.usbmodem1201");
    data->baud_rate = 9600;
    
    data->latitude = 0.0;
    data->longitude = 0.0;
    data->altitude = 0.0;
    data->position_valid = false;
    
    data->time.hours = -1;
    data->time.minutes = -1;
    data->time.seconds = -1;
    data->time.microseconds = 0;
    data->date.day = -1;
    data->date.month = -1;
    data->date.year = -1;
    data->time_valid = false;
    
    data->speed_knots = 0.0f;
    data->speed_kmh = 0.0f;
    data->course = 0.0f;
    data->motion_valid = false;
    
    data->fix_quality = GPS_FIX_NONE;
    data->satellites_used = 0;
    data->satellites_visible = 0;
    data->hdop = 0.0f;
    data->vdop = 0.0f;
    data->pdop = 0.0f;
    
    data->satellite_count = 0;
    
    data->total_sentences = 0;
    data->valid_sentences = 0;
    data->invalid_sentences = 0;
    data->last_update = 0;
    
    data->logging_enabled = false;
    data->log_filename[0] = '\0';
    data->log_file = NULL;
}

void gps_data_cleanup(gps_data_t* data) {
    if (data->log_file) {
        fclose(data->log_file);
        data->log_file = NULL;
    }
}

void gps_data_update_from_rmc(gps_data_t* data, const struct minmea_sentence_rmc* rmc) {
    if (!data || !rmc) return;
    
    data->total_sentences++;
    
    if (rmc->valid) {
        data->valid_sentences++;
        
        // Position
        data->latitude = minmea_tocoord(&rmc->latitude);
        data->longitude = minmea_tocoord(&rmc->longitude);
        data->position_valid = true;
        
        // Time and date
        data->time = rmc->time;
        data->date = rmc->date;
        data->time_valid = true;
        
        // Motion
        data->speed_knots = minmea_tofloat(&rmc->speed);
        data->speed_kmh = data->speed_knots * 1.852f; // Convert knots to km/h
        data->course = minmea_tofloat(&rmc->course);
        data->motion_valid = true;
        
        data->last_update = time(NULL);
    } else {
        data->invalid_sentences++;
        // RMC geçersizse yalnızca hareket/zaman verilerini sıfırla;
        // GGA'dan gelen position_valid değerine dokunma
        data->motion_valid = false;
        data->time_valid = false;
    }
}

void gps_data_update_from_gga(gps_data_t* data, const struct minmea_sentence_gga* gga) {
    if (!data || !gga) return;
    
    data->total_sentences++;
    data->valid_sentences++;
    
    // Fix quality (parse before position so position_valid is set correctly)
    data->fix_quality = (gps_fix_quality_t)gga->fix_quality;
    data->satellites_used = gga->satellites_tracked;
    data->hdop = minmea_tofloat(&gga->hdop);

    // Position - only mark valid when GGA reports an actual fix
    if (gga->fix_quality > 0) {
        data->latitude = minmea_tocoord(&gga->latitude);
        data->longitude = minmea_tocoord(&gga->longitude);
        data->altitude = minmea_tofloat(&gga->altitude);
        data->position_valid = true;
    } else {
        data->position_valid = false;
    }

    data->last_update = time(NULL);
}

void gps_data_update_from_gsa(gps_data_t* data, const struct minmea_sentence_gsa* gsa) {
    if (!data || !gsa) return;
    
    data->total_sentences++;
    data->valid_sentences++;
    
    // DOP values
    data->pdop = minmea_tofloat(&gsa->pdop);
    data->hdop = minmea_tofloat(&gsa->hdop);
    data->vdop = minmea_tofloat(&gsa->vdop);
    
    data->last_update = time(NULL);
}

void gps_data_update_from_gsv(gps_data_t* data, const struct minmea_sentence_gsv* gsv) {
    if (!data || !gsv) return;
    
    data->total_sentences++;
    data->valid_sentences++;
    
    data->satellites_visible = gsv->total_sats;
    
    // Update satellite information
    for (int i = 0; i < 4; i++) {
        if (gsv->sats[i].nr > 0 && data->satellite_count < 32) {
            int idx = -1;
            // Find existing satellite or add new one
            for (int j = 0; j < data->satellite_count; j++) {
                if (data->satellites[j].prn == gsv->sats[i].nr) {
                    idx = j;
                    break;
                }
            }
            if (idx == -1 && data->satellite_count < 32) {
                idx = data->satellite_count++;
            }
            
            if (idx >= 0) {
                data->satellites[idx].prn = gsv->sats[i].nr;
                data->satellites[idx].elevation = gsv->sats[i].elevation;
                data->satellites[idx].azimuth = gsv->sats[i].azimuth;
                data->satellites[idx].snr = gsv->sats[i].snr;
            }
        }
    }
    
    data->last_update = time(NULL);
}

const char* gps_fix_quality_to_string(gps_fix_quality_t quality) {
    switch (quality) {
        case GPS_FIX_NONE: return "No Fix";
        case GPS_FIX_GPS: return "GPS";
        case GPS_FIX_DGPS: return "DGPS";
        case GPS_FIX_PPS: return "PPS";
        case GPS_FIX_RTK: return "RTK";
        case GPS_FIX_FLOAT_RTK: return "Float RTK";
        case GPS_FIX_ESTIMATED: return "Estimated";
        case GPS_FIX_MANUAL: return "Manual";
        case GPS_FIX_SIMULATION: return "Simulation";
        default: return "Unknown";
    }
}

const char* gps_status_to_string(gps_connection_status_t status) {
    switch (status) {
        case GPS_STATUS_DISCONNECTED: return "DISCONNECTED";
        case GPS_STATUS_CONNECTING: return "CONNECTING";
        case GPS_STATUS_CONNECTED: return "CONNECTED";
        case GPS_STATUS_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool gps_data_start_logging(gps_data_t* data, const char* filename) {
    if (!data || !filename) return false;
    
    if (data->log_file) {
        fclose(data->log_file);
    }
    
    data->log_file = fopen(filename, "a");
    if (data->log_file) {
        strcpy(data->log_filename, filename);
        data->logging_enabled = true;
        
        // Write header
        fprintf(data->log_file, "# GPS Log started at %ld\n", time(NULL));
        fflush(data->log_file);
        return true;
    }
    
    return false;
}

void gps_data_stop_logging(gps_data_t* data) {
    if (!data) return;
    
    if (data->log_file) {
        fprintf(data->log_file, "# GPS Log stopped at %ld\n", time(NULL));
        fclose(data->log_file);
        data->log_file = NULL;
    }
    
    data->logging_enabled = false;
    data->log_filename[0] = '\0';
}