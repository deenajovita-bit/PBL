/*
 * gantt.c  -  Gantt chart display for the Process Scheduling Simulator
 *
 * gantt_display() only READS the chart and prints it.
 * It never changes the chart.
 *
 * Example of what it prints:
 *
 *   | P1  | P2  | IDLE | P1  |
 *   0     3     6      8     9
 */

#include <stdio.h>
#include <string.h>
#include "scheduler.h"

#define SEGMENTS_PER_ROW 8   /* wrap the chart so it fits on the screen */

/* Count how many characters a non-negative number needs when printed. */
static int digits_in(int x)
{
    int d = 1;
    if (x < 0) {          /* should not happen, but stay safe */
        x = -x;
        d++;
    }
    while (x >= 10) {
        x /= 10;
        d++;
    }
    return d;
}

/* Write the name of one box into 'label': "P3" or "IDLE". */
static void make_label(const GanttSegment *s, char *label, size_t size)
{
    if (s->pid == -1) {
        snprintf(label, size, "IDLE");
    } else {
        snprintf(label, size, "P%d", s->pid);
    }
}

/* Width of one box = wide enough for the label AND the time numbers. */
static int box_width(const GanttSegment *s)
{
    char label[16];
    int w, need;

    make_label(s, label, sizeof(label));
    w = (int)strlen(label) + 2;          /* one space on each side */
    need = digits_in(s->end) + 1;        /* room for the end time  */
    if (digits_in(s->start) + 1 > need) {
        need = digits_in(s->start) + 1;
    }
    return (w > need) ? w : need;
}

void gantt_display(const GanttChart *g)
{
    int row_start, row_end, i, j;

    /* Safety: no chart, empty chart, or a broken count. */
    if (g == NULL || g->count <= 0) {
        printf("Gantt chart is empty.\n");
        return;
    }

    int total = g->count;
    if (total > MAX_SEGMENTS) {
        total = MAX_SEGMENTS;
    }

    printf("\nGantt Chart:\n");

    /* Print the chart in rows of SEGMENTS_PER_ROW boxes. */
    for (row_start = 0; row_start < total; row_start += SEGMENTS_PER_ROW) {
        row_end = row_start + SEGMENTS_PER_ROW;
        if (row_end > total) {
            row_end = total;
        }

        /* Line 1: the boxes  | P1 | P2 | IDLE | */
        for (i = row_start; i < row_end; i++) {
            char label[16];
            int w = box_width(&g->segments[i]);
            int len, left, right;

            make_label(&g->segments[i], label, sizeof(label));
            len   = (int)strlen(label);
            left  = (w - len) / 2;       /* centre the label */
            right = w - len - left;

            printf("|");
            for (j = 0; j < left; j++)  printf(" ");
            printf("%s", label);
            for (j = 0; j < right; j++) printf(" ");
        }
        printf("|\n");

        /* Line 2: the times, lined up under each "|" */
        for (i = row_start; i < row_end; i++) {
            int w = box_width(&g->segments[i]);
            printf("%-*d", w + 1, g->segments[i].start);
        }
        printf("%d\n", g->segments[row_end - 1].end);
    }

    printf("\n");
}
