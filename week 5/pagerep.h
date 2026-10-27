#ifndef PAGEREP_H
#define PAGEREP_H

#define MAX_REFS 100
#define MAX_FRAMES 10
#define EMPTY -1

typedef struct {
    int page;
    int frames[MAX_FRAMES];
    int hit;                 /* 1 = hit, 0 = fault */
} Step;

typedef struct {
    Step steps[MAX_REFS];
    int total, hits, faults;
} Result;

void fifo   (const int *refs, int n, int nf, Result *r);
void lru    (const int *refs, int n, int nf, Result *r);
void optimal(const int *refs, int n, int nf, Result *r);

int    parse_refs(const char *text, int *refs, int *n);
int    best_algo(const Result res[], int count);
double hit_ratio(const Result *r);
double fault_ratio(const Result *r);

/* find a value in the frames, returns slot number or -1 */
static inline int find_page(const int *f, int nf, int p) {
    for (int i = 0; i < nf; i++)
        if (f[i] == p) return i;
    return -1;
}

/* save the state after one request */
static inline void save_step(Result *r, int idx, int page,
                             const int *f, int nf, int hit) {
    r->steps[idx].page = page;
    r->steps[idx].hit = hit;
    for (int i = 0; i < MAX_FRAMES; i++)
        r->steps[idx].frames[i] = (i < nf) ? f[i] : EMPTY;
}

#endif
