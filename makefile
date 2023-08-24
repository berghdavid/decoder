CC=gcc
CFLAGS=-Wall -pedantic-errors -pthread
LDFLAGS=-lm -pthread
OPT_FLAGS=-O3
DEBUG_FLAGS=-ggdb3

CLIENTS = $(wildcard src/clients/*.c)
CLIENTS2 = $(wildcard src/clients2/*.c)
SERVER = $(wildcard src/server/*.c)
SERVER2 = $(wildcard src/server2/*.c)

all: clients server debug_server debug_clients

run_clients2: clients2
	./clients2

run_clients: clients
	./clients

clients2: $(CLIENTS2)
	$(CC) -o clients2 $(CFLAGS) $(OPT_FLAGS) $(CLIENTS2) $(LDFLAGS)

clients: $(CLIENTS)
	$(CC) -o clients $(CFLAGS) $(OPT_FLAGS) $(CLIENTS) $(LDFLAGS)

run_server2: server2
	./server2 127.0.0.1 5142 100

run_server: server
	./server

server2: $(SERVER2)
	$(CC) -o server2 $(CFLAGS) $(OPT_FLAGS) $(SERVER2) $(LDFLAGS)

server: $(SERVER)
	$(CC) -o server $(CFLAGS) $(OPT_FLAGS) $(SERVER) $(LDFLAGS)

valgrind_clients2: debug_clients2
	valgrind --leak-check=full --track-origins=yes ./d_clients2

debug_clients2: $(CLIENTS2)
	$(CC) -o d_clients2 $(DEBUG_FLAGS) $(CFLAGS) $(CLIENTS2) $(LDFLAGS)

valgrind_clients: debug_clients
	valgrind --leak-check=full --track-origins=yes ./d_clients

debug_clients: $(CLIENTS)
	$(CC) -o d_clients $(DEBUG_FLAGS) $(CFLAGS) $(CLIENTS) $(LDFLAGS)

valgrind_server2: debug_server2
	valgrind --leak-check=full --track-origins=yes ./d_server2 127.0.0.1 5142 100

debug_server2: $(SERVER2)
	$(CC) -o d_server2 $(DEBUG_FLAGS) $(CFLAGS) $(SERVER2) $(LDFLAGS)

valgrind_server: debug_server
	valgrind --leak-check=full --track-origins=yes ./d_server

debug_server: $(SERVER)
	$(CC) -o d_server $(DEBUG_FLAGS) $(CFLAGS) $(SERVER) $(LDFLAGS)

clean: all
	rm server
	rm d_server
	rm clients
	rm d_clients
