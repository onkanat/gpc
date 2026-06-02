//
// Serial port interface for GPS GUI application
//
#ifndef GPS_SERIAL_H
#define GPS_SERIAL_H

#include <stdbool.h>
#if defined(__has_include)
#if __has_include(<SDL.h>)
#include <SDL.h>
#elif __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#error "SDL2 headers not found. Please install SDL2 development packages."
#endif
#else
#include <SDL2/SDL.h>
#endif
#include "gps_data.h"

#define GPS_SERIAL_UPDATE_QUEUE_SIZE 128

typedef struct {
    bool valid;
    gps_data_t snapshot;
    char raw_line[256];
} gps_serial_update_t;

typedef struct {
    int fd;
    bool is_open;
    char buffer[4096];
    int buffer_pos;
    char line_buffer[256];
    int line_pos;
    gps_data_t worker_data;
    SDL_Thread* worker_thread;
    SDL_mutex* queue_mutex;
    SDL_atomic_t worker_stop;
    gps_serial_update_t update_queue[GPS_SERIAL_UPDATE_QUEUE_SIZE];
    int update_queue_head;
    int update_queue_tail;
    int update_queue_count;
    unsigned long dropped_updates;
} gps_serial_t;

// Function declarations
bool gps_serial_init(gps_serial_t* serial);
void gps_serial_cleanup(gps_serial_t* serial);
bool gps_serial_open(gps_serial_t* serial, const char* port, int baud_rate);
void gps_serial_close(gps_serial_t* serial);
bool gps_serial_is_open(const gps_serial_t* serial);
int gps_serial_read_data(gps_serial_t* serial, gps_data_t* gps_data);

// Read GPS data and optionally add raw NMEA to console
int gps_serial_read_data_with_console(gps_serial_t* serial, gps_data_t* gps_data, void* console_ptr);

bool gps_serial_list_ports(char ports[][256], int max_ports, int* count);

// Send command to GPS device
bool gps_serial_send_command(gps_serial_t* serial, const char* command);

#endif // GPS_SERIAL_H