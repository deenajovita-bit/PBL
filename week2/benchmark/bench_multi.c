/* =====================================================================
 * bench_multi.c - Multi-process benchmark client
 *
 * Acts like the UI process: connects to the REAL running Core
 * (socket + shared memory + semaphores; Core also feeds the Logger
 * through the pipe) and sends the same workload as bench_standalone.
 * Each command's time is measured from "send" to "reply received".
 *
 * Usage: ./bench_multi <n> <core_pid>
 * ===================================================================== */
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>

#include "../sim_interface.h"
#include "../sync.h"
#include "bench_common.h"

static int write_all(int fd, const void *buf, size_t size)
{
    const char *p = buf; size_t d = 0;
    while (d < size) {
        ssize_t k = write(fd, p + d, size - d);
        if (k < 0) { if (errno == EINTR) continue; return -1; }
        d += (size_t)k;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t size)
{
    char *p = buf; size_t d = 0;
    while (d < size) {
        ssize_t k = read(fd, p + d, size - d);
        if (k == 0) return 0;
        if (k < 0) { if (errno == EINTR) continue; return -1; }
        d += (size_t)k;
    }
    return 1;
}

static int find_child(int parent)
{
    char cmd[64], line[32];
    snprintf(cmd, sizeof(cmd), "pgrep -P %d", parent);
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    int pid = -1;
    if (fgets(line, sizeof(line), p)) pid = atoi(line);
    pclose(p);
    return pid;
}

int main(int argc, char *argv[])
{
    long n = (argc > 1) ? atol(argv[1]) : 100000;
    int core_pid = (argc > 2) ? atoi(argv[2]) : -1;

    signal(SIGPIPE, SIG_IGN);

    if (sync_open() == -1 || wait_core_ready(10) == -1) return 1;

    int sfd = shm_open(SIM_SHM_NAME, O_RDWR, 0666);
    if (sfd == -1) { perror("shm_open"); return 1; }
    SharedState *sh = mmap(NULL, sizeof(SharedState), PROT_READ | PROT_WRITE, MAP_SHARED, sfd, 0);
    close(sfd);
    if (sh == MAP_FAILED) { perror("mmap"); return 1; }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SIM_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) { perror("connect"); return 1; }

    double *lat = malloc(sizeof(double) * (size_t)n);
    if (!lat) return 1;

    long errors = 0;
    Message m, r;
    double t0 = now_us();

    for (long i = 0; i < n; i++)
    {
        make_command(&m, i);

        double a = now_us();
        if (write_all(sock, &m, sizeof(m)) == -1) { perror("send"); return 1; }
        if (read_all(sock, &r, sizeof(r)) != 1)   { fprintf(stderr, "Core closed early\n"); return 1; }
        lat[i] = now_us() - a;

        if (r.type == RESPONSE_ERROR) errors++;
    }

    double total_ms = (now_us() - t0) / 1000.0;

    /* Check the state through shared memory (also uses the semaphore). */
    lock_state();
    int ok = (sh->accumulator == 6 && sh->stack_top == -1 && errors == 0);
    unlock_state();

    /* Take CPU and memory of Core and Logger BEFORE telling Core to exit. */
    int logger_pid = (core_pid > 0) ? find_child(core_pid) : -1;
    double core_cpu   = (core_pid   > 0) ? proc_cpu_ms(core_pid)   : -1;
    double logger_cpu = (logger_pid > 0) ? proc_cpu_ms(logger_pid) : -1;
    long   core_kb    = (core_pid   > 0) ? proc_peak_kb(core_pid)   : -1;
    long   logger_kb  = (logger_pid > 0) ? proc_peak_kb(logger_pid) : -1;

    double avg, p50, p95, p99;
    latency_stats(lat, n, &avg, &p50, &p95, &p99);

    double ui_cpu = self_cpu_ms();
    long   ui_kb  = self_peak_kb();

    printf("BENCH mode=multi n=%ld total_ms=%.2f avg_us=%.3f p50_us=%.3f p95_us=%.3f p99_us=%.3f "
           "cpu_ms=%.2f rss_kb=%ld ui_cpu_ms=%.2f core_cpu_ms=%.2f logger_cpu_ms=%.2f "
           "ui_kb=%ld core_kb=%ld logger_kb=%ld errors=%ld correct=%d\n",
           n, total_ms, avg, p50, p95, p99,
           ui_cpu + core_cpu + logger_cpu, ui_kb + core_kb + logger_kb,
           ui_cpu, core_cpu, logger_cpu, ui_kb, core_kb, logger_kb, errors, ok);

    /* Tell Core to exit cleanly. */
    make_command(&m, 0);
    m.type = CMD_EXIT;
    write_all(sock, &m, sizeof(m));
    read_all(sock, &r, sizeof(r));

    close(sock);
    sync_close();
    free(lat);
    return ok ? 0 : 1;
}
