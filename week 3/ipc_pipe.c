/*
 * ipc_pipe.c - Week 3 IPC: Pipe + fork()
 *
 * Parent  : makes a pipe, forks a child, reads the results.
 * Child   : runs the scheduling algorithm, writes the results to the pipe.
 *
 *   Parent  <------ pipe ------  Child (runs the algorithm)
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "scheduler.h"
#include "ipc_pipe.h"

/* write() can send less than asked, so keep writing until all is sent */
static int write_all(int fd, const void *buf, size_t len)
{
    const char *ptr = buf;

    while (len > 0) {
        ssize_t w = write(fd, ptr, len);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        ptr += w;
        len -= (size_t)w;
    }
    return 0;
}

/* read() can also return less, so keep reading until all is received */
static int read_all(int fd, void *buf, size_t len)
{
    char *ptr = buf;

    while (len > 0) {
        ssize_t r = read(fd, ptr, len);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;          /* pipe closed too early */
        ptr += r;
        len -= (size_t)r;
    }
    return 0;
}

int run_algorithm_ipc(int choice, Process p[], int n,
                      int quantum, GanttChart *g)
{
    int fd[2];          /* fd[0] = read end, fd[1] = write end */
    pid_t pid;
    int status;

    if (pipe(fd) == -1) {
        perror("pipe");
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }

    /* ---------------- CHILD ---------------- */
    if (pid == 0) {
        close(fd[0]);                       /* child only writes */

        g->count = 0;
        run_algorithm(choice, p, n, quantum, g);
        calculate_metrics(p, n, g);

        /* send: process table, segment count, then only the used segments */
        if (write_all(fd[1], p, sizeof(Process) * (size_t)n) < 0 ||
            write_all(fd[1], &g->count, sizeof(int)) < 0 ||
            write_all(fd[1], g->segments,
                      sizeof(GanttSegment) * (size_t)g->count) < 0)
        {
            close(fd[1]);
            _exit(1);
        }

        close(fd[1]);
        _exit(0);
    }

    /* ---------------- PARENT ---------------- */
    close(fd[1]);                           /* parent only reads */

    if (read_all(fd[0], p, sizeof(Process) * (size_t)n) < 0 ||
        read_all(fd[0], &g->count, sizeof(int)) < 0 ||
        g->count < 0 || g->count > MAX_SEGMENTS ||
        read_all(fd[0], g->segments,
                 sizeof(GanttSegment) * (size_t)g->count) < 0)
    {
        fprintf(stderr, "IPC: failed to read results from child.\n");
        close(fd[0]);
        waitpid(pid, &status, 0);
        return -1;
    }

    close(fd[0]);
    waitpid(pid, &status, 0);               /* no zombie process */

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "IPC: child process failed.\n");
        return -1;
    }
    return 0;
}
