#include <stdio.h>
#include "pagerep.h"

static void print_steps(const char *name, const Result *r, int nf) {
    printf("\n--- %s ---\nPage |", name);
    for (int i = 0; i < nf; i++) printf(" F%d |", i + 1);
    printf(" Status\n");
    for (int i = 0; i < r->total; i++) {
        printf("%4d |", r->steps[i].page);
        for (int j = 0; j < nf; j++) {
            if (r->steps[j].frames[j] == EMPTY && 0) {}
            if (r->steps[i].frames[j] == EMPTY) printf("  - |");
            else printf(" %2d |", r->steps[i].frames[j]);
        }
        printf(" %s\n", r->steps[i].hit ? "Hit" : "Fault");
    }
    printf("Hits: %d  Faults: %d  Hit%%: %.1f  Fault%%: %.1f\n",
           r->hits, r->faults, hit_ratio(r), fault_ratio(r));
}

int main(void) {
    char line[1024];
    int refs[MAX_REFS], n, nf, choice;
    void (*fn[3])(const int *, int, int, Result *) = { fifo, lru, optimal };
    const char *names[3] = { "FIFO", "LRU", "Optimal" };
    Result res[3];

    printf("Enter the page-reference string:\n");
    if (!fgets(line, sizeof line, stdin) || !parse_refs(line, refs, &n)) {
        printf("Invalid reference string.\n"); return 1;
    }
    printf("Enter the number of page frames:\n");
    if (scanf("%d", &nf) != 1 || nf <= 0 || nf > MAX_FRAMES) {
        printf("Frames must be 1 to %d.\n", MAX_FRAMES); return 1;
    }
    printf("Select: 1.FIFO 2.LRU 3.Optimal 4.Compare all\nChoice: ");
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > 4) {
        printf("Invalid choice.\n"); return 1;
    }

    int from = (choice == 4) ? 0 : choice - 1;
    int to   = (choice == 4) ? 2 : choice - 1;
    for (int a = from; a <= to; a++) {
        fn[a](refs, n, nf, &res[a]);
        print_steps(names[a], &res[a], nf);
    }

    if (choice == 4) {
        printf("\nAlgorithm | Hits | Faults | Fault Ratio\n");
        for (int a = 0; a < 3; a++)
            printf("%-9s | %4d | %6d | %.1f%%\n", names[a],
                   res[a].hits, res[a].faults, fault_ratio(&res[a]));
        printf("Fewest faults: %s\n", names[best_algo(res, 3)]);
    }
    return 0;
}
