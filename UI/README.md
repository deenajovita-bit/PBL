# UI Process

The UI process is integrated with the Core and Logger processes using POSIX Message Queues.

## Communication

```text
              /ui_to_core
UI ─────────────────────────► Core
 ▲                             │
 │                             │ /core_to_logger
 │ /core_to_ui                 ▼
 └────────────────────────── Logger
