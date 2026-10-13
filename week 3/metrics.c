/*
 * metrics.c - small benchmarking helper for the simulator
 *
 * Collects latency samples (in microseconds), then reports
 * count, min, max, mean, std dev, p50/p95/p99 and throughput.
 *
 * Build the demo:   gcc -O2 -Wall -DMETRICS_DEMO metrics.c -o metrics -lm
 * Use in project:   gcc -O2 -Wall -c metrics.c   (then link metrics.o -lm)
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "metrics.h"

/* ---------- time helpers ---------- */

int now_us(double *out)
{
    struct timespec ts;
    if (!out) return -1;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return -1;
    *out = ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
    return 0;
}

/* ---------- lifecycle ---------- */

int metrics_init(Metrics *m, size_t capacity)
{
    if (!m) return -1;
    m->samples = NULL;
    m->count = m->capacity = 0;
    m->start_us = m->end_us = 0;

    if (capacity == 0) return -1;
    if (capacity > SIZE_MAX / sizeof(double)) return -1;   /* overflow */

    m->samples = malloc(capacity * sizeof(double));
    if (!m->samples) return -1;
    m->capacity = capacity;
    return 0;
}

void metrics_free(Metrics *m)
{
    if (!m) return;
    free(m->samples);
    m->samples = NULL;
    m->count = m->capacity = 0;
}

int metrics_start(Metrics *m)
{
    if (!m) return -1;
    return now_us(&m->start_us);
}

int metrics_stop(Metrics *m)
{
    if (!m) return -1;
    return now_us(&m->end_us);
}

int metrics_record(Metrics *m, double latency_us)
{
    if (!m || !m->samples) return -1;
    if (isnan(latency_us) || isinf(latency_us) || latency_us < 0) return -1;
    if (m->count >= m->capacity) return -1;   /* full */
    m->samples[m->count++] = latency_us;
    return 0;
}

/* ---------- stats ---------- */

typedef struct {
    double min, max, mean, stddev, p50, p95, p99;
    double elapsed_us, throughput;
} Stats;

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* p is 0..100, array must be sorted */
static double percentile(const double *sorted, size_t n, double p)
{
    if (n == 0) return 0;
    double rank = (p / 100.0) * (n - 1);
    size_t lo = (size_t)floor(rank);
    size_t hi = (size_t)ceil(rank);
    double frac = rank - lo;
    return sorted[lo] + (sorted[hi] - sorted[lo]) * frac;
}

/* Fills *st from the samples. Works on a sorted copy. */
static int compute_stats(const Metrics *m, Stats *st)
{
    if (!m || !m->samples || m->count == 0) return -1;

    double *s = malloc(m->count * sizeof(double));
    if (!s) return -1;
    memcpy(s, m->samples, m->count * sizeof(double));
    qsort(s, m->count, sizeof(double), cmp_double);

    double sum = 0;
    for (size_t i = 0; i < m->count; i++) sum += s[i];
    double mean = sum / m->count;

    double var = 0;
    for (size_t i = 0; i < m->count; i++) var += (s[i] - mean) * (s[i] - mean);

    st->min    = s[0];
    st->max    = s[m->count - 1];
    st->mean   = mean;
    st->stddev = sqrt(var / m->count);
    st->p50    = percentile(s, m->count, 50);
    st->p95    = percentile(s, m->count, 95);
    st->p99    = percentile(s, m->count, 99);

    st->elapsed_us = m->end_us - m->start_us;
    st->throughput = st->elapsed_us > 0
                   ? m->count / (st->elapsed_us / 1e6) : 0;

    free(s);
    return 0;
}

void metrics_report(const Metrics *m, const char *label)
{
    if (!label) label = "(unnamed)";

    Stats st;
    if (compute_stats(m, &st) != 0) {
        printf("[%s] no samples recorded\n", label);
        return;
    }

    printf("=== %s ===\n", label);
    printf("samples      : %zu\n", m->count);
    printf("min          : %.2f us\n", st.min);
    printf("max          : %.2f us\n", st.max);
    printf("mean         : %.2f us\n", st.mean);
    printf("std dev      : %.2f us\n", st.stddev);
    printf("p50 (median) : %.2f us\n", st.p50);
    printf("p95          : %.2f us\n", st.p95);
    printf("p99          : %.2f us\n", st.p99);
    if (st.elapsed_us > 0) {
        printf("total time   : %.2f ms\n", st.elapsed_us / 1e3);
        printf("throughput   : %.0f ops/sec\n", st.throughput);
    }
    printf("\n");
}

/* ---------- CSV ---------- */

/* Write a CSV field, quoting it if needed and doubling any quotes. */
static void write_csv_field(FILE *f, const char *s)
{
    int needs_quotes = 0;
    for (const char *p = s; *p; p++) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') {
            needs_quotes = 1;
            break;
        }
    }
    if (!needs_quotes) {
        fputs(s, f);
        return;
    }
    fputc('"', f);
    for (const char *p = s; *p; p++) {
        if (*p == '"') fputc('"', f);   /* "" inside a quoted field */
        fputc(*p, f);
    }
    fputc('"', f);
}

int metrics_save_csv(const Metrics *m, const char *label, const char *path)
{
    if (!path) return -1;
    if (!label) label = "";

    Stats st;
    if (compute_stats(m, &st) != 0) return -1;

    FILE *f = fopen(path, "a");
    if (!f) return -1;

    /* write a header only if the file is empty */
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0)
        fputs("label,count,min_us,max_us,mean_us,p50_us,p95_us,p99_us,"
              "throughput_ops_per_sec\n", f);

    write_csv_field(f, label);
    fprintf(f, ",%zu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.0f\n",
            m->count, st.min, st.max, st.mean,
            st.p50, st.p95, st.p99, st.throughput);

    return fclose(f) == 0 ? 0 : -1;
}

/* ---------- demo (only built with -DMETRICS_DEMO) ----------
 * NOTE: this times a placeholder math loop, NOT your scheduler.
 * It only shows how the API is used. To compare FCFS / SJF /
 * Priority / Round Robin, replace the marked block with a call to
 * the real scheduling function and give each algorithm its own
 * Metrics + label.
 */
#ifdef METRICS_DEMO
int main(void)
{
    Metrics m;
    const size_t N = 10000;

    if (metrics_init(&m, N) != 0) {
        fprintf(stderr, "metrics_init failed\n");
        return 1;
    }

    if (metrics_start(&m) != 0) { fprintf(stderr, "clock error\n"); return 1; }
    for (size_t i = 0; i < N; i++) {
        double t0, t1;
        if (now_us(&t0) != 0) { fprintf(stderr, "clock error\n"); return 1; }

        /* ---- replace with the operation you want to measure ---- */
        volatile double x = 0;
        for (int k = 0; k < 500; k++) x += sqrt((double)k);
        /* -------------------------------------------------------- */

        if (now_us(&t1) != 0) { fprintf(stderr, "clock error\n"); return 1; }
        if (metrics_record(&m, t1 - t0) != 0)
            fprintf(stderr, "bad sample skipped\n");
    }
    metrics_stop(&m);

    metrics_report(&m, "placeholder workload");
    metrics_save_csv(&m, "placeholder, \"demo\"", "results.csv");
    metrics_free(&m);
    return 0;
}
#endif
