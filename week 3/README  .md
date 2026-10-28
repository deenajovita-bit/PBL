# CPU Process Scheduling Simulator (PBL Week 3)

A CPU scheduling simulator written in C. You enter the processes, pick an algorithm, and it shows the Gantt chart, the metrics table and the averages.

**IPC used this week: Pipe + fork()**

## What it does

- Takes process details: arrival time, burst time, priority
- Runs these algorithms:
  - FCFS (First Come First Served)
  - SJF (Non-preemptive)
  - SRTF (Preemptive SJF)
  - Priority Scheduling (Non-preemptive)
  - Priority Scheduling (Preemptive)
  - Round Robin (with time quantum)
- Shows the Gantt chart (idle time is shown too)
- Calculates Completion, Turnaround, Waiting and Response time for each process
- Shows average Turnaround, Waiting, Response time and CPU utilization
- Compare All option runs all 6 algorithms and shows one comparison table

## IPC: Pipe + fork()

When you choose one algorithm (options 1 to 6), the work is done in a **child process**. The result comes back to the **parent process** through a **pipe**.

```text
   PARENT                                   CHILD
   ------                                   -----
   1. pipe()  makes the pipe
   2. fork()  makes the child  ------------> starts
   3. closes the write end                  3. closes the read end
   4. waits and reads from pipe             4. runs the algorithm
                                            5. calculates the metrics
                                            6. writes results to pipe
   7. reads results       <---- pipe ------
   8. waitpid() (no zombie)                 7. exits
   9. shows Gantt chart and table
```

What is sent through the pipe:
1. The process table (with completion, waiting, turnaround and response time)
2. The number of Gantt chart segments
3. The Gantt chart segments

Notes:
- `write_all()` and `read_all()` keep writing/reading until all the data is sent, because one `write()` or `read()` can send less than asked.
- The parent checks the data it gets, and it also checks if the child exited properly.
- Compare All (option 7) runs in one process and does not use the pipe.

## Files

| File | What it has |
|---|---|
| `main.c` | Menu, input, calls the IPC function, shows results |
| `ipc_pipe.c` | Pipe + fork() code (`run_algorithm_ipc`) |
| `ipc_pipe.h` | Header for the IPC function |
| `scheduler.h` | Shared structures and function list |
| `scheduler_utils.c` | `run_algorithm`, metrics, results table, compare |
| `fcfs_sjf/fcfs_sjf.c` | FCFS and SJF |
| `srtf_priority/srtf_priority.c` | SRTF and Priority (both types) |
| `round_robin/round_robin.c` | Round Robin |
| `gantt/gantt.c` | Gantt chart display |
| `metrics.c`, `metrics.h` | Small timing/benchmark helper |

## Build and run

Use Linux or GitHub Codespaces. `fork()` and `pipe()` do not work on plain Windows.

```bash
cd "week 3"
gcc -Wall -Wextra -std=c99 -I. main.c scheduler_utils.c ipc_pipe.c \
    fcfs_sjf/fcfs_sjf.c gantt/gantt.c round_robin/round_robin.c \
    srtf_priority/srtf_priority.c -lm -o simulator
./simulator
```

Make sure `ipc_pipe.c` is in the command. If not, you get `undefined reference to run_algorithm_ipc`.

## How to use

1. Enter the number of processes (1 to 100).
2. For each process, enter Arrival Time, Burst Time and Priority (smaller number = higher priority).
3. Choose an option from the menu:
   - `1` FCFS, `2` SJF, `3` SRTF, `4` Priority (NP), `5` Priority (P), `6` Round Robin
   - `7` Compare All
   - `0` Exit
4. For Round Robin and Compare All, enter the time quantum (more than 0).

Input checks: arrival time cannot be negative, burst time must be more than 0, quantum must be more than 0, and wrong menu input is rejected.

## Test case

3 processes:

| Process | Arrival | Burst | Priority |
|---|---|---|---|
| P1 | 0 | 5 | 2 |
| P2 | 1 | 3 | 1 |
| P3 | 2 | 8 | 3 |

Expected results:

| Algorithm | Gantt chart | Avg Turnaround | Avg Waiting | Avg Response |
|---|---|---|---|---|
| FCFS | P1, P2, P3 | 8.67 | 3.33 | 3.33 |
| SRTF | P1, P2, P1, P3 | 8.33 | 3.00 | 2.00 |
| Round Robin (quantum 2) | P1, P2, P3, P1, P2, P3, P1, P3 | 11.33 | 6.00 | 1.00 |

FCFS Gantt chart:
```text
| P1 | P2 | P3 |
0    5    8    16
```

## Rules used by all algorithms

- Tie-break: earlier arrival first, then lower PID.
- Idle CPU time is shown in the Gantt chart as an idle block.
- Turnaround = Completion - Arrival
- Waiting = Turnaround - Burst
- Response = First CPU start - Arrival

## Weekly IPC list

| Week | IPC technique |
|---|---|
| 2 | POSIX Message Queues |
| 3 | Pipe + fork() |
| 4 | (to be added) |
| 5 | Pipes + fork() |
