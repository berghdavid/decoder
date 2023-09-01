#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <curl/curl.h>

#include "server.h"
#include "../utils/parser.h"
#include "../utils/logger.h"

char* get_ip_addr()
{
	FILE*	fp;
	char*	command = "ifconfig | grep 'inet ' | awk '{print $2}' | cut -d':' -f2";
	char*	ipAddress;
	char*	newlinePos;
	char	buffer[256];

	fp = popen(command, "r");
	ipAddress = NULL;
	if (fp == NULL) {
		log_msg(stderr, "Error - couldn't run 'popen()'");
		return NULL;
	}

	if (fgets(buffer, sizeof(buffer), fp) != NULL) {
		ipAddress = (char*) malloc(strlen(buffer) + 1);
		if (ipAddress != NULL) {
			strcpy(ipAddress, buffer);
			newlinePos = strchr(ipAddress, '\n');
			if (newlinePos != NULL) {
				*newlinePos = '\0';
			}
		}
	}
	pclose(fp);
	return ipAddress;
}

void worker_log(Worker* w, char* other, char* log)
{
	char*		str;
	size_t		len;
	time_t		curr_t;
	struct tm*	info_t;
	char		t_str[32];

	time(&curr_t);
	info_t = localtime(&curr_t);
	strftime(t_str, sizeof(t_str), "%Y-%m-%d %H:%M:%S", info_t);
	log_msg(stdout, "%s [ Worker %d %s ]: ", t_str, w->id, other);

	str = log;
	len = strlen(str);
	if (len >= 2 && str[len - 2] == '\r' && str[len - 1] == '\n') {
		/* Print the string without the last two characters */
		log_msg(stdout, "%.*s\n", (int) (len - 2), str);
	} else {
		log_msg(stdout, "%s\n", str);
	}
}

int concat_json(char* cur, const char* end, char* key, char* val)
{
	return snprintf(cur, end - cur, ", \"%s\": \"%s\"", key, val);
}

void build_forward_req(Worker* w)
{
	Param*		p;
	Data*		d;
	char*		cur;
	const char*	end;

	d = w->data;
	cur = d->json;
	end = d->json + d->json_s;

	cur += snprintf(cur, end - cur,
		"{\"data\": {\"signals\": {\"pack-len\": \"%d\",", d->pack_len);
	cur += concat_json(cur, end, "id", d->id);
	cur += concat_json(cur, end, "work-no", d->work_nb);
	cur += concat_json(cur, end, "cmd-code", d->cmd_code);
	cur += concat_json(cur, end, "checksum", d->checksum);

	p = d->cmd_para;
	while (p != NULL) {
		cur += concat_json(cur, end, p->key, p->val);
		p = p->next;
	}

	strcat(w->data->json, "}}}");
}

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
	free(server->host);
	free(server->forwrd);
	free(server);
	log_msg(stdout, "Server closed gracefully\n");
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
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, server->forwrd);
	server->curl = curl;
	server->slist = slist;
	return 0;
}

Server* init_server(int argc, char* argv[])
{
	Server* server;
	Sokadin	server_addr;

	/* TODO: Replace with defaults and argument parsing */
	if (argc != 6) {
		log_msg(stderr, "Missing parameters:\n");
		log_msg(stderr, "\tPort: For example 5124\n");
		log_msg(stderr, "\tPending connections: For example 100\n");
		log_msg(stderr, "\tMax buf_size: For example 2048\n");
		log_msg(stderr, "\tReuse port: 0 or 1\n");
		log_msg(stderr, "\tForward url: For example localhost:80\n");
		exit(0);
	}

	server = malloc(sizeof(Server));
	/* Default values to be overridden by argv[] */
	server->worker = NULL;
	server->work_s = 0;
	server->host = NULL;
	server->port = 5124;
	server->pend = 1;
	server->reuse = 0;
	server->forwrd = calloc(256, sizeof(char));
	server->socket = socket(PF_INET, SOCK_STREAM, 0);

	server->host = get_ip_addr();
	if (server->host == NULL) {
		close_server(server);
		exit(0);
	}
	if (sscanf(argv[1], "%i", &server->port) != 1) {
		log_msg(stderr, "Error - port argument '%s' is not an integer.\n",
			argv[1]);
		close_server(server);
		exit(0);
	}
	if (sscanf(argv[2], "%i", &server->pend) != 1) {
		log_msg(stderr, "Error - pending argument '%s' is not an integer.\n",
			argv[2]);
		close_server(server);
		exit(0);
	}
	if (sscanf(argv[3], "%i", &server->buf_s) != 1) {
		log_msg(stderr, "Error - buffer size argument '%s' is not an integer.\n",
			argv[3]);
		close_server(server);
		exit(0);
	}
	if (sscanf(argv[4], "%i", &server->reuse) != 1) {
		log_msg(stderr, "Error - port reuse argument '%s' is neither 1 or 0.\n",
			argv[4]);
		close_server(server);
		exit(0);
	}
	if (strlen(argv[5]) < 256) {
		strcpy(server->forwrd, argv[5]);
	} else {
		log_msg(stderr, "Error - forward argument '%s' is larger than 256 bytes.\n",
			argv[5]);
		close_server(server);
		exit(0);
	}

	if (init_curl(server) != 0) {
		log_msg(stderr, "Error - could not initialize curl connection to %s\n",
			server->forwrd);
		close_server(server);
		exit(0);
	}

	memset(&server_addr, '\0', sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server->port);
	server_addr.sin_addr.s_addr = inet_addr(server->host);

	if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &server->reuse,
		sizeof(int)) < 0) {
		log_msg(stderr, "Error - Unable to set SO_REUSEADDR option.\n");
		close_server(server);
		exit(0);
	}

	if (bind(server->socket, (struct sockaddr*) &server_addr,
		sizeof(server_addr)) != 0) {
		log_msg(stderr,
			"Error - Unsuccessfully bound to socket at '%s:%d'\n",
			server->host, server->port);
		close_server(server);
		exit(0);
	}

	if (listen(server->socket, server->pend) != 0) {
		log_msg(stderr,	"Error - Unsuccessfully listened to socket "
			"with max '%d' pending connections\n", server->pend);
		close_server(server);
		exit(0);
	}
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
		/* Fifo protocol says no response to A10 heartbeat signal */
		return;
	}
}

int forward_data(Worker* w)
{
	CURLcode	res;

	/* TODO: Make thread-safe if multithreading */

	curl_easy_setopt(w->server->curl, CURLOPT_POSTFIELDS, w->data->json);
	res = curl_easy_perform(w->server->curl);
	if (res == CURLE_OK) {
		worker_log(w, "> forw", w->data->json);
		return 0;
	}
	log_msg(stderr, "Warning - curl forward failed: %s\n", curl_easy_strerror(res));
	return 1;
}

void reset_data(Worker* w)
{
	/* TODO: Maybe skip string reset */
	memset(w->buf_sd, 0, strlen(w->buf_sd));
	free_params(w->data);
}

void* work(void* arg)
{
	Worker*	w;
	int	res;
	
	w = (Worker*) arg;
	worker_log(w, "", "Accepting clients");

	while (1) {
		w->socket = accept(w->server->socket, w->addr, &w->addr_s);
		if (recv(w->socket, w->buf_rc, w->server->buf_s * sizeof(char), 0) == -1) {
			log_msg(stderr,	"Error - worker %d could not receive.\n", w->id);
			continue;
		}
		worker_log(w, "< fifo", w->buf_rc);
		res = parse_package(w->data, w->buf_rc);
		if (res == 0) {
			build_response(w);
			send(w->socket, w->buf_sd, w->server->buf_s * sizeof(char), 0);
			worker_log(w, "> fifo", w->buf_sd);
		}
		close(w->socket);
		if (res == 0) {
			build_forward_req(w);
			forward_data(w);
		}
		reset_data(w);
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
	int	i;

	log_msg(stdout, " ------------- Starting server -------------\n");
	log_msg(stdout, "\tHOST: %s\n", server->host);
	log_msg(stdout, "\tPORT: %d\n", server->port);
	log_msg(stdout, "\tPENDING: %d\n", server->pend);
	log_msg(stdout, "\tMAX_BUF: %d\n", server->buf_s);
	log_msg(stdout, "\tREUSE: %d\n", server->reuse);
	log_msg(stdout, "\tFORWARD: %s\n", server->forwrd);
	log_msg(stdout, " -------------------------------------------\n");

	/* TODO: Multithreading =) */
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
