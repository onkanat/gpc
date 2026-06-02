#ifndef GPS_IMPLOT_H
#define GPS_IMPLOT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPS_IMPLOT_FLAG_NO_LEGEND (1 << 1)
#define GPS_IMPLOT_AXIS_FLAG_AUTO_FIT (1 << 11)
#define GPS_IMPLOT_COND_ALWAYS 1

void gps_implot_create_context(void);
void gps_implot_destroy_context(void);
void gps_implot_set_next_axes_to_fit(void);
void gps_implot_set_next_axes_limits(double x_min, double x_max, double y_min, double y_max, int cond);
bool gps_implot_begin_plot(const char *title,
                           float width,
                           float height,
                           const char *x_label,
                           const char *y_label,
                           int flags,
                           int x_axis_flags,
                           int y_axis_flags);
void gps_implot_plot_line(const char *label, const double *xs, const double *ys, int count);
bool gps_implot_is_plot_hovered(void);
bool gps_implot_get_plot_mouse_pos(double *x, double *y);
void gps_implot_end_plot(void);

#ifdef __cplusplus
}
#endif

#endif // GPS_IMPLOT_H
