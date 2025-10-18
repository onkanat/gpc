#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "include/minmea.h"

#define DEFAULT_SERIAL_PORT "/dev/tty.usbmodem1201"

static bool baud_from_int(int baud, speed_t *out_speed)
{
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

static volatile sig_atomic_t g_stop = 0;
static int g_fd = -1;
static FILE *g_out = NULL;
static int g_timeout_sec = 0; // 0 = sınırsız
static bool g_verbose = false;

static void sig_stop(int sig) {
    (void)sig;
    g_stop = 1;
    if (g_verbose) fprintf(stderr, "[sig] stop requested (signal)\n");
}

static int open_serial_port(const char *port_name, speed_t speed) {
    if (g_verbose) fprintf(stderr, "[open] port=%s speed=%ld (termios)\n", port_name, (long)speed);
    int fd = open(port_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "Seri port açılırken hata: %s\n", strerror(errno));
        return -1;
    }
    // Başka süreçlerin aynı TTY'yi açmasını engelle
    if (ioctl(fd, TIOCEXCL) == -1) {
        fprintf(stderr, "TIOCEXCL yapılamadı: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Port özellikleri alınamadı: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    // Ham moda al ve hızları ayarla
    cfmakeraw(&tty);
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_cflag &= ~CRTSCTS;           // HW flow off
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // SW flow off
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // ek güvence
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VTIME] = 1; // 0.1s
    tty.c_cc[VMIN] = 0;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Port özellikleri ayarlanamadı: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    // Non-blocking teyit
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (g_verbose) fprintf(stderr, "[open] ok, fd=%d (nonblock)\n", fd);
    return fd;
}

static void handle_sentence(const char *sentence, FILE *out) {
    enum minmea_sentence_id id = minmea_sentence_id(sentence, false);
    switch (id) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, sentence)) {
                fprintf(out, "[RMC] valid=%s time=%02d:%02d:%02d date=%02d/%02d/%04d\n",
                    frame.valid ? "true" : "false",
                    frame.time.hours, frame.time.minutes, frame.time.seconds,
                    frame.date.day, frame.date.month, frame.date.year < 100 ? frame.date.year + 2000 : frame.date.year);
                if (frame.valid) {
                    fprintf(out, "      lat=%.6f lon=%.6f spd=%.2fkn cog=%.2f°\n",
                        minmea_tocoord(&frame.latitude),
                        minmea_tocoord(&frame.longitude),
                        minmea_tofloat(&frame.speed),
                        minmea_tofloat(&frame.course));
                }
            }
        } break;
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, sentence)) {
                fprintf(out, "[GGA] sats=%d fixq=%d hdop=%.2f alt=%.2f%c\n",
                    frame.satellites_tracked,
                    frame.fix_quality,
                    minmea_tofloat(&frame.hdop),
                    minmea_tofloat(&frame.altitude), frame.altitude_units);
            }
        } break;
        case MINMEA_SENTENCE_GSA: {
            struct minmea_sentence_gsa frame;
            if (minmea_parse_gsa(&frame, sentence)) {
                fprintf(out, "[GSA] mode=%c fix=%d pdop=%.2f hdop=%.2f vdop=%.2f\n",
                    frame.mode,
                    frame.fix_type,
                    minmea_tofloat(&frame.pdop),
                    minmea_tofloat(&frame.hdop),
                    minmea_tofloat(&frame.vdop));
            }
        } break;
        case MINMEA_SENTENCE_GSV: {
            struct minmea_sentence_gsv frame;
            if (minmea_parse_gsv(&frame, sentence)) {
                fprintf(out, "[GSV] msg %d/%d, total_sats=%d\n",
                    frame.msg_nr, frame.total_msgs, frame.total_sats);
            }
        } break;
        case MINMEA_SENTENCE_GLL: {
            struct minmea_sentence_gll frame;
            if (minmea_parse_gll(&frame, sentence)) {
                fprintf(out, "[GLL] status=%c lat=%.6f lon=%.6f\n",
                    frame.status,
                    minmea_tocoord(&frame.latitude),
                    minmea_tocoord(&frame.longitude));
            }
        } break;
        case MINMEA_SENTENCE_VTG: {
            struct minmea_sentence_vtg frame;
            if (minmea_parse_vtg(&frame, sentence)) {
                fprintf(out, "[VTG] true=%.2f° knots=%.2f kph=%.2f\n",
                    minmea_tofloat(&frame.true_track_degrees),
                    minmea_tofloat(&frame.speed_knots),
                    minmea_tofloat(&frame.speed_kph));
            }
        } break;
        case MINMEA_SENTENCE_ZDA: {
            struct minmea_sentence_zda frame;
            if (minmea_parse_zda(&frame, sentence)) {
                fprintf(out, "[ZDA] time=%02d:%02d:%02d date=%02d/%02d/%04d offset=%+03d:%02d\n",
                    frame.time.hours, frame.time.minutes, frame.time.seconds,
                    frame.date.day, frame.date.month, frame.date.year,
                    frame.hour_offset, frame.minute_offset);
            }
        } break;
        case MINMEA_INVALID:
            // Satır bozuk ya da checksum hatalı
            break;
        default:
            // Bilinmeyen cümle türü; sessiz geç
            break;
    }
    fflush(out);
}

static bool g_raw_dump = false;

static void read_from_fd(int fd, FILE *out) {
    char buf[256];
    char line[MINMEA_MAX_SENTENCE_LENGTH + 1];
    int idx = 0;
    int elapsed = 0;
    while (!g_stop) {
        fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int maxfd = fd > STDIN_FILENO ? fd : STDIN_FILENO;
        int sel = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (sel < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "select() hatası: %s\n", strerror(errno));
            break;
        }
        if (sel == 0) {
            // timeout - döngüye devam
            elapsed += 1;
            if (g_verbose) fprintf(stderr, "[wait] no data (%ds)\n", elapsed);
            if (g_timeout_sec > 0 && elapsed >= g_timeout_sec) {
                fprintf(stderr, "Zaman aşımı: %d sn\n", g_timeout_sec);
                break;
            }
            continue;
        }
        // Kullanıcıdan 'q' gelirse çık
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            char ch;
            if (read(STDIN_FILENO, &ch, 1) > 0) {
                if (ch == 'q' || ch == 'Q') {
                    fprintf(stderr, "Kullanıcı isteği: çıkış (q)\n");
                    break;
                }
            }
        }
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            elapsed = 0; // veri geldi, sayaç sıfırla
            for (ssize_t i = 0; i < n; i++) {
                char c = buf[i];
                if (c == '\r' || c == '\n') {
                    if (idx > 0) {
                        line[idx] = '\0';
                        if (g_raw_dump) {
                            fprintf(out, "%s\n", line);
                            fflush(out);
                        }
                        handle_sentence(line, out);
                        idx = 0;
                    }
                } else {
                    if (idx < MINMEA_MAX_SENTENCE_LENGTH) line[idx++] = c;
                    else idx = 0; // çok uzun satır - sıfırla
                }
            }
        } else if (n == 0) {
            // EOF
            break;
        } else {
            if (errno == EINTR) {
                if (g_stop) break; else continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // non-blocking modda veri yok; kısa uyku ile bekle
                usleep(10000);
                continue;
            }
            fprintf(stderr, "Okuma hatası: %s\n", strerror(errno));
            break;
        }
    }
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Kullanım: %s [--stdin] [--port DEV] [--baud N] [--out FILE]\n", prog);
    fprintf(stderr, "  --stdin        : NMEA verisini stdin'den oku\n");
    fprintf(stderr, "  --port DEV     : Seri port cihaz yolu (varsayılan: %s)\n", DEFAULT_SERIAL_PORT);
    fprintf(stderr, "  --baud N       : Baud hızı (4800,9600,19200,38400,57600,115200,...)\n");
    fprintf(stderr, "  --out FILE     : Çıktıyı verilen dosyaya yaz (varsayılan stdout)\n");
    fprintf(stderr, "  --raw          : Ayrıştırmanın yanı sıra ham NMEA satırlarını da yaz\n");
    fprintf(stderr, "  --timeout N    : N saniye sonra otomatik çık (0=bekle)\n");
    fprintf(stderr, "  --verbose      : Teşhis loglarını etkinleştir\n");
}

int main(int argc, char **argv) {
    const char *port = DEFAULT_SERIAL_PORT;
    int baud = 9600;
    const char *out_path = NULL;
    bool use_stdin = false;
    g_raw_dump = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--stdin") == 0) {
            use_stdin = true;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = argv[++i];
        } else if (strcmp(argv[i], "--baud") == 0 && i + 1 < argc) {
            baud = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (strcmp(argv[i], "--raw") == 0) {
            g_raw_dump = true;
        } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            g_timeout_sec = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--verbose") == 0) {
            g_verbose = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Bilinmeyen argüman: %s\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        }
    }

    FILE *out = stdout;
    bool out_is_file = false;
    if (out_path) {
        out = fopen(out_path, "a");
        if (!out) {
            fprintf(stderr, "Uyarı: Çıkış dosyası açılamadı: %s (%s). Konsola yazılacak.\n", out_path, strerror(errno));
            out = stdout;
            out_is_file = false;
        } else {
            out_is_file = true;
        }
    }
    // Dosya/terminal fark etmeksizin satır bazlı bufferlama açalım.
    setvbuf(out, NULL, _IOLBF, 0);

    if (use_stdin) {
        g_out = out;
        signal(SIGINT, sig_stop);
        signal(SIGTERM, sig_stop);
        read_from_fd(STDIN_FILENO, out);
        if (out_is_file) fclose(out);
        return 0;
    }

    speed_t speed;
    if (!baud_from_int(baud, &speed)) {
        fprintf(stderr, "Desteklenmeyen baud: %d\n", baud);
        if (out_is_file) fclose(out);
        return 2;
    }

    int fd = open_serial_port(port, speed);
    if (fd < 0) {
        fprintf(stderr, "Seri porta bağlanılamadı: %s\n", port);
        if (out_is_file) fclose(out);
        return 1;
    }
    fprintf(stderr, "%s portuna %d baud ile bağlandı. Çıkmak için Ctrl+C.\n", port, baud);
    g_fd = fd; g_out = out;
    signal(SIGINT, sig_stop);
    signal(SIGTERM, sig_stop);
    read_from_fd(fd, out);
    close(fd);
    if (out_is_file) fclose(out);
    return 0;
}
