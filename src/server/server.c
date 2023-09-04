#include <getopt.h>
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

void parse_args(Server* server, int argc, char* argv[])
{
	int	opt;
	int	long_i;
	static struct option long_options[] = {
		{"port",	no_argument,	0,	'P'},
		{"pending",	no_argument,	0,	'p'},
		{"max_buf",	no_argument,	0,	'b'},
		{"reuse port",	no_argument,	0,	'r'},
		{"forward",	no_argument,	0,	'f'},
		{"api_key",	no_argument,	0,	'k'}
	};
	
	opt = 0;
	long_i = 0;
	while ((opt = getopt_long(argc, argv,"P:p:b:r:f:k:",
		long_options, &long_i )) != -1) {
	switch (opt) {
	case 'P':
		if (sscanf(optarg, "%i", &server->port) != 1) {
			log_msg(stderr, "Error - port argument '%s' is not an integer.\n",
				optarg);
			close_server(server);
			exit(0);
		}
		break;
	case 'p':
		if (sscanf(optarg, "%i", &server->pend) != 1) {
			log_msg(stderr, "Error - pending argument '%s' is not an integer.\n",
				optarg);
			close_server(server);
			exit(0);
		}
		break;
	case 'b':
		if (sscanf(optarg, "%i", &server->buf_s) != 1) {
			log_msg(stderr, "Error - buffer size argument '%s' is not an integer.\n",
				optarg);
			close_server(server);
			exit(0);
		}
		break;
	case 'r':
		if (sscanf(optarg, "%i", &server->reuse) != 1) {
			log_msg(stderr, "Error - port reuse argument '%s' is neither 1 or 0.\n",
				optarg);
			close_server(server);
			exit(0);
		}
		break;
	case 'f':
		server->forwrd = calloc(strlen(optarg) + 1, sizeof(char));
		strcpy(server->forwrd, optarg);
		break;
	case 'k':
		if (sscanf(optarg, "%i", &server->id_key) != 1) {
			log_msg(stderr, "Error - id key argument '%s' is neither 1 or 0.\n",
				optarg);
			close_server(server);
			exit(0);
		}
		break;
	default:
		log_msg(stdout, "Usage: -P num -p num -b num -r num -f string -k num\n");
		close_server(server);
		exit(1);
	}}
}

void close_server(Server* server)
{
	Worker*	w;
	int	i;

	shutdown(server->socket, SHUT_RDWR);
	for (i = 0; i < server->work_s; i++) {
		w = server->worker[i];
		if (server->forwrd != NULL) {
			/* Curl is only initialized if server->forwrd is set */
			curl_slist_free_all(w->slist);
			if (w->server->id_key != 0) {
				/* Static curl url must be cleaned */
				curl_easy_cleanup(w->curl);
			}
			free(w->forwrd);
		}
		free_data(w->data);
		free(w->buf_rc);
		free(w->buf_sd);
		free(w);
	}
	if (server->forwrd != NULL) {
		curl_global_cleanup();
	}
	free(server->worker);
	free(server->host);
	free(server->forwrd);
	free(server);
	log_msg(stdout, "Server closed gracefully\n");
}

void set_curl_msg(Worker* w)
{
	if (w->server->id_key != 0) {
		snprintf(w->forwrd, w->forw_s, "%s/%s",	w->server->forwrd, w->data->id);
	} else {
		snprintf(w->forwrd, w->forw_s, "%s", w->server->forwrd);
	}

	/* Set url and data to send */
	curl_easy_setopt(w->curl, CURLOPT_URL, w->forwrd);
	curl_easy_setopt(w->curl, CURLOPT_POSTFIELDS, w->data->json);
}

int init_curl(Worker* w)
{
	w->curl = curl_easy_init();
	if (!w->curl) {
		return 1;
	}
	curl_easy_setopt(w->curl, CURLOPT_HTTPHEADER, w->slist);
	curl_easy_setopt(w->curl, CURLOPT_CONNECTTIMEOUT, 3L);
	curl_easy_setopt(w->curl, CURLOPT_TIMEOUT, 3L);
	return 0;
}

void init_socket_connection(Server* server)
{
	Sokadin	server_addr;

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
}

Server* init_server(int argc, char* argv[])
{
	Server* server;

	server = malloc(sizeof(Server));
	/* Default values to be overridden by argv[] */
	server->worker = NULL;
	server->work_s = 0;
	server->port = 5124;
	server->buf_s = 2048;
	server->pend = 256;
	server->reuse = 0;
	server->id_key = 0;
	server->forwrd = NULL;
	server->socket = socket(PF_INET, SOCK_STREAM, 0);
	server->host = get_ip_addr();

	parse_args(server, argc, argv);
	init_socket_connection(server);
	return server;
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
		"{\"data\": {\"signals\": {\"pack-len\": \"%d\"", d->pack_len);
	cur += concat_json(cur, end, "id", d->id);
	cur += concat_json(cur, end, "work-no", d->work_nb);
	cur += concat_json(cur, end, "cmd-code", d->cmd_code);
	cur += concat_json(cur, end, "checksum", d->checksum);

	p = d->cmd_para;
	while (p != NULL) {
		if (p->key != NULL) {
			cur += concat_json(cur, end, p->key, p->val);
		}
		p = p->next;
	}

	strcat(w->data->json, "}}}");
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
	set_curl_msg(w);
	res = curl_easy_perform(w->curl);

	if (res != CURLE_OK) {
		log_msg(stderr, "Warning - curl forward to %s failed: %s\n",
			w->forwrd, curl_easy_strerror(res));
		return 1;
	}
	worker_log(w, "> forw", w->data->json);
	return 0;
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
		if (res == 0 && w->server->forwrd != NULL && w->curl) {
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
	w->curl = NULL;
	w->slist = NULL;
	w->id = id;
	w->server = server;
	w->addr = NULL;
	w->addr_s = 0;
	w->socket = 0;
	w->forwrd = NULL;
	w->forw_s = 0;
	w->data = init_data(server->buf_s);
	w->buf_rc = calloc(server->buf_s, sizeof(char));
	w->buf_sd = calloc(server->buf_s, sizeof(char));

	if (w->server->forwrd != NULL) {
		curl_global_init(CURL_GLOBAL_DEFAULT);
		w->slist = curl_slist_append(w->slist, "Content-Type: application/json");
		w->slist = curl_slist_append(w->slist, "Accept: application/json");

		if (w->server->id_key != 0) {
			/* Dynamic forward url, init curl when forwarding msg */
			/* data->id_s includes '\0' char, +1 is for '/' sign */
			w->forw_s = strlen(w->server->forwrd) + w->data->id_s + 1;
			w->forwrd = calloc(w->forw_s, sizeof(char));
		} else {
			/* Static forward url, init curl on worker init */
			w->forw_s = strlen(w->server->forwrd) + 1;
			w->forwrd = calloc(w->forw_s, sizeof(char));
		}
		if (init_curl(w) != 0) {
			log_msg(stderr, "Error - worker %d could not "
				"initialize curl connection to %s\n",
				w->id, server->forwrd);
		}
	}
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
	log_msg(stdout, "\tHOST:    %s\n", server->host);
	log_msg(stdout, "\tPORT:    %d\n", server->port);
	log_msg(stdout, "\tPENDING: %d\n", server->pend);
	log_msg(stdout, "\tMAX_BUF: %d\n", server->buf_s);
	log_msg(stdout, "\tREUSE:   %d\n", server->reuse);
	if (server->forwrd != NULL) {
		if (server->id_key != 0) {
			log_msg(stdout, "\tFORWARD: %s/DEVICE_ID\n", server->forwrd);
		} else {
			log_msg(stdout, "\tFORWARD: %s\n", server->forwrd);
		}
	} else {
		log_msg(stdout, "\tFORWARD: %s\n", "None");
	}
	log_msg(stdout, "\tAPI_KEY: %d\n", server->id_key);
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
