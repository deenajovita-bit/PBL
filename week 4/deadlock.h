#ifndef DEADLOCK_H
#define DEADLOCK_H

#include <cairo.h>

#define MAX_P 10   /* max processes */
#define MAX_R 10   /* max resource types */

typedef enum { STATE_SAFE, STATE_UNSAFE, STATE_DEADLOCK } SysState;
typedef enum { REQ_GRANTED, REQ_WAIT, REQ_DENIED_UNSAFE, REQ_ERROR } ReqResult;

typedef struct {
    int n, m;                        /* number of processes, resources */
    int total[MAX_R];                /* total instances of each resource */
    int available[MAX_R];            /* free instances now */
    int alloc[MAX_P][MAX_R];         /* Allocation matrix */
    int max[MAX_P][MAX_R];           /* Maximum matrix */
    int need[MAX_P][MAX_R];          /* Need = max - alloc */
    int request[MAX_P][MAX_R];       /* waiting (blocked) requests */
    int active[MAX_P];               /* 1 = running, 0 = terminated */
} System;

/* ---- resource.c (Member 2) ---- */
void      sys_init(System *s, int n, int m, const int total[]);
void      update_available(System *s);
ReqResult request_resources(System *s, int p, const int req[], int use_banker);
void      release_resources(System *s, int p);   /* frees all of p's resources */

/* ---- banker.c (Member 1) ---- */
void calc_need(System *s);
int  is_safe(const System *s, int seq[]);        /* 1 = safe, fills seq[] with n entries */
int  banker_check(System *s, int p, const int req[]); /* 1 = granting keeps it safe */

/* ---- detection.c (Member 2) ---- */
int  detect_deadlock(const System *s, int dead[]);    /* returns count, fills dead[] */

/* ---- recovery.c (Member 2) ---- */
int  pick_victim(const System *s, const int dead[], int count);
void recover_terminate(System *s, int p);        /* kill p, free resources, retry waiting requests */

/* ---- main.c (Leader) ---- */
SysState system_state(System *s);                /* DEADLOCK > UNSAFE > SAFE */

/* ---- graph.c / gui.c (Member 3) ---- */
void graph_draw(cairo_t *cr, int w, int h, const System *s,
                const int dead[], int dead_count);
int  gui_run(System *s, int argc, char **argv);

#endif
