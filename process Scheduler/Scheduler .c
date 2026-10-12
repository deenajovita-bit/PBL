
#include <stdio.h>

#define MAX 20

typedef struct {
    int pid;         /* process number: 1, 2, 3 ...            */
    int arrival;     /* arrival time (input)                   */
    int burst;       /* burst time (input)                     */
    int priority;    /* smaller number = higher priority       */
    int completion;  /* time when the process finishes         */
    int tat;         /* turnaround time = completion - arrival */
    int wt;          /* waiting time = tat - burst             */
    int rt;          /* response time = first CPU time - arrival */
} Process;

/* Prints the table and the averages. Call this at the end of your function. */
void printTable(Process p[], int n) {
    float totalTat = 0, totalWt = 0, totalRt = 0;
    printf("\nPID\tAT\tBT\tCT\tTAT\tWT\tRT\n");
    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n", p[i].pid, p[i].arrival, p[i].burst,
               p[i].completion, p[i].tat, p[i].wt, p[i].rt);
        totalTat += p[i].tat;
        totalWt += p[i].wt;
        totalRt += p[i].rt;
    }
    printf("Average TAT = %.2f\n", totalTat / n);
    printf("Average WT  = %.2f\n", totalWt / n);
    printf("Average RT  = %.2f\n", totalRt / n);
}

/* ---------- Each person fills in ONLY their own function ---------- */

void fcfs(Process p[], int n) {
    /* Rithvik: TODO */
    printf("FCFS not written yet\n");
}

void sjf(Process p[], int n) {
    /* Rishabh: TODO */
    printf("SJF not written yet\n");
}

void priorityScheduling(Process p[], int n) {
    /* Nishant: TODO */
    printf("Priority not written yet\n");
}

void roundRobin(Process p[], int n, int quantum) {
    /* Jovita: TODO */
    printf("Round Robin not written yet\n");
}

/* ---------- main: reads input and shows a menu ---------- */

int main() {
    Process p[MAX];
    int n, choice, quantum;

    printf("How many processes? ");
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        printf("P%d - arrival time, burst time, priority: ", i + 1);
        scanf("%d %d %d", &p[i].arrival, &p[i].burst, &p[i].priority);
    }

    printf("\n1. FCFS\n2. SJF\n3. Priority\n4. Round Robin\nChoose: ");
    scanf("%d", &choice);

    if (choice == 1) fcfs(p, n);
    else if (choice == 2) sjf(p, n);
    else if (choice == 3) priorityScheduling(p, n);
    else if (choice == 4) {
        printf("Time quantum: ");
        scanf("%d", &quantum);
        roundRobin(p, n, quantum);
    } else printf("Wrong choice\n");

    return 0;
}