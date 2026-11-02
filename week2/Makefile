CC     = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -I.
LIBS   = -pthread -lrt

all: core/core UI/ui logger/logger

core/core: core/core.c sync.c sync.h sim_interface.h ipc_common.h
	$(CC) $(CFLAGS) -o $@ core/core.c sync.c $(LIBS)

UI/ui: UI/ui.c sync.c sync.h sim_interface.h ipc_common.h
	$(CC) $(CFLAGS) -o $@ UI/ui.c sync.c $(LIBS)

logger/logger: logger/logger.c ipc_common.h
	$(CC) $(CFLAGS) -o $@ logger/logger.c $(LIBS)

clean:
	rm -f core/core UI/ui logger/logger
