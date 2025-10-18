//
// Serial port implementation for GPS GUI
//
#include "gps_serial.h"
#include "minmea.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <dirent.h>

static bool baud_from_int(int baud, speed_t *out_speed) {
    switch (baud) {
        case 4800:   *out_speed = B4800;   return true;
        case 9600:   *out_speed = B9600;   return true;
        case 19200:  *out_speed = B19200;  return true;
        case 38400:  *out_speed = B38400;  return true;
        case 57600:  *out_speed = B57600;  return true;
        case 115200: *out_speed = B115200; return true;
#ifdef B230400
        case 230400: *out_speed = B230400; return true;
#endif
        default: return false;
    }
}

bool gps_serial_init(gps_serial_t* serial) {
    if (!serial) return false;
    
    serial->fd = -1;
    serial->is_open = false;
    serial->buffer_pos = 0;
    serial->line_pos = 0;
    memset(serial->buffer, 0, sizeof(serial->buffer));
    memset(serial->line_buffer, 0, sizeof(serial->line_buffer));
    
    return true;
}

void gps_serial_cleanup(gps_serial_t* serial) {
    if (serial && serial->is_open) {
        gps_serial_close(serial);
    }
}

bool gps_serial_open(gps_serial_t* serial, const char* port, int baud_rate) {
    if (!serial || !port) return false;
    
    if (serial->is_open) {
        gps_serial_close(serial);
    }
    
    speed_t speed;
    if (!baud_from_int(baud_rate, &speed)) {
        fprintf(stderr, "Unsupported baud rate: %d\n", baud_rate);
        return false;
    }
    
    serial->fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial->fd < 0) {
        fprintf(stderr, "Failed to open port %s: %s\n", port, strerror(errno));
        return false;
    }
    
    // Set exclusive access
    if (ioctl(serial->fd, TIOCEXCL) == -1) {
        fprintf(stderr, "Failed to set exclusive access: %s\n", strerror(errno));
        close(serial->fd);
        serial->fd = -1;
        return false;
    }
    
    struct termios tty;
    if (tcgetattr(serial->fd, &tty) != 0) {
        fprintf(stderr, "Failed to get port attributes: %s\n", strerror(errno));
        close(serial->fd);
        serial->fd = -1;
        return false;
    }
    
    // Configure port for raw mode
    cfmakeraw(&tty);
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_cflag &= ~CRTSCTS;           // No hardware flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    
    tty.c_cc[VTIME] = 1;  // 0.1 second timeout
    tty.c_cc[VMIN] = 0;   // Non-blocking read
    
    if (tcsetattr(serial->fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Failed to set port attributes: %s\n", strerror(errno));
        close(serial->fd);
        serial->fd = -1;
        return false;
    }
    
    // Ensure non-blocking mode
    int flags = fcntl(serial->fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(serial->fd, F_SETFL, flags | O_NONBLOCK);
    }
    
    serial->is_open = true;
    serial->buffer_pos = 0;
    serial->line_pos = 0;
    
    printf("GPS serial port opened: %s at %d baud\n", port, baud_rate);
    return true;
}

void gps_serial_close(gps_serial_t* serial) {
    if (!serial) return;
    
    if (serial->fd >= 0) {
        close(serial->fd);
        serial->fd = -1;
    }
    
    serial->is_open = false;
    serial->buffer_pos = 0;
    serial->line_pos = 0;
}

bool gps_serial_is_open(const gps_serial_t* serial) {
    return serial && serial->is_open;
}

static void process_nmea_sentence(const char* sentence, gps_data_t* gps_data) {
    if (!sentence || !gps_data) return;
    
    // Log raw sentence if logging is enabled
    if (gps_data->logging_enabled && gps_data->log_file) {
        fprintf(gps_data->log_file, "%s\n", sentence);
        fflush(gps_data->log_file);
    }
    
    enum minmea_sentence_id id = minmea_sentence_id(sentence, false);
    
    switch (id) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, sentence)) {
                gps_data_update_from_rmc(gps_data, &frame);
            }
            break;
        }
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, sentence)) {
                gps_data_update_from_gga(gps_data, &frame);
            }
            break;
        }
        case MINMEA_SENTENCE_GSA: {
            struct minmea_sentence_gsa frame;
            if (minmea_parse_gsa(&frame, sentence)) {
                gps_data_update_from_gsa(gps_data, &frame);
            }
            break;
        }
        case MINMEA_SENTENCE_GSV: {
            struct minmea_sentence_gsv frame;
            if (minmea_parse_gsv(&frame, sentence)) {
                gps_data_update_from_gsv(gps_data, &frame);
            }
            break;
        }
        default:
            // Ignore unknown sentence types
            break;
    }
}

int gps_serial_read_data(gps_serial_t* serial, gps_data_t* gps_data) {
    if (!serial || !gps_data || !serial->is_open) return -1;
    
    ssize_t bytes_read = read(serial->fd, serial->buffer + serial->buffer_pos, 
                             sizeof(serial->buffer) - serial->buffer_pos - 1);
    
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No data available
        }
        return -1; // Error
    }
    
    if (bytes_read == 0) return 0; // No data
    
    serial->buffer_pos += bytes_read;
    serial->buffer[serial->buffer_pos] = '\0';
    
    // Process complete lines
    char* line_start = serial->buffer;
    char* line_end;
    int processed_lines = 0;
    
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        *line_end = '\0';
        
        // Remove carriage return if present
        if (line_end > line_start && *(line_end - 1) == '\r') {
            *(line_end - 1) = '\0';
        }
        
        // Process the line if it looks like NMEA
        if (strlen(line_start) > 0 && line_start[0] == '$') {
            process_nmea_sentence(line_start, gps_data);
            processed_lines++;
        }
        
        line_start = line_end + 1;
    }
    
    // Move remaining data to beginning of buffer
    int remaining = serial->buffer + serial->buffer_pos - line_start;
    if (remaining > 0) {
        memmove(serial->buffer, line_start, remaining);
    }
    serial->buffer_pos = remaining;
    
    return processed_lines;
}

bool gps_serial_list_ports(char ports[][256], int max_ports, int* count) {
    if (!ports || !count) return false;
    
    *count = 0;
    
#ifdef __APPLE__
    // macOS: Look for USB modem devices
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL && *count < max_ports) {
            if (strncmp(entry->d_name, "tty.usbmodem", 12) == 0 ||
                strncmp(entry->d_name, "cu.usbmodem", 11) == 0 ||
                strncmp(entry->d_name, "tty.usbserial", 13) == 0 ||
                strncmp(entry->d_name, "cu.usbserial", 12) == 0) {
                snprintf(ports[*count], 256, "/dev/%s", entry->d_name);
                (*count)++;
            }
        }
        closedir(dir);
    }
#else
    // Linux: Look for USB serial devices
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL && *count < max_ports) {
            if (strncmp(entry->d_name, "ttyUSB", 6) == 0 ||
                strncmp(entry->d_name, "ttyACM", 6) == 0) {
                snprintf(ports[*count], 256, "/dev/%s", entry->d_name);
                (*count)++;
            }
        }
        closedir(dir);
    }
#endif
    
    return *count > 0;
}