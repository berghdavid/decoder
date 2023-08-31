#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curl/curl.h>
#include <arpa/inet.h>
#include "server.h"
#include "../utils/parser.h"

void close_server(Server* server)
{
	int	i;

	shutdown(server->socket, SHUT_RDWR);
	curl_slist_free_all(server->slist);
	curl_easy_cleanup(server->curl);
	curl_global_cleanup();
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

int init_curl(Server* server)
{
	CURL*		curl;
	CurlSlist*	slist;
	
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	slist = NULL;
	if (!curl) {
		return 1;
	}
	slist = curl_slist_append(slist, "Content-Type: application/json");
	slist = curl_slist_append(slist, "Accept: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
	/* TODO: Pass connection timeout limits as parameters */
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
	/* TODO: Pass URL as parameter */
	//curl_easy_setopt(curl, CURLOPT_URL, "https://online.staging.traxmate.io:8000");
	curl_easy_setopt(curl, CURLOPT_URL, "localhost:5111");
	server->curl = curl;
	server->slist = slist;
	return 0;
}

Server* init_server(int argc, char* argv[])
{
	Server* 		server;
	struct sockaddr_in	server_addr;

	/* TODO: Replace with defaults and argument parsing */
	if (argc != 6) {
		fprintf(stderr, "Missing parameters:\n");
		fprintf(stderr, "\tHost: For example 127.0.0.1\n");
		fprintf(stderr, "\tPort: For example 5124\n");
		fprintf(stderr, "\tPending connections: For example 100\n");
		fprintf(stderr, "\tMax buf_size: For example 2048\n");
		fprintf(stderr, "\tReuse port: 0 or 1\n");
		exit(0);
	}

	server = malloc(sizeof(Server));
	server->worker = NULL;
	server->work_s = 0;
	server->host = NULL;
	server->port = 5124;
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

	if (init_curl(server) != 0) {
		close_server(server);
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

void response_A03(Worker* w)
{
	char*		resp;
	time_t		curr_t;
	struct tm*	info_t;
	char		t_str[12 + 1];
	int		pack_len;
	int		i;
	char		xor;

	time(&curr_t);
	info_t = localtime(&curr_t);
	strftime(t_str, sizeof(t_str), "%y%m%d%H%M%S", info_t);
	pack_len = 4 + strlen(w->data->id) + strlen(w->data->work_nb) +
		strlen(w->data->cmd_code) + strlen(t_str);

	resp = w->buf_sd;
	resp[0] = '#';
	resp[1] = '#';
	snprintf(resp, 5, "##%d", pack_len);
	strcat(resp, ",");
	strcat(resp, w->data->id);
	strcat(resp, ",");
	strcat(resp, w->data->work_nb);
	strcat(resp, ",");
	strcat(resp, w->data->cmd_code);
	strcat(resp, ",");
	strcat(resp, t_str);

	xor = 0;
	for (i = 2; i < strlen(resp); i++) {
		xor ^= resp[i];
	}
	strcat(resp, "*");
        sprintf(resp + strlen(resp), "%02X", xor);
	strcat(resp, "\r\n");
}

void build_response(Worker* w)
{
	if (strcmp(w->data->cmd_code, "A03") == 0) {
		response_A03(w);
	} else if (strcmp(w->data->cmd_code, "A10") == 0) {
		/* There is no response to A10 heartbeat */
		return;
	}
}

int forward_data(Worker* w)
{
	CURLcode	res;

	/* TODO: Make thread-safe */

	curl_easy_setopt(w->server->curl, CURLOPT_POSTFIELDS, w->data->json);
	res = curl_easy_perform(w->server->curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "Error - %s\n", curl_easy_strerror(res));
	}
	return res == CURLE_OK;
}

void reset_data(Worker* w)
{
	/* TODO: Maybe skip string reset */
	memset(w->buf_sd, 0, strlen(w->buf_sd));
	free_params(w->data);
}

void print_received(Worker* w)
{
	char*		str;
	size_t		len;
	time_t		curr_t;
	struct tm*	info_t;
	char		t_str[32];

	time(&curr_t);
	info_t = localtime(&curr_t);
	strftime(t_str, sizeof(t_str), "%Y-%m-%d %H:%M:%S", info_t);
	printf("%s [ Worker %d < fifo ]: ", t_str, w->id);
	
	str = w->buf_rc;
	len = strlen(str);
	if (len >= 2 && str[len - 2] == '\r' && str[len - 1] == '\n') {
		/* Print the string without the last two characters */
		printf("%.*s", (int) (len - 2), str);
	} else {
		printf("%s", str);
	}
	printf("\n\n");
}

void print_sent(Worker* w)
{
	char*		str;
	size_t		len;
	time_t		curr_t;
	struct tm*	info_t;
	char		t_str[32];

	time(&curr_t);
	info_t = localtime(&curr_t);
	strftime(t_str, sizeof(t_str), "%Y-%m-%d %H:%M:%S", info_t);
	printf("%s [ Worker %d > fifo ]: ", t_str, w->id);
	
	str = w->buf_sd;
	len = strlen(str);
	if (len >= 2 && str[len - 2] == '\r' && str[len - 1] == '\n') {
		/* Print the string without the last two characters */
		printf("%.*s", (int) (len - 2), str);
	} else {
		printf("%s", str);
	}
	printf("\n\n");
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
			fprintf(stderr,	"Error - worker %d could not receive.\n", w->id);
			continue;
		}
		print_received(w);
		if (parse_package(w->data, w->buf_rc) == 0) {
			build_response(w);
			send(w->socket, w->buf_sd, w->server->buf_s * sizeof(char), 0);
			print_sent(w);
		}
		close(w->socket);
		build_forward_req(w->data);
		forward_data(w);
		reset_data(w);
		if (w->buf_rc[0] == 'q') {
			/* TODO: Remove this quit condition */
			break;
		}
	}
	return NULL;
}

Worker* init_worker(int id, Server* server)
{
	Worker*	w;

	w = malloc(sizeof(Worker));
	w->id = id;
	w->server = server;
	w->addr = NULL;
	w->addr_s = 0;
	w->data = init_data(server->buf_s);
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
