# Logger Process

The Logger process is integrated with the Core process using POSIX Message Queues.

## Communication

```text
UI → Core → Logger
          |
          └── /core_to_logger
The Core process sends execution, system, and error messages to the Logger through:

/core_to_logger

The Logger receives these messages using mq_receive() and records them with timestamps.

Integration

The complete system has been tested successfully:

UI sends commands to Core using /ui_to_core
Core sends responses to UI using /core_to_ui
Core sends execution and system logs to Logger using /core_to_logger

Example logged commands:

LOAD 10
ADD 5
PRINT
SHOW CPU

This confirms that the Logger is integrated with the Core and UI-based CPU Simulator.


Save:

```text
Ctrl + O
Enter
Ctrl + X

