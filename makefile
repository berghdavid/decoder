CC=gcc
CFLAGS=-Wall -pedantic-errors -pthread
LDFLAGS=-lm -pthread
OPT_FLAGS=-O3
DEBUG_FLAGS=-ggdb3

CLIENTS = $(wildcard src/clients/*.c)
SERVER = $(wildcard src/server/*.c)

all: clients server debug_server debug_clients

run_clients: clients
	./clients

clients: $(CLIENTS)
	$(CC) -o clients $(CFLAGS) $(OPT_FLAGS) $(CLIENTS) $(LDFLAGS)

run_server: server
	./server

server: $(SERVER)
	$(CC) -o server $(CFLAGS) $(OPT_FLAGS) $(SERVER) $(LDFLAGS)

valgrind_clients: debug_clients
	valgrind --leak-check=full --track-origins=yes ./d_clients

debug_clients: $(CLIENTS)
	$(CC) -o d_clients $(DEBUG_FLAGS) $(CFLAGS) $(CLIENTS) $(LDFLAGS)

valgrind_server: debug_server
	valgrind --leak-check=full --track-origins=yes ./d_server

debug_server: $(CLIENTS)
	$(CC) -o d_server $(DEBUG_FLAGS) $(CFLAGS) $(CLIENTS) $(LDFLAGS)

clean: all
	rm server
	rm d_server
	rm clients
	rm d_clients
