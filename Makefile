# ==============================================================================
# Makefile for FCFS Batch Process Dispatcher
# Operating Systems Coursework
# ==============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
TARGETS = dispatcher sleepy process

all: $(TARGETS)

dispatcher: dispatcher.o pcb.o queue.o
	$(CC) $(CFLAGS) -o $@ $^

dispatcher.o: dispatcher.c pcb.h queue.h
	$(CC) $(CFLAGS) -c dispatcher.c

pcb.o: pcb.c pcb.h
	$(CC) $(CFLAGS) -c pcb.c

queue.o: queue.c queue.h pcb.h
	$(CC) $(CFLAGS) -c queue.c

sleepy: sleepy.c
	$(CC) $(CFLAGS) -o $@ sleepy.c

process: process.c
	$(CC) $(CFLAGS) -o $@ process.c

test-simple: all
	./dispatcher dispatchlist_simple.txt

test-full: all
	./dispatcher dispatchlist_full.txt

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean test-simple test-full
