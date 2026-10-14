
#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

/* Display the scheduling algorithm menu */
void show_menu(void)
{
    printf("\n========================================\n");
    printf("   PROCESS SCHEDULING SIMULATOR\n");
    printf("========================================\n");
    printf("1. FCFS (First Come First Served)\n");
    printf("2. SJF (Non-preemptive)\n");
    printf("3. SRTF (Preemptive SJF)\n");
    printf("4. Priority Scheduling (Non-preemptive)\n");
    printf("5. Priority Scheduling (Preemptive)\n");
    printf("6. Round Robin\n");
    printf("7. Compare All Algorithms\n");
    printf("0. Exit\n");
    printf("Enter your choice: ");
}

/* Read process details from the user */
int input_processes(Process p[], int n)
{
    int i;

    for (i = 0; i < n; i++)
    {
        p[i].pid = i + 1;

        printf("\nEnter details for Process P%d\n", p[i].pid);

        printf("Arrival Time: ");
        if (scanf("%d", &p[i].arrival_time) != 1 ||
            p[i].arrival_time < 0)
        {
            printf("Invalid arrival time.\n");
            return 0;
        }

        printf("Burst Time: ");
        if (scanf("%d", &p[i].burst_time) != 1 ||
            p[i].burst_time <= 0)
        {
            printf("Burst time must be greater than zero.\n");
            return 0;
        }

        printf("Priority (smaller number = higher priority): ");
        if (scanf("%d", &p[i].priority) != 1)
        {
            printf("Invalid priority.\n");
            return 0;
        }

        /* Initialize calculated fields */
        p[i].completion_time = 0;
        p[i].waiting_time = 0;
        p[i].turnaround_time = 0;
        p[i].response_time = 0;
    }

    return 1;
}

/* Display the process information */
void display_processes(const Process p[], int n)
{
    int i;

    printf("\n========== PROCESS DETAILS ==========\n");
    printf("PID\tArrival\tBurst\tPriority\n");

    for (i = 0; i < n; i++)
    {
        printf("P%d\t%d\t%d\t%d\n",
               p[i].pid,
               p[i].arrival_time,
               p[i].burst_time,
               p[i].priority);
    }
}

/* Clear the input buffer after invalid menu input */
static void clear_input_buffer(void)
{
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF)
    {
        /* Discard remaining characters */
    }
}

int main(void)
{
    Process processes[MAX_PROCESSES];
    GanttChart chart;

    int n;
    int choice;
    int quantum = 0;

    printf("========================================\n");
    printf("   CPU PROCESS SCHEDULING SIMULATOR\n");
    printf("========================================\n");

    /* Ask how many processes will be scheduled */
    printf("Enter number of processes (1-%d): ",
           MAX_PROCESSES);

    if (scanf("%d", &n) != 1 ||
        n < 1 || n > MAX_PROCESSES)
    {
        printf("Invalid number of processes.\n");
        return 1;
    }

    /* Get process details */
    if (!input_processes(processes, n))
    {
        return 1;
    }

    /* Show entered processes */
    display_processes(processes, n);

    /* Repeat the menu until the user chooses Exit */
    do
    {
        show_menu();

        if (scanf("%d", &choice) != 1)
        {
            printf("Please enter a valid number.\n");
            clear_input_buffer();
            continue;
        }

        if (choice == 0)
        {
            printf("Exiting simulator. Goodbye!\n");
            break;
        }

        if (choice < 1 || choice > 7)
        {
            printf("Invalid choice. Select 0 to 7.\n");
            continue;
        }

        /* Ask for a time quantum only for Round Robin */
        quantum = 0;

        if (choice == 6 || choice == 7)
        {
            printf("Enter Round Robin time quantum (> 0): ");

            if (scanf("%d", &quantum) != 1 || quantum <= 0)
            {
                printf("Time quantum must be greater than zero.\n");
                clear_input_buffer();
                continue;
            }
        }

        if (choice == 7)
        {
            /*
             * compare_algorithms() should copy the original process
             * list before running each algorithm so one algorithm's
             * results do not affect another's results.
             */
            compare_algorithms(processes, n, quantum);
        }
        else
        {
            /*
             * run_algorithm() selects and runs one algorithm.
             * It should initialize the Gantt chart and fill in
             * completion times.
             */
            chart.count = 0;

            printf("\nAlgorithm: %s\n", algorithm_name(choice));

            run_algorithm(choice, processes, n, quantum, &chart);

            /* Calculate metrics using the Gantt chart */
            calculate_metrics(processes, n, &chart);

            /* Show the CPU execution timeline */
            printf("\nGantt Chart:\n");
            gantt_display(&chart);

            /* Show per-process metrics and averages */
            display_results(processes, n);
        }

    } while (1);

    return 0;
}
