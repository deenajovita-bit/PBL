#include <emscripten.h>
#include "pagerep.h"

/* algo: 0=FIFO 1=LRU 2=Optimal
   out gets, for each step: page, hit, frame1..frameN
   returns number of faults, or -1 if input is bad */
EMSCRIPTEN_KEEPALIVE
int simulate(int algo, const int *refs, int n, int nf, int *out) {
    static Result r;
    if (n <= 0 || n > MAX_REFS || nf <= 0 || nf > MAX_FRAMES) return -1;
    if (algo == 0) fifo(refs, n, nf, &r);
    else if (algo == 1) lru(refs, n, nf, &r);
    else if (algo == 2) optimal(refs, n, nf, &r);
    else return -1;

    int k = 0;
    for (int i = 0; i < n; i++) {
        out[k++] = r.steps[i].page;
        out[k++] = r.steps[i].hit;
        for (int j = 0; j < nf; j++) out[k++] = r.steps[i].frames[j];
    }
    return r.faults;
}
