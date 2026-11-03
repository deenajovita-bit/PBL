# Benchmark results

Workload: 100000 commands per run (LOAD, ADD, PUSH, POP, STORE repeated), average of 3 runs.

| Metric | Standalone (1 process) | Multi-process (UI + Core + Logger) |
|---|---|---|
| Total time for 100000 commands (ms) | 100.27 | 3882.39 |
| Average time per command (us) | 0.95 | 38.71 |
| Median (p50) per command (us) | 0.83 | 25.72 |
| p95 per command (us) | 1.22 | 85.30 |
| p99 per command (us) | 1.40 | 121.40 |
| Total CPU time (ms) | 110.61 | 4247.83 |
| Peak memory, all processes (KB) | 2944.00 | 6978.67 |

**IPC overhead per command:** 37.76 us (38.71 - 0.95). The multi-process version is about 40.7x slower per command.

## Multi-process breakdown (average)

| Process | IPC | CPU time (ms) | Peak memory (KB) |
|---|---|---|---|
| UI (benchmark client) | Socket | 1334.49 | 3328.00 |
| Core | Shared memory + semaphores | 1530.00 | 1784.00 |
| Logger | Pipe | 1383.33 | 1866.67 |

Correct results in every run: standalone=3/3, multi=3/3. Errors: 0.00.
