//
// GPS Raw Data Console module implementation
//
#include "gps_console.h"
#include <string.h>
#include <stdio.h>

bool console_init(console_t* console) {
    if (!console) return false;
    
    // Initialize buffer
    for (int i = 0; i < CONSOLE_MAX_LINES; i++) {
        console->buffer[i][0] = '\0';
    }
    
    console->current_index = 0;
    console->command_input[0] = '\0';
    console->auto_scroll = true;
    console->total_lines = 0;
    
    return true;
}

void console_cleanup(console_t* console) {
    if (!console) return;
    // Currently no cleanup needed
}

void console_add_line(console_t* console, const char* line) {
    if (!console || !line) return;
    
    // Copy line to current position in circular buffer
    strncpy(console->buffer[console->current_index], line, CONSOLE_LINE_LENGTH - 1);
    console->buffer[console->current_index][CONSOLE_LINE_LENGTH - 1] = '\0';
    
    // Move to next position
    console->current_index = (console->current_index + 1) % CONSOLE_MAX_LINES;
    console->total_lines++;
}

void console_clear(console_t* console) {
    if (!console) return;
    
    for (int i = 0; i < CONSOLE_MAX_LINES; i++) {
        console->buffer[i][0] = '\0';
    }
    
    console->current_index = 0;
    console->total_lines = 0;
}

const char* console_get_line(const console_t* console, int line_index) {
    if (!console || line_index < 0 || line_index >= CONSOLE_MAX_LINES) {
        return NULL;
    }
    
    // Calculate actual buffer index for this line position
    // line_index 0 = oldest, 4 = newest
    int buffer_index = (console->current_index + line_index) % CONSOLE_MAX_LINES;
    
    // Return empty string if line is empty
    if (console->buffer[buffer_index][0] == '\0') {
        return NULL;
    }
    
    return console->buffer[buffer_index];
}

void console_set_command(console_t* console, const char* command) {
    if (!console || !command) return;
    
    strncpy(console->command_input, command, CONSOLE_LINE_LENGTH - 1);
    console->command_input[CONSOLE_LINE_LENGTH - 1] = '\0';
}

const char* console_get_command(const console_t* console) {
    if (!console) return NULL;
    return console->command_input;
}

void console_clear_command(console_t* console) {
    if (!console) return;
    console->command_input[0] = '\0';
}