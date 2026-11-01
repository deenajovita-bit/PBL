CC     = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -Icommon
LIBS   = -pthread -lrt

all: core/core UI/ui logger/logger

core/core: core/core.c common/sync.c common/sync.h common/sim_interface.h common/ipc_common.h
	$(CC) $(CFLAGS) -o $@ core/core.c common/sync.c $(LIBS)

UI/ui: UI/ui.c common/sync.c common/sync.h common/sim_interface.h common/ipc_common.h
	$(CC) $(CFLAGS) -o $@ UI/ui.c common/sync.c $(LIBS)

logger/logger: logger/logger.c common/ipc_common.h
	$(CC) $(CFLAGS) -o $@ logger/logger.c $(LIBS)

clean:
	rm -f core/core UI/ui logger/logger
