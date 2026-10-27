#include "pagerep.h"

double hit_ratio(const Result *r)   { return r->hits   * 100.0 / r->total; }
double fault_ratio(const Result *r) { return r->faults * 100.0 / r->total; }

/* returns the index of the algorithm with fewest faults */
int best_algo(const Result res[], int count) {
    int best = 0;
    for (int i = 1; i < count; i++)
        if (res[i].faults < res[best].faults) best = i;
    return best;
}
