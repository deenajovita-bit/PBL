/*
 * round_robin.c  -  Round Robin scheduling for the Process Scheduling Simulator
 *
 * What this function does:
 *   - sets p[i].completion_time for every process
 *   - appends GanttSegments to g in time order (pid = -1 means CPU idle)
 *
 * What it does NOT do:
 *   - it does not change arrival_time or burst_time
 *   - it does not calculate waiting / turnaround / response time
 *     (calculate_metrics does that)
 *   - no printf / scanf here
 *
 * Tie-break rule (team rule): earlier arrival first, then lower PID.
 */

#include <stddef.h>
#include "scheduler.h"

/* Add one box to the Gantt chart. Returns 1 if added, 0 if the chart is full. */
static int add_segment(GanttChart *g, int pid, int start, int end)
{
    if (g->count >= MAX_SEGMENTS) {
        return 0;
    }
    g->segments[g->count].pid   = pid;
    g->segments[g->count].start = start;
    g->segments[g->count].end   = end;
    g->count++;
    return 1;
}

void round_robin(Process p[], int n, int quantum, GanttChart *g)
{
    int remaining[MAX_PROCESSES];  /* burst time still left (p[].burst_time stays untouched) */
    int order[MAX_PROCESSES];      /* process indexes sorted by arrival, then PID            */
    int queue[MAX_PROCESSES];      /* ready queue (circular), holds process indexes          */
    int front = 0;                 /* where we take from the queue                           */
    int size  = 0;                 /* how many are waiting in the queue                      */
    int m = 0;                     /* number of processes that really need CPU time          */
    int next = 0;                  /* next entry of order[] that has not arrived yet         */
    int done = 0;                  /* how many processes have finished                       */
    int time = 0;                  /* the clock                                              */
    int i, j;

    if (g == NULL) {
        return;
    }
    g->count = 0;                  /* start with an empty chart */

    /* Safe exit for bad input. */
    if (p == NULL || n <= 0 || n > MAX_PROCESSES || quantum <= 0) {
        return;
    }

    /* Step 1: copy burst times, and build the arrival order. */
    for (i = 0; i < n; i++) {
        remaining[i] = p[i].burst_time;
        if (p[i].burst_time <= 0) {
            /* A process that needs no CPU finishes the moment it arrives. */
            p[i].completion_time = p[i].arrival_time;
        } else {
            order[m++] = i;
        }
    }

    /* Step 2: insertion sort of order[] by (arrival_time, pid). */
    for (i = 1; i < m; i++) {
        int key = order[i];
        j = i - 1;
        while (j >= 0 &&
               (p[order[j]].arrival_time >  p[key].arrival_time ||
               (p[order[j]].arrival_time == p[key].arrival_time &&
                p[order[j]].pid          >  p[key].pid))) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    /* Step 3: main loop. Runs until every process has finished. */
    while (done < m) {

        /* 3a. Everyone who has arrived by 'time' joins the queue. */
        while (next < m && p[order[next]].arrival_time <= time) {
            queue[(front + size) % MAX_PROCESSES] = order[next];
            size++;
            next++;
        }

        /* 3b. Queue empty: the CPU is idle until the next process arrives. */
        if (size == 0) {
            int wake = p[order[next]].arrival_time;   /* next < m is guaranteed here */
            if (!add_segment(g, -1, time, wake)) {
                return;                                /* chart full, stop safely */
            }
            time = wake;
            continue;
        }

        /* 3c. Take the first process from the queue. */
        int cur = queue[front];
        front = (front + 1) % MAX_PROCESSES;
        size--;

        /* 3d. Run it for one quantum, or less if it needs less. */
        int run = (remaining[cur] < quantum) ? remaining[cur] : quantum;
        if (!add_segment(g, p[cur].pid, time, time + run)) {
            return;                                    /* chart full, stop safely */
        }
        time += run;
        remaining[cur] -= run;

        /* 3e. New arrivals during this slice join the queue FIRST ... */
        while (next < m && p[order[next]].arrival_time <= time) {
            queue[(front + size) % MAX_PROCESSES] = order[next];
            size++;
            next++;
        }

        /* 3f. ... and only THEN the unfinished process goes to the back. */
        if (remaining[cur] > 0) {
            queue[(front + size) % MAX_PROCESSES] = cur;
            size++;
        } else {
            p[cur].completion_time = time;
            done++;
        }
    }
}
