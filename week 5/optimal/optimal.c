#include "pagerep.h"

void optimal(const int *refs, int n, int nf, Result *r) {
    int frames[MAX_FRAMES];
    for (int i = 0; i < nf; i++) frames[i] = EMPTY;
    r->total = n; r->hits = 0; r->faults = 0;

    for (int i = 0; i < n; i++) {
        int hit = (find_page(frames, nf, refs[i]) != -1);
        if (hit) {
            r->hits++;
        } else {
            r->faults++;
            int slot = find_page(frames, nf, EMPTY);
            if (slot == -1) {
                /* replace the page used farthest in the future */
                int far = -1;
                slot = 0;
                for (int s = 0; s < nf; s++) {
                    int next = n + 1;             /* never used again */
                    for (int j = i + 1; j < n; j++)
                        if (refs[j] == frames[s]) { next = j; break; }
                    if (next > far) { far = next; slot = s; }
                }
            }
            frames[slot] = refs[i];
        }
        save_step(r, i, refs[i], frames, nf, hit);
    }
}
