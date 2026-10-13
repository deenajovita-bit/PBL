#ifndef METRICS_H
#define METRICS_H

#include <stddef.h>

typedef struct {
    double *samples;      /* latencies in microseconds */
    size_t  count;
    size_t  capacity;
    double  start_us;     /* start of the whole run, for throughput */
    double  end_us;
} Metrics;

/* All functions returning int give 0 on success, -1 on failure. */

/* Current monotonic time in microseconds, written to *out. */
int  now_us(double *out);

/* Allocate room for `capacity` samples. Fails on NULL m, capacity 0,
 * or allocation failure. */
int  metrics_init(Metrics *m, size_t capacity);
void metrics_free(Metrics *m);

/* Mark the start / end of the whole run (used for throughput). */
int  metrics_start(Metrics *m);
int  metrics_stop(Metrics *m);

/* Record one latency in microseconds. Rejects negative, NaN, infinite
 * values, and fails when the buffer is full. */
int  metrics_record(Metrics *m, double latency_us);

/* Print a summary to stdout. */
void metrics_report(const Metrics *m, const char *label);

/* Append one CSV row (a header is written if the file is new/empty).
 * Labels with commas, quotes or newlines are quoted and escaped. */
int  metrics_save_csv(const Metrics *m, const char *label, const char *path);

#endif /* METRICS_H */
