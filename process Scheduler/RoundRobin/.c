#include <stdio.h>
#define MAX 20
typedef struct {
    int pid;
    int arrival;
    int burst;
    int priority;
    int completion;
    int tat;
    int wt;
    int rt;
} Process;
/* Display process results */
void printTable(Process p[], int n) {
    float totalTat = 0, totalWt = 0, totalRt = 0;
    printf("\nPID\tAT\tBT\tCT\tTAT\tWT\tRT\n");
    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
               p[i].pid, p[i].arrival, p[i].burst,
               p[i].completion, p[i].tat, p[i].wt, p[i].rt);
        totalTat += p[i].tat;
        totalWt += p[i].wt;
        totalRt += p[i].rt;
    }
    printf("Average TAT = %.2f\n", totalTat / n);
    printf("Average WT  = %.2f\n", totalWt / n);
    printf("Average RT  = %.2f\n", totalRt / n);
}
/* Jovita: Round Robin Scheduling */
void roundRobin(Process p[], int n, int quantum) {
    int remaining[MAX];
    int firstStart[MAX];
    int added[MAX] = {0};
    int queue[MAX * (MAX + 1)];
    int front = 0, rear = 0;
    int time = 0, completed = 0;
    if (n <= 0 || n > MAX || quantum <= 0) {
        printf("Invalid number of processes or time quantum.\n");
        return;
    }
    for (int i = 0; i < n; i++) {
        if (p[i].arrival < 0 || p[i].burst <= 0) {
            printf("Arrival must be >= 0 and burst must be > 0.\n");
            return;
        }
        remaining[i] = p[i].burst;
        firstStart[i] = -1;
        p[i].completion = 0;
        p[i].tat = 0;
        p[i].wt = 0;
        p[i].rt = 0;
    }
    printf("\nGantt Chart:\n");
    while (completed < n) {
        /* Add newly arrived processes */
        for (int i = 0; i < n; i++) {
            if (!added[i] && p[i].arrival <= time) {
                queue[rear++] = i;
                added[i] = 1;
            }
        }

        /* Handle CPU idle time */
        if (front == rear) {
            int nextArrival = -1;

            for (int i = 0; i < n; i++) {
                if (!added[i] &&
                    (nextArrival == -1 ||
                     p[i].arrival < nextArrival)) {
                    nextArrival = p[i].arrival;
                }
            }
            if (nextArrival != -1) {
                printf("| IDLE (%d-%d) ", time, nextArrival);
                time = nextArrival;
            }
            continue;
        }
        /* Select the next process */
        int i = queue[front++];
        /* Record response time */
        if (firstStart[i] == -1) {
            firstStart[i] = time;
            p[i].rt = time - p[i].arrival;
        }
        /* Execute for one time quantum or less */
        int run = remaining[i] < quantum
                    ? remaining[i] : quantum;
        printf("| P%d (%d-%d) ",
               p[i].pid, time, time + run);
        time += run;
        remaining[i] -= run;
        /* Add processes that arrived during execution */
        for (int j = 0; j < n; j++) {
            if (!added[j] && p[j].arrival <= time) {
                queue[rear++] = j;
                added[j] = 1;
            }
        }

        /* Requeue or complete the process */
        if (remaining[i] > 0) {
            queue[rear++] = i;
        } else {
            p[i].completion = time;
            p[i].tat = p[i].completion - p[i].arrival;
            p[i].wt = p[i].tat - p[i].burst;
            completed++;
        }
    }

    printf("|\n");
    printTable(p, n);
}

