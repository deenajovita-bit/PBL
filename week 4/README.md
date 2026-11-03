# Page Replacement Simulator (PBL Week 5)

Simulates FIFO, LRU and Optimal page replacement. Shows the frame contents after each request, hits, faults, ratios, and a comparison of the three algorithms.

## Team
- Risbah: FIFO + LRU (`fifo_lru/fifo_lru.c`)
- Rithvik: Optimal + comparison (`optimal/optimal.c`, `compare.c`)
- Nisthanth: Website (`web/`)
- Jovita (Leader): integration, input check, IPC (pipes + `fork()`), testing, report

## Files
```text
week 5/
├── main.c            Parent process: input, fork(), pipes, output
├── input.c           Checks the page-reference string
├── compare.c         Hit/fault ratios and best algorithm
├── pagerep.h         Result and Step structures
├── fifo_lru/         FIFO and LRU
├── optimal/          Optimal
├── wasm_api.c        Entry point for the website (WebAssembly)
├── web/              Website: index.html, style.css, script.js, sim.js, sim.wasm
└── Makefile
```

## Run the C program (Linux or WSL)
```bash
cd "week 5"
make
./sim
```
The program asks for the input, runs all three algorithms, and prints the results:

1. `Enter the page-reference string:` type numbers with spaces, for example `7 0 1 2 0 3 0 4 2 3`
2. `Enter the number of page frames:` type a number from 1 to 10

Wrong input (letters, an empty line, 0 frames, more than 10 frames) shows an error message and stops.

## Run the website
1. Install Emscripten, then in `week 5` run: `make wasm` (this creates `web/sim.js` and `web/sim.wasm`)
2. In `week 5/web` run: `python3 -m http.server`
3. Open http://localhost:8000

The website runs in the browser, so it calls the algorithms directly. The pipe version is the C program (`./sim`).

## IPC: pipes with fork()

```text
                    ┌── fork() ──► Child 1: FIFO     ──┐
                    │                                   │ pipe 1
 Parent (main.c) ───┼── fork() ──► Child 2: LRU      ──┤ pipe 2   (each child writes its
   reads input      │                                   │ pipe 3    Result struct, parent reads)
   prints results   └── fork() ──► Child 3: Optimal  ──┘
```

1. The parent reads the reference string and the number of frames.
2. For each algorithm it creates a pipe and calls `fork()`.
3. Each child closes the read end, runs its algorithm, writes the whole `Result` struct into its pipe, and exits.
4. The parent closes the write ends, reads the three `Result` structs (`read_all` loops until the whole struct arrives), and calls `waitpid()` for each child.
5. The parent prints the step tables and the comparison.

Why pipes: the parent and children are related processes, the data flows one way (child to parent), and the three algorithms run at the same time without sharing memory.

## Test
Reference string `7 0 1 2 0 3 0 4 2 3`, 3 frames:

| Algorithm | Hits | Faults | Fault ratio |
|---|---|---|---|
| FIFO | 1 | 9 | 90.0% |
| LRU | 2 | 8 | 80.0% |
| Optimal | 4 | 6 | 60.0% |

Fewest faults: Optimal. Bad input (letters, empty line, 0 frames, more than 10 frames) shows an error message and stops.
