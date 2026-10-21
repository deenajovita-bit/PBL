Deadlock Simulation System (PBL Week 4)
An interactive simulator written in C with a GTK window. It shows how an Operating System handles resources and deadlock.
What it does
Create processes and resources
Allocation, Maximum, Need and Available matrices
Banker's Algorithm (safe / unsafe state, safe sequence)
Deadlock detection
Resource Allocation Graph
Deadlock recovery (terminate a process, free its resources)
Resource utilization and system status
Who wrote what
File
Owner
main.c, deadlock.h, Makefile, README.md
Jovita deena dsouza
banker.c
rithvik
detection.c, resource.c, recovery.c
nishanth
gui.c, graph.c
rishbah

Setup (GitHub Codespaces)
Install GTK:
 sudo apt updatesudo apt install -y build-essential pkg-config libgtk-3-dev


Add the desktop-lite feature in .devcontainer/devcontainer.json, then rebuild the container.
Open port 6080 in the browser to see the GTK window.
Build and run
make
./deadlock_sim

Clean old files with make clean.
How to use
Setup: enter the number of processes, resource types and total instances. Click Create, or load a sample.
Matrices: fill in Allocation and Maximum. Need and Available are calculated for you.
Graph: see who holds and who waits. Deadlocked processes are red.
Request: pick a process and ask for resources. Tick the box to check with Banker's Algorithm first.
Banker: run the safety check and see the safe sequence.
Detect: find the deadlocked processes.
Recover: end a process to break the deadlock.
Status: see SAFE, UNSAFE or DEADLOCKED, and resource use.
Test cases
Safe: 5 processes, 3 resources, total 10 5 7. Safe sequence is P1, P3, P4, P0, P2.
Deadlock: 3 processes, 3 resources, total 1 1 1, circular wait.
