/*
 * fcfs_sjf.c
 * ---------------------------------------------------------------
 * Process Scheduling Simulator - FCFS and non-preemptive SJF.
 *
 * Both functions follow the same contract from scheduler.h:
 *   - set p[i].completion_time for every process
 *   - append Gantt segments to g in chronological order
 *     (pid = -1 means the CPU was idle)
 *   - tie-break: earlier arrival first, then lower PID
 *
 * The input array p[] is NEVER reordered. arrival_time and burst_time
 * are only read, never changed. Only completion_time is written.
 * (calculate_metrics() fills the other fields later.)
 * ---------------------------------------------------------------
 */

#include <stddef.h>     /* NULL */
#include "scheduler.h"

/* ================================================================
 * Small helper functions (static = private to this file, so they
 * cannot clash with names in other team members' files).
 * ================================================================ */

/*
 * Gets everything ready before an algorithm runs.
 * Returns 1 if it is safe to continue, 0 if the run must be skipped.
 *
 * It checks the pointers, the process count, the process data and
 * the Gantt capacity, and clears old output so that running an
 * algorithm twice never leaves stale results behind.
 */
static int prepare_run(Process p[], int n, GanttChart *g)
{
    int i;

    if (g == NULL) {
        return 0;
    }

    g->count = 0;                    /* reset the Gantt chart: start empty */

    if (p == NULL || n <= 0 || n > MAX_PROCESSES) {
        return 0;                    /* bad process count or array */
    }

    /* Reject impossible input (negative arrival, zero/negative burst). */
    for (i = 0; i < n; i++) {
        if (p[i].arrival_time < 0 || p[i].burst_time <= 0) {
            return 0;
        }
    }

    /*
     * Worst case, every process needs one idle segment plus one run
     * segment, so we need room for 2 * n segments. Checking up front
     * means we never end up with a half-finished chart.
     */
    if (2 * n > MAX_SEGMENTS) {
        return 0;
    }

    /* Clear the old output from any previous run. */
    for (i = 0; i < n; i++) {
        p[i].completion_time = 0;
    }

    return 1;
}

/*
 * Appends one segment (pid, start, end) to the Gantt chart.
 * Returns 1 on success, 0 if the chart is full.
 */
static int add_segment(GanttChart *g, int pid, int start, int end)
{
    if (g->count >= MAX_SEGMENTS) {
        return 0;                    /* no room: do not write past the array */
    }
    g->segments[g->count].pid   = pid;
    g->segments[g->count].start = start;
    g->segments[g->count].end   = end;
    g->count++;
    return 1;
}

/*
 * SJF comparison: returns 1 if process a should be chosen over b.
 *   1st rule: shorter burst time wins
 *   2nd rule: (tie) earlier arrival time wins
 *   3rd rule: (tie) lower PID wins
 */
static int sjf_is_better(const Process *a, const Process *b)
{
    if (a->burst_time != b->burst_time) {
        return a->burst_time < b->burst_time;
    }
    if (a->arrival_time != b->arrival_time) {
        return a->arrival_time < b->arrival_time;
    }
    return a->pid < b->pid;
}

/* ================================================================
 * FCFS - First Come, First Served
 * ================================================================ */
void fcfs(Process p[], int n, GanttChart *g)
{
    int order[MAX_PROCESSES];   /* indexes of p[], sorted by arrival, then PID */
    int i, j, key;
    int time = 0;               /* the simulated clock */

    if (!prepare_run(p, n, g)) {
        return;
    }

    /*
     * Step 1: work out the arrival order WITHOUT touching p[].
     * order[] holds indexes (0..n-1). We sort the indexes with a simple
     * insertion sort: earlier arrival first, and lower PID if arrival
     * times are equal.
     */
    for (i = 0; i < n; i++) {
        order[i] = i;
    }
    for (i = 1; i < n; i++) {
        key = order[i];
        j = i - 1;
        while (j >= 0 &&
               (p[order[j]].arrival_time > p[key].arrival_time ||
                (p[order[j]].arrival_time == p[key].arrival_time &&
                 p[order[j]].pid > p[key].pid))) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    /* Step 2: run the processes one by one in that order. */
    for (i = 0; i < n; i++) {
        Process *cur = &p[order[i]];

        /* If the CPU is free but the next process has not arrived yet,
         * the CPU sits idle until it arrives. Record that as pid = -1. */
        if (time < cur->arrival_time) {
            if (!add_segment(g, -1, time, cur->arrival_time)) {
                return;
            }
            time = cur->arrival_time;
        }

        /* Run the process to completion (FCFS never interrupts). */
        if (!add_segment(g, cur->pid, time, time + cur->burst_time)) {
            return;
        }
        time += cur->burst_time;
        cur->completion_time = time;     /* it finishes when the clock stops */
    }
}

/* ================================================================
 * SJF - Shortest Job First (non-preemptive)
 * ================================================================ */
void sjf(Process p[], int n, GanttChart *g)
{
    int done[MAX_PROCESSES];    /* done[i] = 1 once process i has finished */
    int completed = 0;          /* how many processes have finished */
    int time = 0;               /* the simulated clock */
    int i;

    if (!prepare_run(p, n, g)) {
        return;
    }

    for (i = 0; i < n; i++) {
        done[i] = 0;
    }

    while (completed < n) {
        int best = -1;          /* index of the chosen process, -1 = none yet */

        /* Step 1: among processes that have ARRIVED and are not finished,
         * pick the best one (shortest burst, then arrival, then PID). */
        for (i = 0; i < n; i++) {
            if (done[i] || p[i].arrival_time > time) {
                continue;       /* finished already, or not here yet */
            }
            if (best == -1 || sjf_is_better(&p[i], &p[best])) {
                best = i;
            }
        }

        /* Step 2a: nobody is waiting -> CPU is idle until the next arrival. */
        if (best == -1) {
            int next_arrival = -1;

            /* Every unfinished process arrives after 'time',
             * so the smallest arrival time is the next event. */
            for (i = 0; i < n; i++) {
                if (!done[i] &&
                    (next_arrival == -1 || p[i].arrival_time < next_arrival)) {
                    next_arrival = p[i].arrival_time;
                }
            }

            if (!add_segment(g, -1, time, next_arrival)) {
                return;
            }
            time = next_arrival;
            continue;           /* go back and pick again */
        }

        /* Step 2b: run the chosen process to the end (non-preemptive:
         * a shorter job arriving meanwhile must wait). */
        if (!add_segment(g, p[best].pid, time, time + p[best].burst_time)) {
            return;
        }
        time += p[best].burst_time;
        p[best].completion_time = time;
        done[best] = 1;
        completed++;
    }
}
