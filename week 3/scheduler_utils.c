
#include <stdio.h>
#include <string.h>
#include "scheduler.h"

const char *algorithm_name(int choice)
{
    switch (choice) {
        case 1: return "FCFS";
        case 2: return "SJF (Non-preemptive)";
        case 3: return "SRTF";
        case 4: return "Priority (Non-preemptive)";
        case 5: return "Priority (Preemptive)";
        case 6: return "Round Robin";
        default: return "Unknown";
    }
}

void run_algorithm(int choice, Process p[], int n, int quantum,
                   GanttChart *g)
{
    if (g == NULL || p == NULL || n <= 0)
        return;

    g->count = 0;

    switch (choice) {
        case 1: fcfs(p, n, g); break;
        case 2: sjf(p, n, g); break;
        case 3: srtf(p, n, g); break;
        case 4: priority_np(p, n, g); break;
        case 5: priority_p(p, n, g); break;
        case 6:
            if (quantum > 0)
                round_robin(p, n, quantum, g);
            else
                printf("Error: quantum must be positive.\n");
            break;
        default:
            printf("Invalid algorithm choice.\n");
    }
}

void calculate_metrics(Process p[], int n, const GanttChart *g)
{
    if (p == NULL || g == NULL || n <= 0)
        return;

    for (int i = 0; i < n; i++) {
        p[i].turnaround_time =
            p[i].completion_time - p[i].arrival_time;

        p[i].waiting_time =
            p[i].turnaround_time - p[i].burst_time;

        p[i].response_time = -1;

        for (int j = 0; j < g->count && j < MAX_SEGMENTS; j++) {
            if (g->segments[j].pid == p[i].pid) {
                p[i].response_time =
                    g->segments[j].start - p[i].arrival_time;
                break;
            }
        }
    }
}

void display_results(const Process p[], int n)
{
    if (p == NULL || n <= 0)
        return;

    double total_tat = 0;
    double total_wt = 0;
    double total_rt = 0;
    int completed = 0;
    int earliest = p[0].arrival_time;
    int latest = p[0].completion_time;

    printf("\nScheduling Results\n");
    printf("PID\tAT\tBT\tCT\tTAT\tWT\tRT\n");

    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
               p[i].pid,
               p[i].arrival_time,
               p[i].burst_time,
               p[i].completion_time,
               p[i].turnaround_time,
               p[i].waiting_time,
               p[i].response_time);

        total_tat += p[i].turnaround_time;
        total_wt += p[i].waiting_time;

        if (p[i].response_time >= 0)
            total_rt += p[i].response_time;

        if (p[i].completion_time > latest)
            latest = p[i].completion_time;

        if (p[i].arrival_time < earliest)
            earliest = p[i].arrival_time;

        completed++;
    }

    printf("\nAverage Turnaround Time: %.2f\n",
           total_tat / completed);
    printf("Average Waiting Time:    %.2f\n",
           total_wt / completed);
    printf("Average Response Time:   %.2f\n",
           total_rt / completed);

    int elapsed = latest - earliest;
    int total_burst = 0;

    for (int i = 0; i < n; i++)
        total_burst += p[i].burst_time;

    if (elapsed > 0) {
        printf("CPU Utilization:         %.2f%%\n",
               100.0 * total_burst / elapsed);
    } else {
        printf("CPU Utilization:         N/A\n");
    }
}

void compare_algorithms(const Process orig[], int n, int quantum)
{
    if (orig == NULL || n <= 0 || n > MAX_PROCESSES)
        return;

    printf("\n========== ALGORITHM COMPARISON ==========\n");

    for (int choice = 1; choice <= 6; choice++) {
        Process copy[MAX_PROCESSES];
        GanttChart chart;

        memcpy(copy, orig, (size_t)n * sizeof(Process));
        chart.count = 0;

        printf("\n--- %s ---\n", algorithm_name(choice));

        run_algorithm(choice, copy, n, quantum, &chart);
        calculate_metrics(copy, n, &chart);
        gantt_display(&chart);
        display_results(copy, n);
    }
}


