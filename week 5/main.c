#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pagerep.h"

/* read exactly len bytes from a pipe */
static int read_all(int fd, void *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t k = read(fd, (char *)buf + got, len - got);
        if (k <= 0) return -1;
        got += k;
    }
    return 0;
}

/* write exactly len bytes to a pipe */
static int write_all(int fd, const void *buf, size_t len) {
    size_t put = 0;
    while (put < len) {
        ssize_t k = write(fd, (const char *)buf + put, len - put);
        if (k <= 0) return -1;
        put += k;
    }
    return 0;
}

static void print_steps(const char *name, const Result *r, int nf) {
    printf("\n--- %s ---\nPage |", name);
    for (int i = 0; i < nf; i++) printf(" F%d |", i + 1);
    printf(" Status\n");
    for (int i = 0; i < r->total; i++) {
        printf("%4d |", r->steps[i].page);
        for (int j = 0; j < nf; j++) {
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
    int refs[MAX_REFS], n, nf;
    void (*fn[3])(const int *, int, int, Result *) = { fifo, lru, optimal };
    const char *names[3] = { "FIFO", "LRU", "Optimal" };
    int fd[3][2];
    pid_t pid[3];
    static Result res[3];

    printf("Enter the page-reference string:\n");
    if (!fgets(line, sizeof line, stdin) || !parse_refs(line, refs, &n)) {
        printf("Invalid reference string.\n"); return 1;
    }
    printf("Enter the number of page frames:\n");
    if (scanf("%d", &nf) != 1 || nf <= 0 || nf > MAX_FRAMES) {
        printf("Frames must be 1 to %d.\n", MAX_FRAMES); return 1;
    }

    /* parent makes one child process per algorithm */
    for (int a = 0; a < 3; a++) {
        if (pipe(fd[a]) == -1) { perror("pipe"); return 1; }
        pid[a] = fork();
        if (pid[a] == -1) { perror("fork"); return 1; }

        if (pid[a] == 0) {                       /* CHILD */
            for (int k = 0; k <= a; k++) close(fd[k][0]);  /* close read ends */
            static Result r;
            fn[a](refs, n, nf, &r);              /* run the algorithm */
            if (write_all(fd[a][1], &r, sizeof r) == -1)
            { 
                perror("Pipe write failed"); 
                close(fd[a][1]);    
                _exit(1); }   /* send result to parent */
            close(fd[a][1]);
            _exit(0);
        }
        close(fd[a][1]);                         /* PARENT closes write end */
    }

    /* parent collects the results */
    for (int a = 0; a < 3; a++) {
        if (read_all(fd[a][0], &res[a], sizeof res[a]) == -1) {
            printf("Failed to read result of %s\n", names[a]); return 1;
        }
        close(fd[a][0]);
        waitpid(pid[a], NULL, 0);
    }

    for (int a = 0; a < 3; a++) print_steps(names[a], &res[a], nf);

    printf("\nAlgorithm | Hits | Faults | Fault Ratio\n");
    for (int a = 0; a < 3; a++)
        printf("%-9s | %4d | %6d | %.1f%%\n", names[a], res[a].hits, res[a].faults, fault_ratio(&res[a]));
    printf("Fewest faults: %s\n", names[best_algo(res, 3)]);
    return 0;
}
