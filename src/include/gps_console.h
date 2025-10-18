//
// GPS Raw Data Console module - NMEA monitoring and command interface
//
#ifndef GPS_CONSOLE_H
#define GPS_CONSOLE_H

#include "gps_data.h"
#include "gps_serial.h"

#define CONSOLE_MAX_LINES 5
#define CONSOLE_LINE_LENGTH 256

typedef struct {
    char buffer[CONSOLE_MAX_LINES][CONSOLE_LINE_LENGTH];  // Circular buffer for NMEA lines
    int current_index;                                     // Current write position
    char command_input[CONSOLE_LINE_LENGTH];               // Command input buffer
    bool auto_scroll;                                      // Auto-scroll to newest data
    int total_lines;                                       // Total lines received (for stats)
} console_t;

// Initialize console system
bool console_init(console_t* console);

// Cleanup console system
void console_cleanup(console_t* console);

// Add a new NMEA line to the console buffer
void console_add_line(console_t* console, const char* line);

// Clear all console buffer
void console_clear(console_t* console);

// Get a specific line from the buffer (0 = oldest, 4 = newest)
const char* console_get_line(const console_t* console, int line_index);

// Set command input text
void console_set_command(console_t* console, const char* command);

// Get current command input text
const char* console_get_command(const console_t* console);

// Clear command input
void console_clear_command(console_t* console);

#endif // GPS_CONSOLE_H