#ifndef DEADLOCK_H
#define DEADLOCK_H

#include <cairo.h>

#define MAX_P 10   /* max processes */
#define MAX_R 10   /* max resource types */

typedef enum {
    STATE_SAFE,
    STATE_UNSAFE,
    STATE_DEADLOCK
} SysState;

typedef enum {
    REQ_GRANTED,
    REQ_WAIT,
    REQ_DENIED_UNSAFE,
    REQ_ERROR
} ReqResult;

typedef struct {
    int n, m;                        /* number of processes, resources */
    int total[MAX_R];                /* total instances */
    int available[MAX_R];            /* currently available */
    int alloc[MAX_P][MAX_R];         /* allocation matrix */
    int max[MAX_P][MAX_R];           /* maximum matrix */
    int need[MAX_P][MAX_R];          /* need = max - allocation */
    int request[MAX_P][MAX_R];       /* pending requests */
    int active[MAX_P];               /* 1 = active, 0 = terminated */
} System;


/* ==========================================
   resource.c
   ========================================== */

void sys_init(
    System *s,
    int n,
    int m,
    const int total[]
);

void update_available(System *s);

ReqResult request_resources(
    System *s,
    int p,
    const int req[],
    int use_banker
);

void release_resources(
    System *s,
    int p
);


/* ==========================================
   banker.c
   ========================================== */

void calc_need(System *s);

int is_safe(
    const System *s,
    int seq[]
);

int banker_check(
    System *s,
    int p,
    const int req[]
);


/* ==========================================
   detection.c
   ========================================== */

int detect_deadlock(
    const System *s,
    int dead[]
);


/* ==========================================
   recovery.c
   ========================================== */

int pick_victim(
    const System *s,
    const int dead[],
    int count
);

void recover_terminate(
    System *s,
    int p
);

int recover_preempt(
    System *s,
    int p
);


/* ==========================================
   main.c
   ========================================== */

SysState system_state(System *s);


/* ==========================================
   graph.c / gui.c
   ========================================== */

void graph_draw(
    cairo_t *cr,
    int w,
    int h,
    const System *s,
    const int dead[],
    int dead_count
);

int gui_run(
    System *s,
    int argc,
    char **argv
);

#endif