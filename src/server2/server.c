#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "queue.h"
#include "server.h"

/* TODO: Replace 2048 with an option */

Server* init_server(int argc, char* argv[])
{
	Server* 		server;
	struct sockaddr_in	server_addr;

	if (argc != 4) {
		fprintf(stderr, "Missing parameters:\n");
		fprintf(stderr, "\tHost: For example 127.0.0.1\n");
		fprintf(stderr, "\tPort: For example 5142\n");
		fprintf(stderr, "\tPending connections: For example 100\n");
		exit(0);
	}

	server = malloc(sizeof(Server));
	server->worker = NULL;
	server->work_s = 0;
	server->host = NULL;
	server->port = 8080;
	server->pend = 1;
	server->socket = socket(PF_INET, SOCK_STREAM, 0);

	server->host = argv[1];
	if (sscanf (argv[2], "%i", &server->port) != 1) {
		fprintf(stderr, "Error - port argument not an integer.\n");
		exit(0);
	}
	if (sscanf (argv[3], "%i", &server->pend) != 1) {
		fprintf(stderr, "Error - pending argument not an integer.\n");
		exit(0);
	}

	memset(&server_addr, '\0', sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server->port);
	server_addr.sin_addr.s_addr = inet_addr(server->host);

	if (bind(server->socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) != 0) {
		fprintf(stderr, "Error - Unsuccessfully bound to socket.\n");
		exit(0);
	}
	listen(server->socket, server->pend);
	return server;
}

Worker* init_worker(int id, Server* server)
{
	Worker*	w;

	w = malloc(sizeof(Worker));
	w->id = id;
	w->server = server;
	w->addr = NULL;
	w->addr_s = 0;
	w->socket = 0;
	w->buf = calloc(2048, sizeof(char));
	return w;
}

void init_workers(Server* server, int workers)
{
	int	i;

	server->work_s = workers;
	server->worker = malloc(workers * sizeof(Worker*));
	for (i = 0; i < workers; i++) {
		server->worker[i] = init_worker(i, server);
	}
}

void* work(void* arg)
{
	Worker* w = (Worker*) arg;

	printf("Listening for clients...\n");
	while (1) {
		w->socket = accept(w->server->socket, w->addr, &w->addr_s);
		if (recv(w->socket, w->buf, 2048 * sizeof(char), 0) == -1) {
			fprintf(stderr,	"Error when worker %d tried to receive.\n",
				w->id);
		}
		printf("Received: %s\n", w->buf);
		send(w->socket, w->buf, 2048 * sizeof(char), 0);
		close(w->socket);
		printf("Sent: %s\n", w->buf);
		if (w->buf[0] == 'q') {
			break;
		}
	}
	return NULL;
}

void start_server(Server* server)
{
	/* TODO: Multithreading =) */

	work(server->worker[0]);
}

void close_server(Server* server)
{
	int	i;

	shutdown(server->socket, SHUT_RDWR);

	for (i = 0; i < server->work_s; i++) {
		free(server->worker[i]->buf);
		free(server->worker[i]);
	}
	free(server->worker);
	free(server);
	printf("Server closed gracefully\n");
}

int main(int argc, char* argv[])
{
	Server*	server;

	server = init_server(argc, argv);
	init_workers(server, 1);
	start_server(server);
	close_server(server);
	return 0;
}
