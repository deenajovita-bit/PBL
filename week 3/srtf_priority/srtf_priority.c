
#include "scheduler.h"
#include <stddef.h>
/* ---------------------------------------------------------
   Helper 1: Reset the Gantt chart and completion times.
   --------------------------------------------------------- */
static int prepare_run(Process p[], int n, GanttChart *g)
{
    int i;

    if (g == NULL)
        return 0;

    g->count = 0;

    if (n < 1 || n > MAX_PROCESSES || p == NULL)
        return 0;

    /* Mark completion times as not completed yet. */
    for (i = 0; i < n; i++)
        p[i].completion_time = -1;

    /* Invalid input should not cause an infinite loop. */
    for (i = 0; i < n; i++) {
        if (p[i].arrival_time < 0 || p[i].burst_time <= 0)
            return 0;
    }

    return 1;
}

/* ---------------------------------------------------------
   Helper 2: Add a Gantt segment.
   Adjacent segments with the same PID are merged.
   PID -1 represents an idle CPU.
   Returns 1 on success and 0 if the chart is full.
   --------------------------------------------------------- */
static int add_segment(GanttChart *g, int pid, int start, int end)
{
    GanttSegment *last;

    if (g == NULL || end <= start)
        return 0;

    /* Merge continuous execution by the same process. */
    if (g->count > 0) {
        last = &g->segments[g->count - 1];

        if (last->pid == pid && last->end == start) {
            last->end = end;
            return 1;
        }
    }

    /* Never write beyond the supplied chart capacity. */
    if (g->count >= MAX_SEGMENTS)
        return 0;

    g->segments[g->count].pid = pid;
    g->segments[g->count].start = start;
    g->segments[g->count].end = end;
    g->count++;

    return 1;
}

/* ---------------------------------------------------------
   Helper 3: Tie-breaking for SRTF.
   Shorter remaining time wins.
   If equal, earlier arrival wins, then lower PID.
   --------------------------------------------------------- */
static int srtf_better(const Process p[], int candidate,
                       int current, const int remaining[])
{
    if (current == -1)
        return 1;

    if (remaining[candidate] != remaining[current])
        return remaining[candidate] < remaining[current];

    if (p[candidate].arrival_time != p[current].arrival_time)
        return p[candidate].arrival_time < p[current].arrival_time;

    return p[candidate].pid < p[current].pid;
}

/* ---------------------------------------------------------
   Helper 4: Tie-breaking for Priority Scheduling.
   Smaller priority number means higher priority.
   If equal, earlier arrival wins, then lower PID.
   --------------------------------------------------------- */
static int priority_better(const Process p[], int candidate,
                           int current)
{
    if (current == -1)
        return 1;

    if (p[candidate].priority != p[current].priority)
        return p[candidate].priority < p[current].priority;

    if (p[candidate].arrival_time != p[current].arrival_time)
        return p[candidate].arrival_time < p[current].arrival_time;

    return p[candidate].pid < p[current].pid;
}

/* =========================================================
   1. SRTF: Shortest Remaining Time First
   Preemptive version of Shortest Job First.
   ========================================================= */
void srtf(Process p[], int n, GanttChart *g)
{
    int remaining[MAX_PROCESSES];
    int completed = 0;
    int time = 0;
    int i;

    if (!prepare_run(p, n, g))
        return;

    for (i = 0; i < n; i++)
        remaining[i] = p[i].burst_time;

    while (completed < n) {
        int selected = -1;

        /* Find the best process that has already arrived. */
        for (i = 0; i < n; i++) {
            if (remaining[i] > 0 &&
                p[i].arrival_time <= time &&
                srtf_better(p, i, selected, remaining)) {
                selected = i;
            }
        }

        /* No process is ready: record CPU idle time. */
        if (selected == -1) {
            if (!add_segment(g, -1, time, time + 1))
                return;

            time++;
            continue;
        }

        /* Execute for one time unit so new arrivals can preempt. */
        if (!add_segment(g, p[selected].pid, time, time + 1))
            return;

        remaining[selected]--;
        time++;

        if (remaining[selected] == 0) {
            p[selected].completion_time = time;
            completed++;
        }
    }
}

/* =========================================================
   2. Non-preemptive Priority Scheduling
   Once selected, a process runs until it finishes.
   ========================================================= */
void priority_np(Process p[], int n, GanttChart *g)
{
    int completed_flag[MAX_PROCESSES] = {0};
    int completed = 0;
    int time = 0;
    int i;

    if (!prepare_run(p, n, g))
        return;

    while (completed < n) {
        int selected = -1;

        /* Select the highest-priority arrived process. */
        for (i = 0; i < n; i++) {
            if (!completed_flag[i] &&
                p[i].arrival_time <= time &&
                priority_better(p, i, selected)) {
                selected = i;
            }
        }

        /* If nothing has arrived, advance through idle time. */
        if (selected == -1) {
            if (!add_segment(g, -1, time, time + 1))
                return;

            time++;
            continue;
        }

        /* Run the selected process to completion. */
        if (!add_segment(g, p[selected].pid,
                         time, time + p[selected].burst_time)) {
            return;
        }

        time += p[selected].burst_time;
        p[selected].completion_time = time;
        completed_flag[selected] = 1;
        completed++;
    }
}

/* =========================================================
   3. Preemptive Priority Scheduling
   A newly selected higher-priority process can preempt.
   ========================================================= */
void priority_p(Process p[], int n, GanttChart *g)
{
    int remaining[MAX_PROCESSES];
    int completed = 0;
    int time = 0;
    int i;

    if (!prepare_run(p, n, g))
        return;

    for (i = 0; i < n; i++)
        remaining[i] = p[i].burst_time;

    while (completed < n) {
        int selected = -1;

        /* Select the highest-priority process currently ready. */
        for (i = 0; i < n; i++) {
            if (remaining[i] > 0 &&
                p[i].arrival_time <= time &&
                priority_better(p, i, selected)) {
                selected = i;
            }
        }

        /* No available process: CPU remains idle. */
        if (selected == -1) {
            if (!add_segment(g, -1, time, time + 1))
                return;

            time++;
            continue;
        }

        /* Execute one unit, then reconsider priorities. */
        if (!add_segment(g, p[selected].pid, time, time + 1))
            return;

        remaining[selected]--;
        time++;

        if (remaining[selected] == 0) {
            p[selected].completion_time = time;
            completed++;
        }
    }
}
