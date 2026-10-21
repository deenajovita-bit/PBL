/*
 * detection.c  (Member 2)
 * Deadlock detection using Available, Allocation and the waiting Requests.
 * No printf, no scanf, no main() here.
 */

#include <stddef.h>
#include "deadlock.h"

/*
 * Find the deadlocked processes.
 * Put their numbers in dead[] (room for MAX_P entries) and return how many.
 */
int detect_deadlock(const System *s, int dead[])
{
    int work[MAX_R];
    int finish[MAX_P];
    int i, j;
    int changed;
    int count = 0;

    if (s == NULL || dead == NULL) {
        return 0;
    }

    /* Work starts as Available */
    for (j = 0; j < s->m; j++) {
        work[j] = s->available[j];
    }

    /* A process is already finished if it is terminated or holds nothing */
    for (i = 0; i < s->n; i++) {
        int holds = 0;
        for (j = 0; j < s->m; j++) {
            holds += s->alloc[i][j];
        }
        finish[i] = (!s->active[i] || holds == 0) ? 1 : 0;
    }

    /* Repeat: find a process whose waiting request fits in Work.
       It can finish, so it gives its allocation back to Work. */
    do {
        changed = 0;
        for (i = 0; i < s->n; i++) {
            int fits = 1;
            if (finish[i]) {
                continue;
            }
            for (j = 0; j < s->m; j++) {
                if (s->request[i][j] > work[j]) {
                    fits = 0;
                    break;
                }
            }
            if (fits) {
                for (j = 0; j < s->m; j++) {
                    work[j] += s->alloc[i][j];
                }
                finish[i] = 1;
                changed = 1;
            }
        }
    } while (changed);

    /* Every process still unfinished is deadlocked */
    for (i = 0; i < s->n; i++) {
        if (!finish[i]) {
            dead[count++] = i;
        }
    }
    return count;
}
