#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_PROCESSES 100
#define MAX_SEGMENTS  10000

/* ---------- shared data structures ---------- */

typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int priority;          /* smaller number = higher priority */
    int completion_time;   /* set by the algorithm */
    int waiting_time;      /* set by calculate_metrics */
    int turnaround_time;   /* set by calculate_metrics */
    int response_time;     /* set by calculate_metrics */
} Process;

typedef struct {
    int pid;               /* -1 means CPU idle */
    int start;
    int end;
} GanttSegment;

typedef struct {
    GanttSegment segments[MAX_SEGMENTS];
    int count;
} GanttChart;

/* ---------- scheduling algorithms (Members 1, 2, 3) ----------
 * Every algorithm must:
 *   - set p[i].completion_time for every process
 *   - append segments to g in chronological order (pid = -1 for idle)
 * Tie-break rule: earlier arrival first, then lower PID.
 */
void fcfs(Process p[], int n, GanttChart *g);
void sjf(Process p[], int n, GanttChart *g);
void srtf(Process p[], int n, GanttChart *g);
void priority_np(Process p[], int n, GanttChart *g);
void priority_p(Process p[], int n, GanttChart *g);
void round_robin(Process p[], int n, int quantum, GanttChart *g);

/* ---------- Gantt chart display (Member 3) ---------- */
void gantt_display(const GanttChart *g);

/* ---------- metrics and results (team leader) ---------- */

/* Fills turnaround, waiting and response time for every process.
 * Response time needs the first CPU start, so it reads the Gantt chart. */
void calculate_metrics(Process p[], int n, const GanttChart *g);

/* Prints the per-process table plus averages and CPU utilization. */
void display_results(const Process p[], int n);

/* Runs one algorithm by menu number (1-6) on p[], filling g. */
void run_algorithm(int choice, Process p[], int n, int quantum, GanttChart *g);

/* Name for a menu number (1-6). */
const char *algorithm_name(int choice);

/* Runs all six algorithms on a copy of orig[] and prints a comparison table. */
void compare_algorithms(const Process orig[], int n, int quantum);

#endif
