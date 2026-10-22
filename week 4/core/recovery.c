/*
 * recovery.c  (Member 2)
 * Deadlock recovery: choose a victim process and terminate it.
 * No printf, no scanf, no main() here.
 */

#include <stddef.h>
#include "deadlock.h"

/* Total number of resource instances held by process p */
static int total_held(const System *s, int p)
{
    int j, sum = 0;
    for (j = 0; j < s->m; j++) {
        sum += s->alloc[p][j];
    }
    return sum;
}

/*
 * Pick the deadlocked process that holds the fewest resources
 * (cheapest to terminate). Tie: smaller process number.
 * Returns -1 if there is nothing to pick.
 */
int pick_victim(const System *s, const int dead[], int count)
{
    int k;
    int victim = -1;
    int best = 0;

    if (s == NULL || dead == NULL || count <= 0) {
        return -1;
    }
    for (k = 0; k < count; k++) {
        int p = dead[k];
        int held;
        if (p < 0 || p >= s->n) {
            continue;
        }
        held = total_held(s, p);
        if (victim == -1 || held < best || (held == best && p < victim)) {
            victim = p;
            best = held;
        }
    }
    return victim;
}

/*
 * Terminate process p, free its resources, then give resources to
 * other processes that were waiting and can now continue.
 */
void recover_terminate(System *s, int p)
{
    int i, j;
    int changed;

    if (s == NULL || p < 0 || p >= s->n) {
        return;
    }

    /* 1. Free everything p holds */
    release_resources(s, p);

    /* 2. Mark p as terminated and clear its data */
    s->active[p] = 0;
    for (j = 0; j < s->m; j++) {
        s->max[p][j] = 0;
        s->need[p][j] = 0;
        s->request[p][j] = 0;
    }

    /* 3. Grant waiting requests that now fit. Repeat until nothing changes,
          because one grant can change what is available. */
    do {
        changed = 0;
        for (i = 0; i < s->n; i++) {
            int waiting = 0;
            int fits = 1;

            if (!s->active[i]) {
                continue;
            }
            for (j = 0; j < s->m; j++) {
                if (s->request[i][j] > 0) waiting = 1;
                if (s->request[i][j] > s->available[j]) fits = 0;
            }
            if (waiting && fits) {
                for (j = 0; j < s->m; j++) {
                    s->alloc[i][j] += s->request[i][j];
                    s->need[i][j] -= s->request[i][j];
                    s->available[j] -= s->request[i][j];
                    s->request[i][j] = 0;
                }
                changed = 1;
            }
        }
    } while (changed);
}
/*
 * Preempt resources from process p.
 *
 * Unlike termination:
 * - process p stays active
 * - its currently allocated resources are returned
 * - its allocation becomes zero
 * - its need is recalculated
 *
 * Returns:
 *   1 = resources were preempted
 *   0 = nothing could be preempted
 */
int recover_preempt(System *s, int p)
{
    int j;
    int released = 0;

    if (s == NULL || p < 0 || p >= s->n) {
        return 0;
    }

    if (!s->active[p]) {
        return 0;
    }

    /*
     * Return all resources held by
     * this process to Available.
     */
    for (j = 0; j < s->m; j++) {

        if (s->alloc[p][j] > 0) {

            s->available[j] +=
                s->alloc[p][j];

            released +=
                s->alloc[p][j];

            s->alloc[p][j] = 0;
        }
    }

    /*
     * Process is NOT terminated.
     * Recalculate its remaining need.
     */
    for (j = 0; j < s->m; j++) {

        s->need[p][j] =
            s->max[p][j] -
            s->alloc[p][j];

        if (s->need[p][j] < 0) {
            s->need[p][j] = 0;
        }
    }

    /*
     * Clear the old pending request.
     * The process can request resources
     * again later.
     */
    for (j = 0; j < s->m; j++) {

        s->request[p][j] = 0;
    }

    return released > 0;
}