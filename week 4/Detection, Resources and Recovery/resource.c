/*
 * resource.c  (Member 2)
 * Resource management: set up the system, handle requests, release resources.
 * No printf, no scanf, no main() here.
 */

#include <stddef.h>
#include <string.h>
#include "deadlock.h"

/* Set up an empty system with n processes, m resource types and total[] instances */
void sys_init(System *s, int n, int m, const int total[])
{
    int i, j;

    if (s == NULL || total == NULL) {
        return;
    }
    if (n < 0) n = 0;
    if (m < 0) m = 0;
    if (n > MAX_P) n = MAX_P;
    if (m > MAX_R) m = MAX_R;

    memset(s, 0, sizeof(System));   /* all matrices start at zero */
    s->n = n;
    s->m = m;

    for (j = 0; j < m; j++) {
        s->total[j] = total[j];
        s->available[j] = total[j]; /* nothing is allocated yet */
    }
    for (i = 0; i < n; i++) {
        s->active[i] = 1;           /* every process starts running */
    }
}

/* Available = Total - (sum of Allocation of all processes) */
void update_available(System *s)
{
    int i, j;

    if (s == NULL) {
        return;
    }
    for (j = 0; j < s->m; j++) {
        int used = 0;
        for (i = 0; i < s->n; i++) {
            used += s->alloc[i][j];
        }
        s->available[j] = s->total[j] - used;
    }
}

/* Save a request as "waiting" for process p */
static void store_waiting(System *s, int p, const int req[])
{
    int j;
    for (j = 0; j < s->m; j++) {
        s->request[p][j] = req[j];
    }
}

/*
 * Process p asks for req[].
 * REQ_ERROR           : bad request (nothing changes)
 * REQ_WAIT            : not enough Available, p waits
 * REQ_DENIED_UNSAFE   : Banker's check said no, p waits
 * REQ_GRANTED         : resources given to p
 */
ReqResult request_resources(System *s, int p, const int req[], int use_banker)
{
    int j;
    int any = 0;       /* is at least one number above zero? */
    int waiting = 0;   /* does p already have a waiting request? */

    if (s == NULL || req == NULL || p < 0 || p >= s->n || !s->active[p]) {
        return REQ_ERROR;
    }

    for (j = 0; j < s->m; j++) {
        if (req[j] < 0) {
            return REQ_ERROR;
        }
        if (req[j] > 0) any = 1;
        if (s->request[p][j] > 0) waiting = 1;
    }
    if (!any || waiting) {
        return REQ_ERROR;
    }

    /* A process cannot ask for more than its remaining need */
    for (j = 0; j < s->m; j++) {
        if (req[j] > s->need[p][j]) {
            return REQ_ERROR;
        }
    }

    /* Not enough free resources: the process waits */
    for (j = 0; j < s->m; j++) {
        if (req[j] > s->available[j]) {
            store_waiting(s, p, req);
            return REQ_WAIT;
        }
    }

    /* Ask Banker's Algorithm if the user wants the safety check */
    if (use_banker && !banker_check(s, p, req)) {
        store_waiting(s, p, req);
        return REQ_DENIED_UNSAFE;
    }

    /* Grant the request */
    for (j = 0; j < s->m; j++) {
        s->alloc[p][j] += req[j];
        s->need[p][j] -= req[j];
        s->available[j] -= req[j];
    }
    return REQ_GRANTED;
}

/* Give all of p's resources back. p stays in the system with Need = Max. */
void release_resources(System *s, int p)
{
    int j;

    if (s == NULL || p < 0 || p >= s->n) {
        return;
    }
    for (j = 0; j < s->m; j++) {
        s->available[j] += s->alloc[p][j];
        s->alloc[p][j] = 0;
        s->request[p][j] = 0;
        s->need[p][j] = s->max[p][j];
    }
}
