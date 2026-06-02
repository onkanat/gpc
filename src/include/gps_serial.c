//
// Serial port implementation for GPS GUI
//
#include "gps_serial.h"
#include "gps_console.h"
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

static void process_nmea_sentence(const char* sentence, gps_data_t* gps_data, void* console_ptr);

static void gps_serial_queue_lock(gps_serial_t* serial) {
    if (serial && serial->queue_mutex) {
        SDL_LockMutex(serial->queue_mutex);
    }
}

static void gps_serial_queue_unlock(gps_serial_t* serial) {
    if (serial && serial->queue_mutex) {
        SDL_UnlockMutex(serial->queue_mutex);
    }
}

static bool gps_serial_enqueue_update(gps_serial_t* serial, const gps_data_t* snapshot, const char* raw_line) {
    if (!serial || !snapshot || !raw_line) return false;

    gps_serial_queue_lock(serial);
    if (serial->update_queue_count >= GPS_SERIAL_UPDATE_QUEUE_SIZE) {
        serial->dropped_updates++;
        gps_serial_queue_unlock(serial);
        return false;
    }

    gps_serial_update_t* slot = &serial->update_queue[serial->update_queue_tail];
    memset(slot, 0, sizeof(*slot));
    slot->valid = true;
    slot->snapshot = *snapshot;
    slot->snapshot.log_file = NULL;
    strncpy(slot->raw_line, raw_line, sizeof(slot->raw_line) - 1);
    slot->raw_line[sizeof(slot->raw_line) - 1] = '\0';

    serial->update_queue_tail = (serial->update_queue_tail + 1) % GPS_SERIAL_UPDATE_QUEUE_SIZE;
    serial->update_queue_count++;
    gps_serial_queue_unlock(serial);
    return true;
}

static bool gps_serial_dequeue_update(gps_serial_t* serial, gps_serial_update_t* out_update) {
    if (!serial || !out_update) return false;

    gps_serial_queue_lock(serial);
    if (serial->update_queue_count <= 0) {
        gps_serial_queue_unlock(serial);
        return false;
    }

    *out_update = serial->update_queue[serial->update_queue_head];
    serial->update_queue[serial->update_queue_head].valid = false;
    serial->update_queue_head = (serial->update_queue_head + 1) % GPS_SERIAL_UPDATE_QUEUE_SIZE;
    serial->update_queue_count--;
    gps_serial_queue_unlock(serial);
    return true;
}

static void gps_serial_apply_snapshot(gps_data_t* target, const gps_serial_update_t* update) {
    if (!target || !update) return;

    bool logging_enabled = target->logging_enabled;
    FILE* log_file = target->log_file;
    char log_filename[256];
    gps_connection_status_t status = target->status;
    char port_name[256];
    int baud_rate = target->baud_rate;

    strncpy(log_filename, target->log_filename, sizeof(log_filename) - 1);
    log_filename[sizeof(log_filename) - 1] = '\0';
    strncpy(port_name, target->port_name, sizeof(port_name) - 1);
    port_name[sizeof(port_name) - 1] = '\0';

    *target = update->snapshot;
    target->logging_enabled = logging_enabled;
    target->log_file = log_file;
    strncpy(target->log_filename, log_filename, sizeof(target->log_filename) - 1);
    target->log_filename[sizeof(target->log_filename) - 1] = '\0';
    target->status = status;
    target->baud_rate = baud_rate;
    strncpy(target->port_name, port_name, sizeof(target->port_name) - 1);
    target->port_name[sizeof(target->port_name) - 1] = '\0';
}

static int gps_serial_worker_thread_fn(void* userdata) {
    gps_serial_t* serial = (gps_serial_t*)userdata;
    if (!serial) return 0;

    while (SDL_AtomicGet(&serial->worker_stop) == 0) {
        ssize_t bytes_read = read(serial->fd,
                                  serial->buffer + serial->buffer_pos,
                                  sizeof(serial->buffer) - serial->buffer_pos - 1);

        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                SDL_Delay(10);
                continue;
            }
            SDL_Delay(20);
            continue;
        }

        if (bytes_read == 0) {
            SDL_Delay(10);
            continue;
        }

        serial->buffer_pos += (int)bytes_read;
        serial->buffer[serial->buffer_pos] = '\0';

        char* line_start = serial->buffer;
        char* line_end = NULL;
        while ((line_end = strchr(line_start, '\n')) != NULL) {
            *line_end = '\0';
            if (line_end > line_start && *(line_end - 1) == '\r') {
                *(line_end - 1) = '\0';
            }

            if (strlen(line_start) > 0 && line_start[0] == '$') {
                process_nmea_sentence(line_start, &serial->worker_data, NULL);
                (void)gps_serial_enqueue_update(serial, &serial->worker_data, line_start);
            }

            line_start = line_end + 1;
        }

        int remaining = (int)(serial->buffer + serial->buffer_pos - line_start);
        if (remaining > 0) {
            memmove(serial->buffer, line_start, (size_t)remaining);
        }
        serial->buffer_pos = remaining;
    }

    return 0;
}

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
    gps_data_init(&serial->worker_data);
    serial->worker_thread = NULL;
    serial->queue_mutex = SDL_CreateMutex();
    SDL_AtomicSet(&serial->worker_stop, 0);
    serial->update_queue_head = 0;
    serial->update_queue_tail = 0;
    serial->update_queue_count = 0;
    serial->dropped_updates = 0;
    memset(serial->buffer, 0, sizeof(serial->buffer));
    memset(serial->line_buffer, 0, sizeof(serial->line_buffer));
    
    return true;
}

void gps_serial_cleanup(gps_serial_t* serial) {
    if (serial && serial->is_open) {
        gps_serial_close(serial);
    }
    if (serial && serial->queue_mutex) {
        SDL_DestroyMutex(serial->queue_mutex);
        serial->queue_mutex = NULL;
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
    gps_data_init(&serial->worker_data);
    serial->worker_data.status = GPS_STATUS_CONNECTED;
    serial->worker_data.baud_rate = baud_rate;
    strncpy(serial->worker_data.port_name, port, sizeof(serial->worker_data.port_name) - 1);
    serial->worker_data.port_name[sizeof(serial->worker_data.port_name) - 1] = '\0';
    serial->update_queue_head = 0;
    serial->update_queue_tail = 0;
    serial->update_queue_count = 0;
    serial->dropped_updates = 0;
    SDL_AtomicSet(&serial->worker_stop, 0);

    if (serial->queue_mutex) {
        serial->worker_thread = SDL_CreateThread(gps_serial_worker_thread_fn, "gpc-gps-serial-worker", serial);
    }
    
    printf("GPS serial port opened: %s at %d baud\n", port, baud_rate);
    return true;
}

void gps_serial_close(gps_serial_t* serial) {
    if (!serial) return;

    SDL_AtomicSet(&serial->worker_stop, 1);
    if (serial->worker_thread) {
        SDL_WaitThread(serial->worker_thread, NULL);
        serial->worker_thread = NULL;
    }
    
    if (serial->fd >= 0) {
        close(serial->fd);
        serial->fd = -1;
    }
    
    serial->is_open = false;
    serial->buffer_pos = 0;
    serial->line_pos = 0;
    serial->update_queue_head = 0;
    serial->update_queue_tail = 0;
    serial->update_queue_count = 0;
}

bool gps_serial_is_open(const gps_serial_t* serial) {
    return serial && serial->is_open;
}

static void process_nmea_sentence(const char* sentence, gps_data_t* gps_data, void* console_ptr) {
    if (!sentence || !gps_data) return;
    
    // Add raw NMEA sentence to console if console pointer is provided
    if (console_ptr) {
        console_add_line((console_t*)console_ptr, sentence);
    }
    
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
    return gps_serial_read_data_with_console(serial, gps_data, NULL);
}

int gps_serial_read_data_with_console(gps_serial_t* serial, gps_data_t* gps_data, void* console_ptr) {
    if (!serial || !gps_data || !serial->is_open) return -1;
    int processed_lines = 0;

    gps_serial_update_t update = {};
    while (gps_serial_dequeue_update(serial, &update)) {
        gps_serial_apply_snapshot(gps_data, &update);

        if (console_ptr && update.raw_line[0] != '\0') {
            console_add_line((console_t*)console_ptr, update.raw_line);
        }

        if (gps_data->logging_enabled && gps_data->log_file && update.raw_line[0] != '\0') {
            fprintf(gps_data->log_file, "%s\n", update.raw_line);
            fflush(gps_data->log_file);
        }

        processed_lines++;
    }

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

bool gps_serial_send_command(gps_serial_t* serial, const char* command) {
    if (!serial || !command || !serial->is_open) {
        return false;
    }
    
    size_t command_len = strlen(command);
    if (command_len == 0) {
        return false;
    }
    
    // Send command to GPS device
    ssize_t bytes_written = write(serial->fd, command, command_len);
    if (bytes_written < 0) {
        fprintf(stderr, "Failed to send GPS command: %s\n", strerror(errno));
        return false;
    }
    
    // If command doesn't end with \r\n, add it
    if (command_len < 2 || 
        command[command_len-2] != '\r' || 
        command[command_len-1] != '\n') {
        bytes_written = write(serial->fd, "\r\n", 2);
        if (bytes_written < 0) {
            fprintf(stderr, "Failed to send command terminator: %s\n", strerror(errno));
            return false;
        }
    }
    
    // Flush the output
    if (tcdrain(serial->fd) != 0) {
        fprintf(stderr, "Failed to flush GPS command: %s\n", strerror(errno));
        return false;
    }
    
    printf("GPS command sent: %s\n", command);
    return true;
}