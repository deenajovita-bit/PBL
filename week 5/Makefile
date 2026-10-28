CC = gcc
CFLAGS = -Wall -Wextra
ALGO = fifo_lru.c optimal.c compare.c input.c

all: sim

sim: main.c $(ALGO) pagerep.h
	$(CC) $(CFLAGS) main.c $(ALGO) -o sim

wasm: wasm_api.c fifo_lru.c optimal.c pagerep.h
	emcc wasm_api.c fifo_lru.c optimal.c -O2 -o ../website/sim.js \
	  -s EXPORTED_FUNCTIONS=_simulate,_malloc,_free \
	  -s EXPORTED_RUNTIME_METHODS=HEAP32

clean:
	rm -f sim ../website/sim.js ../website/sim.wasm
