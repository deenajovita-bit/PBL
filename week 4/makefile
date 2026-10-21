# Put this Makefile inside the "week 4" folder, next to deadlock.h and main.c

CC      = gcc
CFLAGS  = -I. -Wall -Wextra -std=c99 $(shell pkg-config --cflags gtk+-3.0)
LIBS    = $(shell pkg-config --libs gtk+-3.0)

SRC = main.c \
      banker/banker.c \
      core/detection.c core/resource.c core/recovery.c \
      gui/gui.c gui/graph.c
OBJ = $(SRC:.c=.o)
BIN = deadlock_sim

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LIBS)

%.o: %.c deadlock.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJ) $(BIN) main_test.o test_integration

# Test without the GUI (needs the tests/ folder)
TESTFLAGS = -I. -Wall -Wextra -std=c99 $(shell pkg-config --cflags cairo)
test:
	$(CC) $(TESTFLAGS) -Dmain=app_main -c main.c -o main_test.o
	$(CC) $(TESTFLAGS) main_test.o banker/banker.c core/detection.c core/resource.c core/recovery.c tests/stub_gui.c tests/test_integration.c -o test_integration
	./test_integration

.PHONY: all run clean test
