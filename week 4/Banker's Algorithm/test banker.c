#include <stdio.h>
#include <string.h>
#include "deadlock.h"

/* Print PASS or FAIL for one test. */
static void report(const char *name, int ok)
{
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
}

/* Build the classic 5-process, 3-resource state used in cases A, B, C. */
static void setup_case_a(System *s)
{
    static const int total[3] = {10, 5, 7};
    static const int alloc[5][3] = {
        {0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}
    };
    static const int max[5][3] = {
        {7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}
    };
    int i, j;

    memset(s, 0, sizeof(*s));
    s->n = 5;
    s->m = 3;
    for (j = 0; j < 3; j++)
        s->total[j] = total[j];

    for (i = 0; i < 5; i++) {
        s->active[i] = 1;
        for (j = 0; j < 3; j++) {
            s->alloc[i][j] = alloc[i][j];
            s->max[i][j] = max[i][j];
        }
    }

    /* available = total - sum of allocation */
    for (j = 0; j < 3; j++) {
        int sum = 0;
        for (i = 0; i < 5; i++)
            sum += s->alloc[i][j];
        s->available[j] = s->total[j] - sum;
    }

    calc_need(s);
}

int main(void)
{
    System s;
    int seq[MAX_P];
    int i, ok;
    int expected[5] = {1, 3, 4, 0, 2};

    /* ---- Case A: safe state, expected order P1 P3 P4 P0 P2 ---- */
    setup_case_a(&s);
    ok = is_safe(&s, seq);
    for (i = 0; i < 5; i++)
        if (seq[i] != expected[i])
            ok = 0;
    printf("Safe sequence found:");
    for (i = 0; i < s.n; i++)
        printf(" P%d", seq[i]);
    printf("\n");
    report("Case A (safe state, sequence P1 P3 P4 P0 P2)", ok);

    /* ---- Case B: P1 requests 1 0 2, must be granted (1) ---- */
    {
        int req[3] = {1, 0, 2};
        setup_case_a(&s);
        report("Case B (P1 requests 1 0 2 -> 1)",
               banker_check(&s, 1, req) == 1);
    }

    /* ---- Case C: P0 requests 0 2 0, P4 requests 3 3 0 ---- */
    {
        int req0[3] = {0, 2, 0};
        int req4[3] = {3, 3, 0};
        int req1[3] = {1, 0, 2};
        int j;

        /* C2 runs on the original state: P4 asking 3 3 0 leaves
         * Available = 0 0 2, so nobody can finish -> unsafe. */
        setup_case_a(&s);
        report("Case C2 (P4 requests 3 3 0 -> 0)",
               banker_check(&s, 4, req4) == 0);

        /* C1: on the ORIGINAL state, P0 asking 0 2 0 is actually safe
         * (the sequence P3 P1 P0 P2 P4 still works), so it returns 1. */
        report("Case C1a (original state, P0 requests 0 2 0 -> 1)",
               banker_check(&s, 0, req0) == 1);

        /* C1: the textbook "unsafe" result is for the state AFTER P1's
         * request 1 0 2 was granted (Available = 2 3 0). Apply that grant
         * by hand, then P0 asking 0 2 0 must be refused. */
        for (j = 0; j < 3; j++) {
            s.available[j] -= req1[j];
            s.alloc[1][j]  += req1[j];
        }
        calc_need(&s);
        report("Case C1b (after P1 gets 1 0 2, P0 requests 0 2 0 -> 0)",
               banker_check(&s, 0, req0) == 0);
    }

    /* ---- Case D: own unsafe state ----
     * 2 processes, 1 resource with 2 instances.
     * Each holds 1 and may ask for 1 more, but nothing is free,
     * so neither can ever finish. */
    {
        System u;
        memset(&u, 0, sizeof(u));
        u.n = 2;
        u.m = 1;
        u.total[0] = 2;
        u.alloc[0][0] = 1;  u.max[0][0] = 2;  u.active[0] = 1;
        u.alloc[1][0] = 1;  u.max[1][0] = 2;  u.active[1] = 1;
        u.available[0] = u.total[0] - u.alloc[0][0] - u.alloc[1][0];
        calc_need(&u);
        report("Case D (own unsafe state -> is_safe 0)",
               is_safe(&u, seq) == 0);
    }

    /* ---- Case E (extra): banker_check must not change the System ---- */
    {
        System before;
        int req[3] = {1, 0, 2};
        setup_case_a(&s);
        before = s;
        banker_check(&s, 1, req);
        report("Case E (banker_check leaves System unchanged)",
               memcmp(&before, &s, sizeof(s)) == 0);
    }

    return 0;
}
