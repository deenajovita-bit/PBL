#include "deadlock.h"

/*
 * calc_need: need = max - alloc for every process and resource.
 * A terminated process needs nothing, so its need is set to 0.
 */
void calc_need(System *s)
{
    int i, j;

    for (i = 0; i < s->n; i++) {
        for (j = 0; j < s->m; j++) {
            if (s->active[i])
                s->need[i][j] = s->max[i][j] - s->alloc[i][j];
            else
                s->need[i][j] = 0;
        }
    }
}

/*
 * is_safe: the safety algorithm.
 * Returns 1 if every active process can finish, 0 otherwise.
 * seq[] gets the safe order (process numbers); unused entries are -1.
 */
int is_safe(const System *s, int seq[])
{
    int work[MAX_R];
    int finish[MAX_P];
    int i, j, k;
    int count = 0;       /* how many processes are in seq[] so far */
    int active_count = 0; /* how many active processes there are */
    int progress;

    /* Step 1: start with no sequence at all (all entries -1). */
    for (k = 0; k < s->n; k++)
        seq[k] = -1;

    /* Step 2: Work = Available. */
    for (j = 0; j < s->m; j++)
        work[j] = s->available[j];

    /* Step 3: terminated processes are already "finished" and are skipped. */
    for (i = 0; i < s->n; i++) {
        finish[i] = s->active[i] ? 0 : 1;
        if (s->active[i])
            active_count++;
    }

    /* Step 4: keep making passes until a full pass finds nothing new. */
    do {
        progress = 0;
        for (i = 0; i < s->n; i++) {
            int can_run = 1;

            if (finish[i])
                continue;

            /* Can process i get everything it still needs from Work? */
            for (j = 0; j < s->m; j++) {
                if (s->need[i][j] > work[j]) {
                    can_run = 0;
                    break;
                }
            }

            if (can_run) {
                /* Process i runs to the end and returns its resources. */
                for (j = 0; j < s->m; j++)
                    work[j] += s->alloc[i][j];
                finish[i] = 1;
                seq[count++] = i;
                progress = 1;
            }
        }
    } while (progress);

    /* Step 5: safe only if every active process finished. */
    return (count == active_count) ? 1 : 0;
}

/*
 * banker_check: returns 1 only if the request is valid AND granting it
 * leaves the system safe. The real System is never changed.
 */
int banker_check(System *s, int p, const int req[])
{
    System copy;
    int seq[MAX_P];
    int j;

    /* Check the process number and that the process is still running. */
    if (p < 0 || p >= s->n || !s->active[p])
        return 0;

    /* The request must be valid: not negative, within need, within available. */
    for (j = 0; j < s->m; j++) {
        if (req[j] < 0)
            return 0;
        if (req[j] > s->need[p][j])
            return 0;
        if (req[j] > s->available[j])
            return 0;
    }

    /* Work on a copy so the caller's System stays untouched. */
    copy = *s;

    /* Pretend to grant the request on the copy. */
    for (j = 0; j < copy.m; j++) {
        copy.available[j]  -= req[j];
        copy.alloc[p][j]   += req[j];
        copy.need[p][j]    -= req[j];
    }

    /* Grant is allowed only if the pretend state is safe. */
    return is_safe(&copy, seq);
}
