
#include "pagerep.h"

void fifo(const int *refs, int n, int nf, Result *r) {
    int frames[MAX_FRAMES], ptr = 0;

    for (int i = 0; i < nf; i++)
        frames[i] = EMPTY;

    r->total = n;
    r->hits = 0;
    r->faults = 0;

    for (int i = 0; i < n; i++) {
        int hit = (find_page(frames, nf, refs[i]) != -1);

        if (hit) {
            r->hits++;
        } else {
            r->faults++;

            int e = find_page(frames, nf, EMPTY);

            if (e != -1) {
                frames[e] = refs[i];
            } else {
                frames[ptr] = refs[i];
                ptr = (ptr + 1) % nf;
            }
        }

        save_step(r, i, refs[i], frames, nf, hit);
    }
}

void lru(const int *refs, int n, int nf, Result *r) {
    int frames[MAX_FRAMES], last[MAX_FRAMES];

    for (int i = 0; i < nf; i++) {
        frames[i] = EMPTY;
        last[i] = -1;
    }

    r->total = n;
    r->hits = 0;
    r->faults = 0;

    for (int i = 0; i < n; i++) {
        int slot = find_page(frames, nf, refs[i]);
        int hit = (slot != -1);

        if (hit) {
            r->hits++;
            last[slot] = i;
        } else {
            r->faults++;

            slot = find_page(frames, nf, EMPTY);

            if (slot == -1) {
                slot = 0;

                for (int s = 1; s < nf; s++) {
                    if (last[s] < last[slot]) {
                        slot = s;
                    }
                }
            }

            frames[slot] = refs[i];
            last[slot] = i;
        }

        save_step(r, i, refs[i], frames, nf, hit);
    }
}
