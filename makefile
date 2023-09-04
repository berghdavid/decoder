CC=gcc
CFLAGS=-Wall -pedantic-errors
LDFLAGS=-lm -pthread -lcurl
OPT_FLAGS=-O3
DEBUG_FLAGS=-ggdb3

CLIENTS = $(wildcard src/c/clients/*.c)
SERVER = $(wildcard src/c/server/*.c)
UTILS = $(wildcard src/c/utils/*.c)
PARSER = src/c/utils/parser.c
QUEUE = src/c/utils/queue.c
LOGGER = src/c/utils/logger.c

PARAMS = -P 5124 -p 100 -b 2048 -r 1 -f localhost:5111 -k 1

all: clients server debug_server debug_clients

# Docker

image:
	docker build -t fifo .

# Clients

run_clients: clients
	./clients

clients: $(CLIENTS) $(UTILS)
	$(CC) -o clients $(CFLAGS) $(OPT_FLAGS) $(CLIENTS) $(UTILS) $(LDFLAGS)

valgrind_clients: debug_clients
	valgrind --leak-check=full --track-origins=yes ./d_clients

debug_clients: $(CLIENTS) $(UTILS)
	$(CC) -o d_clients $(DEBUG_FLAGS) $(CFLAGS) $(CLIENTS) $(UTILS) $(LDFLAGS)

# Server

run_server: server
	./server $(PARAMS)

server: $(SERVER) $(PARSER) $(LOGGER)
	$(CC) -o server $(CFLAGS) $(OPT_FLAGS) $(SERVER) $(PARSER) $(LOGGER) $(LDFLAGS)

valgrind_server: debug_server
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./d_server $(PARAMS)

debug_server: $(SERVER) $(PARSER) $(LOGGER)
	$(CC) -o d_server $(DEBUG_FLAGS) $(CFLAGS) $(SERVER) $(PARSER) $(LOGGER) $(LDFLAGS)

# Python server

py_server:
	python3 src/py/server.py

# Cleanup

clean: all
	rm server
	rm d_server
	rm clients
	rm d_clients
