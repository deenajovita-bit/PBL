# Week 2 - Multi-Process CPU Simulator with IPC

The CPU simulator is split into **three independent processes**. Each process uses a different IPC technique, and semaphores keep them in order.

## Team and IPC

| Part | Owner | File | IPC technique |
|---|---|---|---|
| UI process | Rishabh | `UI/ui.c` | Sockets (UNIX stream socket) |
| Core process | Nishanth | `core/core.c` | Shared memory |
| Logging process | Rithvik | `logger/logger.c` | Pipes (`pipe()` + `fork()`) |
| Integration and testing | Jovita (Team Leader) | `sync.c`, `sync.h`, `sim_interface.h`, `run_tests.sh`, `benchmark/` | Semaphores |

## Architecture diagram

```text
                 socket  /tmp/sim_core.sock
                 (commands and replies, Message structs)
      UI  ◄════════════════════════════════════════════►  Core
   (Rishabh)                                            (Nishanth)
      ▲                                                  │      │
      │        shared memory  /sim_state                 │      │ fork() + pipe
      │        (ACC, PC, memory, stack, queue)           │      ▼
      └──────── read ◄──────────────────────── write ────┘   Logger (Rithvik)
                                                                │
        semaphores (Jovita)                                     ▼
        /sim_state_lock   one process at a time            simulator.log
        /sim_core_ready   Core tells UI "I am ready"
```

Demo flow: **UI → Core → CPU / Memory / Stack / Queue → Logger**

## Selected IPC and justification

| Link | IPC | Why this one |
|---|---|---|
| UI ↔ Core | **Socket** | Two-way: commands go to Core and replies come back on the same connection. Core can tell when the UI disconnects. A UNIX socket works between unrelated programs on one machine, and the same design can later run over a network. |
| Core state → UI | **Shared memory** | The UI only needs to read the CPU, memory and stack state. Shared memory shows it without sending a message for every read. It is the fastest IPC, because there is no copy through the kernel. |
| Core → Logger | **Pipe** | Logging is one-way: Core writes, Logger reads, in order. A pipe is simple, and Core does not wait for the log file to be written. Logger is Core's child (`fork()`), so a pipe fits. When Core closes the pipe, Logger gets end-of-file and stops. |
| Keeping order | **Semaphores** | Shared memory has no protection of its own. The lock semaphore makes sure the UI never reads half-written data. The ready semaphore makes the UI wait until Core has finished setting up. |

The first Week 2 version used POSIX message queues for all links. The design was changed so each member uses a different technique, as sir asked.

## How a command flows

1. Core starts: creates the semaphores, the shared memory and the socket, starts the Logger through a pipe, then posts `core_ready`.
2. UI starts: waits for `core_ready`, maps the shared memory, connects to the socket.
3. You type a command, for example `ADD 5`. UI sends it through the socket.
4. Core runs it, locks the semaphore, copies its state into shared memory, unlocks.
5. Core sends the reply through the socket and one log line through the pipe.
6. The UI command `STATE` reads the shared memory directly (also inside the lock).
7. On `EXIT`, Core closes the pipe, waits for the Logger, and removes the socket, shared memory and semaphores.

## Build and run

```bash
cd week2
make
cd core && ./core       # terminal 1 (starts the Logger by itself)
cd UI   && ./ui         # terminal 2
```

Start Core first. Do not run the Logger by hand. Commands: `LOAD`, `ADD`, `SUB`, `PUSH`, `POP`, `STORE`, `LOADM`, `PRINT`, `SHOW CPU`, `SHOW MEMORY`, `SHOW STACK`, `SHOW QUEUE`, `STATE`, `EXIT`.

## IPC test cases

Run `bash run_tests.sh` after `make`. Result: **11 passed, 0 failed**.

| # | Test | Expected |
|---|---|---|
| 1 | UI started before Core | Clear message, no hang |
| 2 | Valid commands (LOAD, ADD, PUSH, STORE, EXIT) | All succeed |
| 3 | Wrong command | Error reply, Core keeps running |
| 4 | `STATE` after commands | Right values from shared memory |
| 5 | 1000 quick commands | No lost or broken data |
| 6 | Log file | One log line per executed command |
| 7 | Core closes the pipe | Logger stops by itself |
| 8 | After `EXIT` | No leftover socket, shared memory or semaphores |
| 9 | After `EXIT` | Logger process is gone |
| 10 | UI killed suddenly | Core notices and stops |
| 11 | UI killed suddenly | Nothing left behind |

## Benchmark: standalone vs multi-process

Run `cd benchmark && bash run_bench.sh`. Same workload for both versions: 100,000 commands, average of 3 runs. Full numbers are in `benchmark/results.md` and the discussion is in `benchmark/analysis.md`.

| Metric | Standalone | Multi-process |
|---|---|---|
| Average time per command | 0.95 µs | 38.71 µs |
| Total time, 100,000 commands | 100 ms | 3,882 ms |
| Total CPU time | 111 ms | 4,248 ms |
| Peak memory (all processes) | 2.9 MB | 7.0 MB |

IPC overhead is about 38 µs per command (about 41 times slower), which a user cannot notice. In return, the three parts are separate, and Core does not wait for logging.

## Files

```text
week2/
├── Makefile
├── ipc_common.h        Message structure and constants
├── sim_interface.h     Socket path, shared memory layout, semaphore names
├── sync.h / sync.c     Semaphore functions (lock, unlock, ready)
├── run_tests.sh        11 automatic IPC tests
├── UI/ui.c
├── core/core.c
├── logger/logger.c
└── benchmark/          bench_standalone.c, bench_multi.c, run_bench.sh, results.md, analysis.md
```
