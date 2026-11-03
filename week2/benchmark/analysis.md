# Performance analysis (Week 2)

Numbers below are from one test machine. Run `bash run_bench.sh` on your own machine (Codespaces) and put your own numbers in this file. The pattern should be the same.

## What was compared
- **Standalone:** the real simulator code, called directly inside one process. No socket, no shared memory, no semaphores, no Logger process.
- **Multi-process:** UI, Core and Logger as 3 processes. UI to Core uses a socket. Core copies its state into shared memory guarded by semaphores. Core sends each log line to the Logger through a pipe.
- Same workload for both: 100,000 commands (LOAD, ADD, PUSH, POP, STORE repeated), average of 3 runs.

## Results
| Metric | Standalone | Multi-process |
|---|---|---|
| Time per command (average) | 0.22 us | 7.77 us |
| Total time, 100,000 commands | 27 ms | 784 ms |
| Total CPU time | 37 ms | 779 ms |
| Peak memory (all processes) | about 3.1 MB | about 7.0 MB |

IPC overhead: about 7.5 us per command, so the multi-process version is about 35 times slower per command.

## Why it is slower
Every command in the multi-process version pays for:
1. **Socket round trip:** the command goes UI to Core and the reply comes back. This needs system calls and a switch between processes (two context switches).
2. **Shared memory update:** Core locks the semaphore, copies the CPU, memory and stack state (about 1.5 KB), and unlocks. This happens before each reply.
3. **Pipe write to the Logger:** one extra system call per log line, and the Logger process does work for each line.

In the standalone version a command is just a function call, so it takes less than a microsecond.

## Why it is still worth it
- The three parts are separate programs. One part can be changed or restarted without touching the others.
- If the Logger is slow, Core only writes to the pipe and does not wait for the log file.
- Each part has a clear interface and owner, which is how larger systems are built.
- 7.5 us per command is far below what a person can type, so the user sees no delay.
- Memory is about 2 times higher because each process has its own stack and libraries.

## Which IPC costs what
- The socket costs the most, because it needs a system call and a process switch on both sides for every command.
- Shared memory and the semaphore are cheap when only one process uses them at a time. They are fast because there is no copy through the kernel, only a lock and a memory copy.
- The pipe is a one-way write. Core does not wait for the Logger.
