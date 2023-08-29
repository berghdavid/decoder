#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "queue.h"
#include "server.h"
#include "parser.h"

void close_server(Server* server)
{
	int	i;

	shutdown(server->socket, SHUT_RDWR);
	for (i = 0; i < server->work_s; i++) {
		free_data(server->worker[i]->data);
		free(server->worker[i]->buf_rc);
		free(server->worker[i]->buf_sd);
		free(server->worker[i]);
	}
	free(server->worker);
	free(server);
	printf("Server closed gracefully\n");
}

Server* init_server(int argc, char* argv[])
{
	Server* 		server;
	struct sockaddr_in	server_addr;

	/* TODO: Replace with defaults and argument parsing */
	if (argc != 6) {
		fprintf(stderr, "Missing parameters:\n");
		fprintf(stderr, "\tHost: For example 127.0.0.1\n");
		fprintf(stderr, "\tPort: For example 5142\n");
		fprintf(stderr, "\tPending connections: For example 100\n");
		fprintf(stderr, "\tMax buf_size: For example 2048\n");
		fprintf(stderr, "\tReuse port: 0 or 1\n");
		exit(0);
	}

	server = malloc(sizeof(Server));
	server->worker = NULL;
	server->work_s = 0;
	server->host = NULL;
	server->port = 5142;
	server->pend = 1;
	server->reuse = 0;
	server->socket = socket(PF_INET, SOCK_STREAM, 0);

	server->host = argv[1];
	if (sscanf (argv[2], "%i", &server->port) != 1) {
		fprintf(stderr, "Error - port argument not an integer.\n");
		close_server(server);
		exit(0);
	}
	if (sscanf (argv[3], "%i", &server->pend) != 1) {
		fprintf(stderr, "Error - pending argument not an integer.\n");
		close_server(server);
		exit(0);
	}
	if (sscanf (argv[4], "%i", &server->buf_s) != 1) {
		fprintf(stderr, "Error - buffer size argument not an integer.\n");
		close_server(server);
		exit(0);
	}
	if (sscanf (argv[4], "%i", &server->reuse) != 1) {
		fprintf(stderr, "Error - port reuse is not an.\n");
		close_server(server);
		exit(0);
	}

	memset(&server_addr, '\0', sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server->port);
	server_addr.sin_addr.s_addr = inet_addr(server->host);

	if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &server->reuse,
		sizeof(int)) < 0) {
		fprintf(stderr, "Error - Unable to set SO_REUSEADDR option.\n");
		close_server(server);
		exit(0);
	}

	if (bind(server->socket, (struct sockaddr*) &server_addr,
		sizeof(server_addr)) != 0) {
		fprintf(stderr, "Error - Unsuccessfully bound to socket.\n");
		close_server(server);
		exit(0);
	}
	listen(server->socket, server->pend);
	return server;
}

void print_received(Worker* w)
{
	printf("[ Worker %d < fifo ]: %s\n", w->id, w->buf_rc);
}

void print_sent(Worker* w)
{
	printf("[ Worker %d > fifo ]: %s\n", w->id, w->buf_sd);
}

Worker* init_worker(int id, Server* server)
{
	Worker*	w;

	w = malloc(sizeof(Worker));
	w->id = id;
	w->server = server;
	w->addr = NULL;
	w->addr_s = 0;
	w->data = init_data();
	w->socket = 0;
	w->buf_rc = calloc(server->buf_s, sizeof(char));
	w->buf_sd = calloc(server->buf_s, sizeof(char));
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
	Worker*	w;
	
	w = (Worker*) arg;
	printf("Worker %d is accepting clients at %s:%d\n",
		w->id, w->server->host, w->server->port);

	while (1) {
		w->socket = accept(w->server->socket, w->addr, &w->addr_s);
		if (recv(w->socket, w->buf_rc, w->server->buf_s * sizeof(char), 0) == -1) {
			fprintf(stderr,	"Error when worker %d tried to receive.\n", w->id);
			continue;
		}
		print_received(w);
		if (parse_package(w->data, w->buf_rc) == 0) {
			print_sent(w);
			send(w->socket, w->buf_rc, w->server->buf_s * sizeof(char), 0);
		}
		close(w->socket);
		reset_data(w->data);
		if (w->buf_rc[0] == 'q') {
			/* TODO: Remove this quit condition */
			break;
		}
	}
	return NULL;
}

void start_server(Server* server)
{
	/* TODO: Multithreading =) */
	int	i;

	for (i = 0; i < server->work_s; i++) {
		work(server->worker[i]);
	}
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
