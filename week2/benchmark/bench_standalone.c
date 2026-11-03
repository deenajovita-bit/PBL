/* =====================================================================
 * bench_standalone.c - Standalone (single-process) benchmark
 *
 * It uses the REAL simulator code (core/core.c) and calls
 * execute_command() directly. No socket, no shared memory, no
 * semaphores, no logger pipe: this is the "no IPC" baseline.
 *
 * Build and run:  see run_bench.sh
 * ===================================================================== */
#include <fcntl.h>
#include <unistd.h>

#define main core_main          /* rename Core's main so we can use ours */
#include "../core/core.c"
#undef main

#include "bench_common.h"

int main(int argc, char *argv[])
{
    long n = (argc > 1) ? atol(argv[1]) : 100000;

    cpu_init();
    memory_init();
    stack_init();
    queue_init();

    /* Core's replies would go to the UI. Here nobody listens,
     * so they go to /dev/null (one cheap write per command). */
    client_fd = open("/dev/null", O_WRONLY);
    logger_pipe_fd = -1;                    /* no Logger process */
    shared = NULL;                          /* no shared memory  */

    double *lat = malloc(sizeof(double) * (size_t)n);
    if (!lat) return 1;

    Message m;
    double t0 = now_us();

    for (long i = 0; i < n; i++)
    {
        make_command(&m, i);

        double a = now_us();
        execute_command(&m);
        lat[i] = now_us() - a;
    }

    double total_ms = (now_us() - t0) / 1000.0;

    int ok = (cpu.accumulator == 6 && stack.top == -1);

    double avg, p50, p95, p99;
    latency_stats(lat, n, &avg, &p50, &p95, &p99);

    printf("BENCH mode=standalone n=%ld total_ms=%.2f avg_us=%.3f p50_us=%.3f p95_us=%.3f p99_us=%.3f "
           "cpu_ms=%.2f rss_kb=%ld correct=%d\n",
           n, total_ms, avg, p50, p95, p99, self_cpu_ms(), self_peak_kb(), ok);

    free(lat);
    return ok ? 0 : 1;
}
