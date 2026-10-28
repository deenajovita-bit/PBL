# Page Replacement Simulator (PBL Week 5)

Simulates FIFO, LRU and Optimal page replacement. Shows frame contents after each
request, hits, faults, ratios and a comparison.

## Team
- Risbah: FIFO + LRU (`c-code/fifo_lru.c`)
- Rithvik: Optimal + comparison (`c-code/optimal.c`, `c-code/compare.c`)
- Nisthanth: Website (`website/`)
- Leader: integration, input check, IPC, testing, report

## Folders
- `c-code/`  C source files
- `website/` HTML, CSS, JavaScript
- `report/`  report and PPT

## Run the C program (Linux or WSL)
```
cd c-code
make

./sim        # IPC version: 3 child processes, results sent through pipes (Linux/WSL)
```

## Run the website
1. Install Emscripten, then in `c-code` run: `make wasm`
   (this creates `website/sim.js` and `website/sim.wasm`)
2. In `website`, run: `python -m http.server`
3. Open http://localhost:8000

## Test
Reference string `7 0 1 2 0 3 0 4 2 3`, 3 frames:
FIFO 9 faults, LRU 8 faults, Optimal 6 faults.

## IPC
`sim` uses pipes with `fork()`. Each algorithm runs in its own child process
and sends its `Result` struct to the parent through a pipe.
