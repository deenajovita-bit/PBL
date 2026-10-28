#ifndef IPC_PIPE_H
#define IPC_PIPE_H

#include "scheduler.h"

/* Runs one algorithm in a CHILD process.
 * The child sends the results back to the parent through a PIPE.
 * Returns 0 on success, -1 on failure. */
int run_algorithm_ipc(int choice, Process p[], int n,
                      int quantum, GanttChart *g);

#endif
