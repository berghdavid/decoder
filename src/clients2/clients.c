#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "clients.h"

void free_client(Client* c)
{
	Worker*	w;
	int	i;

	for (i = 0; i < c->work_s; i++) {
		w = c->worker[i];
		free(w->buf_rc);
		free(w->buf_sd);
		free(w);
	}
	free(c->worker);
	free(c->thr);
	free(c);
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

void* send_data(void* arg)
{
	Worker*	w;
	int	i;
	
	w = (Worker*) arg;

	w->socket = connect(w->client->socket, (Sockaddr*) &w->client->sockad, 
		sizeof(w->client->sockad));

	if (w->socket < 0) {
		printf("Unsuccessful connection...\n");
		exit(0);
	}

	send(w->socket, w->buf_sd, sizeof(char) * strlen(w->buf_sd), 0);
	print_sent(w);
	recv(w->socket, w->buf_rc, sizeof(char) * strlen(w->buf_rc), 0);
	print_received(w);
	return NULL;
}

void start_workers(Client* c)
{
	int	i;

	for (i = 0; i < c->work_s; i++) {
		pthread_create(c->thr[i], NULL, send_data, c->worker[i]);
	}

	/* Wait for all threads, then join them when finished */
	for (i = 0; i < c->work_s; i++) {
		if (pthread_join(c->thr[i], NULL) != 0) {
			printf("Could not join with thread %d\n", i);
		}
	}
}

Worker* init_worker(Client* c, int id, char* data)
{
	Worker*	w;

	w = malloc(sizeof(Worker));
	w->id = id;
	w->addr = NULL;
	w->addr_s = 0;
	w->socket = 0;
	w->buf_rc = calloc(2048, sizeof(char));
	w->buf_sd = calloc(2048, sizeof(char));
	strcpy(w->buf_sd, data);
}

void init_workers(Client* c, int workers)
{
	int	i;
	char*	data[3] = {
		"fadf",
		"fadf",
		"fadf",
	};

	c->work_s = workers;
	c->worker = malloc(workers * sizeof(Worker*));
	c->thr = malloc(workers * sizeof(pthread_t));
	for (i = 0; i < workers; i++) {
		c->worker[i] = init_worker(c, i, data[i]);
	}
}

Client* init_client(int workers)
{
	Client*	c;

	c = malloc(sizeof(Client));
	c->buf_s = 2048;
	c->host = "127.0.0.1";
	c->port = 5124;
	c->socket = socket(PF_INET, SOCK_STREAM, 0);
	c->work_s = 0;
	c->worker = NULL;
	c->thr = NULL;

	memset(&c->sockad, '\0', sizeof(c->sockad));
	c->sockad.sin_family = AF_INET;
	c->sockad.sin_port = htons(c->port);
	c->sockad.sin_addr.s_addr = inet_addr(c->host);

	init_workers(c, 3);
	return c;
}

int main()
{
	Client*		c;

	c = init_client(3);
	start_workers(c);
	free_client(c);
	return 0;
}
