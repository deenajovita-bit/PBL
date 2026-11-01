# Deadlock Simulation System (PBL Week 4)

An interactive simulator written in C. It shows how an Operating System handles resources and deadlock.

It has two ways to use it:
- **GTK window version** (C + GTK)
- **Website version** (C backend + HTML/JS website)

**IPC used this week: Named Pipes (FIFO)**

## What it does

- Create processes and resources
- Allocation, Maximum, Need and Available matrices
- Banker's Algorithm (safe / unsafe state, safe sequence)
- Deadlock detection
- Resource Allocation Graph (GTK version)
- Deadlock recovery (terminate a process, free its resources)
- Resource utilization and system status

## Who wrote what

| File | Owner |
|---|---|
| main.c, deadlock.h, Makefile, README.md | Jovita deena dsouza |
| banker.c | rithvik |
| detection.c, resource.c, recovery.c | nishanth |
| gui.c, graph.c | rishbah |

## IPC: Named Pipes (FIFO)

In the website version, the Banker's Algorithm runs in a **separate Banker process**. The backend and the Banker process talk through **two named pipes**.

```text
   BACKEND (main process)                     BANKER PROCESS
   ----------------------                     --------------
   starts, makes 2 named pipes
   fork() ------------------------------->   starts, opens the pipes

   website clicks "Run Banker's Algorithm"
   sends the System data   --- request pipe --->   reads the System data
                                                   runs is_safe()
   reads the result        <-- response pipe ---   sends SAFE/UNSAFE + sequence

   Ctrl+C: closes the pipes ---------------->  sees "end of pipe" and stops
   deletes the pipes, waitpid()
```

The two named pipes are made in `/tmp`:
- `deadlock_banker_req_<pid>` : backend to Banker process (the System data)
- `deadlock_banker_res_<pid>` : Banker process to backend (the result and safe sequence)

How it works:
- `banker_server_start()` makes the pipes with `mkfifo()` and starts the Banker process with `fork()`.
- `is_safe_ipc()` sends the System data through the request pipe and reads the answer from the response pipe.
- `banker_server_stop()` closes the pipes, waits for the Banker process (no zombie) and deletes the pipe files.
- `write_all()` and `read_all()` keep sending/reading until all the data is done, because one `write()` or `read()` can do less than asked.
- If the Banker process cannot start, the website still works. It uses the normal local check.
- Ctrl+C is handled, so the Banker process and the pipes are cleaned up.

Which button uses the IPC:
- **Run Banker's Algorithm** uses the IPC (`/api/safety`).
- Other buttons (Send Resource Request, Detect Deadlock, Recovery) run inside the backend as before.

How to show the IPC in a demo:
```bash
ps -ef | grep "[d]eadlock_backend"     # 2 processes: backend + Banker process
ls /tmp | grep deadlock_banker          # 2 named pipes
```
Then click **Run Banker's Algorithm** on the website.

## Files

| File | What it has |
|---|---|
| `main.c` | Starts the GTK version |
| `deadlock.h` | Shared structures and function list |
| `banker/banker.c` | Banker's Algorithm (`is_safe`, need, request) |
| `banker/banker_ipc.c` | Named pipe IPC (Banker process, `is_safe_ipc`) |
| `core/detection.c` | Deadlock detection |
| `core/resource.c` | Resource and system setup |
| `core/recovery.c` | Deadlock recovery |
| `gui/gui.c`, `gui/graph.c` | GTK window and graph |
| `web/backend.c` | C backend server for the website (port 8001) |
| `web/index.html`, `script.js`, `style.css` | The website |

## Website version

### Build and run the backend

```bash
sudo apt-get install -y libcairo2-dev pkg-config     # only the first time
cd "week 4"
gcc -I. $(pkg-config --cflags cairo) -Wall -Wextra -std=c99 \
    web/backend.c banker/banker.c banker/banker_ipc.c \
    core/detection.c core/resource.c core/recovery.c \
    -lm -o web/deadlock_backend
./web/deadlock_backend
```
Keep this terminal open. It shows `Listening on port 8001`.

The old `web/deadlock_backend` file is a build. Always build it again after you change the C code.

### Open the website

1. In the Codespaces **Ports** tab, set port **8001** to **Public** and copy its address.
2. Open `web/script.js`. Line 6 (`API_URL`) must be that address, with no `/` at the end.
3. Open a second terminal and start the website:
   ```bash
   cd "week 4/web"
   python3 -m http.server 8000
   ```
4. In the **Ports** tab, open port **8000** in the browser.

### API endpoints

| Endpoint | What it does |
|---|---|
| `/api/state` | Get the current system |
| `/api/safety` | Run Banker's Algorithm (uses the IPC) |
| `/api/detection` | Detect deadlock |
| `/api/request` | Send a resource request |
| `/api/recovery/terminate` | Terminate a process |
| `/api/reset` | Reset the system |

## GTK window version

Setup (GitHub Codespaces):
```bash
sudo apt update
sudo apt install -y build-essential pkg-config libgtk-3-dev
```
Add the desktop-lite feature in `.devcontainer/devcontainer.json`, then rebuild the container. Open port 6080 in the browser to see the GTK window.

Build and run:
```bash
make
./deadlock_sim
```
Clean old files with `make clean`.

## How to use

1. **Setup:** enter the number of processes and resource types. Click Create, or load a sample.
2. **Matrices:** fill in Allocation and Maximum. Need and Available are calculated for you.
3. **Graph:** see who holds and who waits. Deadlocked processes are red.
4. **Request:** pick a process and ask for resources. Choose Banker's Avoidance to check first.
5. **Banker:** run the safety check and see the safe sequence.
6. **Detect:** find the deadlocked processes.
7. **Recover:** end a process to break the deadlock.
8. **Status:** see SAFE, UNSAFE or DEADLOCKED, and resource use.

## Test cases

- **Safe:** 5 processes, 3 resources, total `10 5 7`. Safe sequence is `P1, P3, P4, P0, P2`.
- **Deadlock:** 3 processes, 3 resources, total `1 1 1`, circular wait.

## Weekly IPC list

| Week | IPC technique |
|---|---|
| 2 | POSIX Message Queues |
| 3 | Pipe + fork() |
| 4 | Named Pipes (FIFO) |
| 5 | Pipes + fork() |
