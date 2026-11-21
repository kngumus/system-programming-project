
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude

all: watchdog worker

watchdog: src/main.c src/signal_manager.c src/process_manager.c include/signal_manager.h include/process_manager.h
	$(CC) $(CFLAGS) -o watchdog src/main.c src/signal_manager.c src/process_manager.c

worker: workers/worker.c
	$(CC) $(CFLAGS) -o worker workers/worker.c

clean:
	rm -f watchdog worker

.PHONY: all clean
