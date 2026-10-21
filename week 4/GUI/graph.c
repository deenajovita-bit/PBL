
#include "deadlock.h"
#include <cairo.h>
#include <math.h>
#include <stdio.h>

/* Check whether a process is in the deadlocked list. */
static int is_deadlocked(int p, const int dead[], int dead_count)
{
    int i;

    for (i = 0; i < dead_count; ++i) {
        if (dead[i] == p)
            return 1;
    }

    return 0;
}

/* Draw an arrow between two points. */
static void draw_arrow(cairo_t *cr,
                       double x1, double y1,
                       double x2, double y2)
{
    double angle = atan2(y2 - y1, x2 - x1);
    double head = 9.0;

    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);

    cairo_move_to(cr, x2, y2);
    cairo_line_to(cr,
                  x2 - head * cos(angle - 0.45),
                  y2 - head * sin(angle - 0.45));
    cairo_line_to(cr,
                  x2 - head * cos(angle + 0.45),
                  y2 - head * sin(angle + 0.45));
    cairo_close_path(cr);
    cairo_fill(cr);
}

/* Draw the Resource Allocation Graph. */
void graph_draw(cairo_t *cr, int w, int h,
                const System *s,
                const int dead[], int dead_count)
{
    int i, j, k;
    double proc_x[MAX_P], res_x[MAX_R];
    double proc_y = 75.0;
    double res_y;
    double left = 50.0;
    double right = w > 100 ? (double)w - 50.0 : 50.0;
    double span = right - left;

    if (cr == NULL || s == NULL || s->n < 1 || s->m < 1)
        return;

    res_y = h > 150 ? (double)h - 115.0 : 150.0;

    if (span < 1.0)
        span = 1.0;

    cairo_save(cr);

    /* White background. */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    cairo_select_font_face(cr, "Sans",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);

    /* Calculate positions for processes and resources. */
    for (i = 0; i < s->n; ++i)
        proc_x[i] = left + span * (i + 1.0) / (s->n + 1.0);

    for (j = 0; j < s->m; ++j)
        res_x[j] = left + span * (j + 1.0) / (s->m + 1.0);

    /* Draw waiting requests as dashed red arrows. */
    for (i = 0; i < s->n; ++i) {
        if (!s->active[i])
            continue;

        for (j = 0; j < s->m; ++j) {
            if (s->request[i][j] > 0) {
                double x1 = proc_x[i];
                double y1 = proc_y + 24.0;
                double x2 = res_x[j];
                double y2 = res_y - 24.0;
                double dx = x2 - x1;
                double dy = y2 - y1;
                double length = hypot(dx, dy);
                char amount[24];
                const double dashes[] = {6.0, 4.0};

                if (length > 1.0) {
                    x1 += dx / length * 25.0;
                    y1 += dy / length * 25.0;
                    x2 -= dx / length * 29.0;
                    y2 -= dy / length * 29.0;
                }

                cairo_set_source_rgb(cr, 0.82, 0.12, 0.12);
                cairo_set_line_width(cr, 1.8);
                cairo_set_dash(cr, dashes, 2, 0.0);

                draw_arrow(cr, x1, y1, x2, y2);

                cairo_set_dash(cr, NULL, 0, 0.0);

                snprintf(amount, sizeof(amount), "%d",
                         s->request[i][j]);

                cairo_move_to(cr,
                              (x1 + x2) / 2.0 + 4,
                              (y1 + y2) / 2.0);
                cairo_show_text(cr, amount);
            }
        }
    }

    /* Draw allocations as solid arrows from resources to processes. */
    for (i = 0; i < s->n; ++i) {
        if (!s->active[i])
            continue;

        for (j = 0; j < s->m; ++j) {
            if (s->alloc[i][j] > 0) {
                double x1 = res_x[j];
                double y1 = res_y - 25.0;
                double x2 = proc_x[i];
                double y2 = proc_y + 25.0;
                double dx = x2 - x1;
                double dy = y2 - y1;
                double length = hypot(dx, dy);
                char amount[24];

                if (length > 1.0) {
                    x1 += dx / length * 28.0;
                    y1 += dy / length * 28.0;
                    x2 -= dx / length * 25.0;
                    y2 -= dy / length * 25.0;
                }

                cairo_set_source_rgb(cr, 0.12, 0.25, 0.55);
                cairo_set_line_width(cr, 1.8);
                cairo_set_dash(cr, NULL, 0, 0.0);

                draw_arrow(cr, x1, y1, x2, y2);

                snprintf(amount, sizeof(amount), "%d",
                         s->alloc[i][j]);

                cairo_move_to(cr,
                              (x1 + x2) / 2.0 - 12,
                              (y1 + y2) / 2.0);
                cairo_show_text(cr, amount);
            }
        }
    }

    /* Draw processes as circles. */
    for (i = 0; i < s->n; ++i) {
        int deadlocked = is_deadlocked(i, dead, dead_count);
        char label[24];

        if (!s->active[i]) {
            cairo_set_source_rgb(cr, 0.82, 0.82, 0.82);
        } else {
            cairo_set_source_rgb(cr, 0.88, 0.93, 1.0);
        }

        cairo_arc(cr, proc_x[i], proc_y, 23.0,
                  0, 2 * 3.141592653589793);
        cairo_fill_preserve(cr);

        if (!s->active[i]) {
            cairo_set_source_rgb(cr, 0.45, 0.45, 0.45);
            cairo_set_line_width(cr, 1.5);
        } else if (deadlocked) {
            cairo_set_source_rgb(cr, 0.85, 0.08, 0.08);
            cairo_set_line_width(cr, 3.0);
        } else {
            cairo_set_source_rgb(cr, 0.12, 0.25, 0.55);
            cairo_set_line_width(cr, 1.5);
        }

        cairo_stroke(cr);

        cairo_set_source_rgb(cr, 0.08, 0.08, 0.08);
        snprintf(label, sizeof(label), "P%d", i);
        cairo_move_to(cr, proc_x[i] - 9.0, proc_y + 4.0);
        cairo_show_text(cr, label);
    }

    /* Draw resource boxes and their instances. */
    for (j = 0; j < s->m; ++j) {
        double box_w = 58.0;
        double box_h = 62.0;
        double x = res_x[j] - box_w / 2.0;
        double y = res_y - box_h / 2.0;
        int allocated = 0;
        int dot_count = s->total[j];
        char label[24];

        for (i = 0; i < s->n; ++i)
            allocated += s->alloc[i][j];

        cairo_set_source_rgb(cr, 0.91, 0.96, 0.88);
        cairo_rectangle(cr, x, y, box_w, box_h);
        cairo_fill_preserve(cr);

        cairo_set_source_rgb(cr, 0.20, 0.45, 0.22);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);

        snprintf(label, sizeof(label), "R%d", j);
        cairo_set_source_rgb(cr, 0.08, 0.08, 0.08);
        cairo_move_to(cr, x + 20.0, y + 16.0);
        cairo_show_text(cr, label);

        /* Show up to 12 dots inside each resource box. */
        if (dot_count > 12)
            dot_count = 12;
        if (dot_count < 0)
            dot_count = 0;

        for (k = 0; k < dot_count; ++k) {
            cairo_arc(cr,
                      x + 9.0 + (k % 6) * 8.0,
                      y + 30.0 + (k / 6) * 13.0,
                      2.6, 0, 2 * 3.141592653589793);

            cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);

            if (k < allocated)
                cairo_fill(cr);
            else
                cairo_stroke(cr);
        }

        if (s->total[j] > 12) {
            char more[24];

            snprintf(more, sizeof(more), "+%d", s->total[j] - 12);
            cairo_move_to(cr, x + 8.0, y + box_h - 4.0);
            cairo_show_text(cr, more);
        }
    }

    /* Draw a small legend. */
    {
        double y = h > 35 ? (double)h - 18.0 : 18.0;
        const double dashes[] = {5.0, 3.0};

        cairo_set_source_rgb(cr, 0.12, 0.25, 0.55);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, 18, y - 4);
        cairo_line_to(cr, 48, y - 4);
        cairo_stroke(cr);

        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        cairo_move_to(cr, 53, y);
        cairo_show_text(cr, "Allocation");

        cairo_set_source_rgb(cr, 0.82, 0.12, 0.12);
        cairo_set_dash(cr, dashes, 2, 0.0);
        cairo_move_to(cr, 140, y - 4);
        cairo_line_to(cr, 170, y - 4);
        cairo_stroke(cr);
        cairo_set_dash(cr, NULL, 0, 0.0);

        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        cairo_move_to(cr, 175, y);
        cairo_show_text(cr, "Waiting request");

        cairo_set_source_rgb(cr, 0.85, 0.08, 0.08);
        cairo_arc(cr, 300, y - 4, 7,
                  0, 2 * 3.141592653589793);
        cairo_stroke(cr);

        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        cairo_move_to(cr, 312, y);
        cairo_show_text(cr, "Deadlocked process");
    }

    cairo_restore(cr);
}
