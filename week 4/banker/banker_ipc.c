/*
 * banker_ipc.c  -  Week 4 IPC: Named Pipes (FIFO)
 *
 * The Banker's Algorithm (safety check) runs in a separate BANKER process.
 * The GUI process and the Banker process talk through two named pipes:
 *
 *   GUI process --- request FIFO  (System) ---> Banker process
 *   GUI process <-- response FIFO (result) ---- Banker process
 *
 * Start:  banker_server_start()  is called in main() BEFORE the GUI opens.
 * Use  :  is_safe_ipc()          is called when the Banker button is clicked.
 * Stop :  banker_server_stop()   is called after the GUI window is closed.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "../deadlock.h"

/* What the Banker process sends back */
typedef struct {
    int safe;            /* 1 = safe, 0 = unsafe */
    int seq[MAX_P];      /* safe sequence (-1 = unused) */
} BankerReply;

static pid_t banker_pid = -1;      /* process id of the Banker process */
static int   req_fd     = -1;      /* GUI writes the System here */
static int   res_fd     = -1;      /* GUI reads the reply here */
static char  req_path[64];
static char  res_path[64];

/* write() may send less than asked: keep writing until all is sent */
static int write_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;

    while (len > 0) {
        ssize_t w = write(fd, p, len);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p   += w;
        len -= (size_t)w;
    }
    return 0;
}

/* read() may return less than asked: keep reading until all is received.
 * Returns 0 = ok, 1 = pipe closed (nothing more to read), -1 = error. */
static int read_all(int fd, void *buf, size_t len)
{
    char  *p     = buf;
    size_t total = 0;

    while (total < len) {
        ssize_t r = read(fd, p + total, len - total);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0)
            return (total == 0) ? 1 : -1;
        total += (size_t)r;
    }
    return 0;
}

/* ------------------------------------------------------------------
 * BANKER PROCESS: wait for a System, check it, send the answer back.
 * It stops when the GUI closes the request pipe.
 * ------------------------------------------------------------------ */
static void banker_loop(void)
{
    int rfd, wfd;
    System sys;
    BankerReply reply;

    /* Same open order as the GUI side, so both sides meet */
    rfd = open(req_path, O_RDONLY);
    wfd = open(res_path, O_WRONLY);
    if (rfd < 0 || wfd < 0)
        _exit(1);

    while (read_all(rfd, &sys, sizeof(sys)) == 0) {
        reply.safe = is_safe(&sys, reply.seq);

        if (write_all(wfd, &reply, sizeof(reply)) < 0)
            break;
    }

    close(rfd);
    close(wfd);
    _exit(0);
}

/* ------------------------------------------------------------------
 * Start the Banker process. Returns 0 = ok, -1 = failed.
 * If it fails, the program still works (is_safe_ipc uses a local check).
 * ------------------------------------------------------------------ */
int banker_server_start(void)
{
    pid_t pid;

    if (banker_pid > 0)
        return 0;                      /* already running */

    /* If the Banker process dies, do not kill the GUI on write() */
    signal(SIGPIPE, SIG_IGN);

    snprintf(req_path, sizeof(req_path),
             "/tmp/deadlock_banker_req_%d", (int)getpid());
    snprintf(res_path, sizeof(res_path),
             "/tmp/deadlock_banker_res_%d", (int)getpid());

    unlink(req_path);
    unlink(res_path);

    if (mkfifo(req_path, 0600) < 0) {
        perror("mkfifo");
        return -1;
    }
    if (mkfifo(res_path, 0600) < 0) {
        perror("mkfifo");
        unlink(req_path);
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        unlink(req_path);
        unlink(res_path);
        return -1;
    }

    if (pid == 0) {                    /* ---- Banker process ---- */
        banker_loop();
        _exit(0);                      /* not reached */
    }

    /* ---- GUI (parent) process ---- */
    banker_pid = pid;

    /* open() waits until the Banker process opens the other end */
    req_fd = open(req_path, O_WRONLY);
    res_fd = open(res_path, O_RDONLY);
    if (req_fd < 0 || res_fd < 0) {
        perror("open fifo");
        banker_server_stop();
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------
 * Stop the Banker process and remove the named pipes.
 * ------------------------------------------------------------------ */
void banker_server_stop(void)
{
    if (req_fd >= 0) { close(req_fd); req_fd = -1; }   /* Banker sees EOF */
    if (res_fd >= 0) { close(res_fd); res_fd = -1; }

    if (banker_pid > 0) {
        waitpid(banker_pid, NULL, 0);  /* no zombie process */
        banker_pid = -1;
    }

    unlink(req_path);
    unlink(res_path);
}

/* ------------------------------------------------------------------
 * Same as is_safe(), but the work is done by the Banker process.
 * If anything goes wrong with the pipes, it falls back to is_safe().
 * ------------------------------------------------------------------ */
int is_safe_ipc(const System *s, int seq[])
{
    BankerReply reply;
    int k;

    if (banker_pid > 0 && req_fd >= 0 && res_fd >= 0 &&
        write_all(req_fd, s, sizeof(System)) == 0 &&
        read_all(res_fd, &reply, sizeof(reply)) == 0)
    {
        for (k = 0; k < s->n; k++)
            seq[k] = reply.seq[k];
        return reply.safe;
    }

    /* Fallback: the Banker process is not working */
    fprintf(stderr, "IPC: Banker process not available, using local check.\n");
    return is_safe(s, seq);
}
