# Core Process

The Core process is the central CPU simulator and connects the UI and Logger using POSIX Message Queues.

## Communication

```text
                 /ui_to_core
UI ─────────────────────────► Core
                               │
                               ├── /core_to_ui ───────► UI
                               │
                               └── /core_to_logger ───► Logger

