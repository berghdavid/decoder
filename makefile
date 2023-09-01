CC=gcc
CFLAGS=-Wall -pedantic-errors
LDFLAGS=-lm -pthread -lcurl
OPT_FLAGS=-O3
DEBUG_FLAGS=-ggdb3

CLIENTS = $(wildcard src/clients/*.c)
SERVER = $(wildcard src/server/*.c)
UTILS = $(wildcard src/utils/*.c)
PARSER = src/utils/parser.c
QUEUE = src/utils/queue.c
LOGGER = src/utils/logger.c

all: clients server debug_server debug_clients

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
	./server 127.0.0.1 5124 100 2048 1 localhost:5111

server: $(SERVER) $(PARSER) $(LOGGER)
	$(CC) -o server $(CFLAGS) $(OPT_FLAGS) $(SERVER) $(PARSER) $(LOGGER) $(LDFLAGS)

valgrind_server: debug_server
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./d_server 127.0.0.1 5124 100 2048 1 localhost:5111

debug_server: $(SERVER) $(PARSER) $(LOGGER)
	$(CC) -o d_server $(DEBUG_FLAGS) $(CFLAGS) $(SERVER) $(PARSER) $(LOGGER) $(LDFLAGS)

# Cleanup

clean: all
	rm server
	rm d_server
	rm clients
	rm d_clients
