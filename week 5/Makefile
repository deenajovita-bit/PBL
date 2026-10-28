CC = gcc
CFLAGS = -Wall -Wextra -I.
SRC = main.c input.c compare.c fifo_lru/fifo_lru.c optimal/optimal.c

sim: $(SRC) pagerep.h
	$(CC) $(CFLAGS) $(SRC) -o sim

wasm: wasm_api.c fifo_lru/fifo_lru.c optimal/optimal.c pagerep.h
	emcc -I. wasm_api.c fifo_lru/fifo_lru.c optimal/optimal.c -O2 -o web/sim.js -s EXPORTED_FUNCTIONS=_simulate,_malloc,_free -s EXPORTED_RUNTIME_METHODS=HEAP32

clean:
	rm -f sim web/sim.js web/sim.wasm
