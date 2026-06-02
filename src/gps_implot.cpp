#include "include/gps_implot.h"

#include "third_party/implot/implot.h"

void gps_implot_create_context(void)
{
    ImPlot::CreateContext();
}

void gps_implot_destroy_context(void)
{
    ImPlot::DestroyContext();
}

void gps_implot_set_next_axes_to_fit(void)
{
    ImPlot::SetNextAxesToFit();
}

void gps_implot_set_next_axes_limits(double x_min, double x_max, double y_min, double y_max, int cond)
{
    ImPlot::SetNextAxesLimits(x_min, x_max, y_min, y_max, cond);
}

bool gps_implot_begin_plot(const char *title,
                           float width,
                           float height,
                           const char *x_label,
                           const char *y_label,
                           int flags,
                           int x_axis_flags,
                           int y_axis_flags)
{
    if (!ImPlot::BeginPlot(title, ImVec2(width, height), flags))
    {
        return false;
    }

    ImPlot::SetupAxes(x_label, y_label, x_axis_flags, y_axis_flags);
    return true;
}

void gps_implot_plot_line(const char *label, const double *xs, const double *ys, int count)
{
    if (!label || !xs || !ys || count <= 0)
    {
        return;
    }

    ImPlot::PlotLine(label, xs, ys, count);
}

bool gps_implot_is_plot_hovered(void)
{
    return ImPlot::IsPlotHovered();
}

bool gps_implot_get_plot_mouse_pos(double *x, double *y)
{
    if (!x || !y)
    {
        return false;
    }

    ImPlotPoint p = ImPlot::GetPlotMousePos();
    *x = p.x;
    *y = p.y;
    return true;
}

void gps_implot_end_plot(void)
{
    ImPlot::EndPlot();
}
