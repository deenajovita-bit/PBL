/* bench_common.h - shared helpers for the two benchmark programs.
 * Both programs run the SAME workload so the results can be compared:
 * a repeating cycle of 5 commands: LOAD 5, ADD 1, PUSH 6, POP, STORE addr. */
#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

#define CYCLE 5

static void make_command(Message *m, long i)
{
    memset(m, 0, sizeof(Message));

    switch (i % CYCLE)
    {
        case 0: m->type = CMD_LOAD;  m->value = 5;               break;
        case 1: m->type = CMD_ADD;   m->value = 1;               break;
        case 2: m->type = CMD_PUSH;  m->value = 6;               break;
        case 3: m->type = CMD_POP;                               break;
        case 4: m->type = CMD_STORE; m->address = (int)((i / CYCLE) % MEMORY_SIZE); break;
    }
}

static double now_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* Fill avg / p50 / p95 / p99 (microseconds) from the latency array. */
static void latency_stats(double *lat, long n, double *avg, double *p50, double *p95, double *p99)
{
    double sum = 0;
    for (long i = 0; i < n; i++) sum += lat[i];
    *avg = sum / n;

    qsort(lat, (size_t)n, sizeof(double), cmp_double);
    *p50 = lat[(long)(n * 0.50)];
    *p95 = lat[(long)(n * 0.95)];
    *p99 = lat[(long)(n * 0.99)];
}

/* CPU time of the current process in milliseconds (user + system). */
static double self_cpu_ms(void)
{
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_utime.tv_sec * 1e3 + r.ru_utime.tv_usec / 1e3
         + r.ru_stime.tv_sec * 1e3 + r.ru_stime.tv_usec / 1e3;
}

/* Peak memory (resident set) of the current process in KB. */
static long self_peak_kb(void)
{
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_maxrss;                 /* Linux: already in KB */
}

/* CPU time (ms) of ANOTHER process, from /proc/<pid>/stat. */
__attribute__((unused)) static double proc_cpu_ms(int pid)
{
    char path[64], buf[1024];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';

    char *p = strrchr(buf, ')');            /* skip "pid (name)" */
    if (!p) return -1;

    unsigned long utime = 0, stime = 0;
    /* after ')' : state ppid pgrp session tty tpgid flags minflt cminflt majflt cmajflt utime stime */
    if (sscanf(p + 2, "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", &utime, &stime) != 2)
        return -1;

    return (utime + stime) * 1000.0 / sysconf(_SC_CLK_TCK);
}

/* Peak memory (VmHWM, KB) of ANOTHER process, from /proc/<pid>/status. */
__attribute__((unused)) static long proc_peak_kb(int pid)
{
    char path[64], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    long kb = -1;
    while (fgets(line, sizeof(line), f))
        if (sscanf(line, "VmHWM: %ld kB", &kb) == 1) break;

    fclose(f);
    return kb;
}

#endif
