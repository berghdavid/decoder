#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>

#include "queue.h"
#include "server.h"

#define PORT 5142       /* Port used				*/
#define BUF_SIZE 5      /* Size of buffer array			*/
#define DATA_SIZE 10    /* Total size of client array		*/
#define T_SIZE 3        /* Number of worker threads		*/
#define BACKLOG 5       /* Max amount of pending connections	*/

pthread_mutex_t lock_work = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_work = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_queue = PTHREAD_COND_INITIALIZER;

int buf[BUF_SIZE];
int active_id = -1;

int* sort_array()
{
	int	key;
	int	i;
	int	j;

	for (i = 1; i < BUF_SIZE; i++) {
		key = buf[i];
		j = i - 1;

		while (j >= 0 && buf[j] > key) {
			buf[j + 1] = buf[j];
			j = j - 1;
		}
		buf[j + 1] = key;
	}
	return buf;
}

void print_buf(char s[], int thr_id, int buf[])
{
	int	i;

	printf("Thread %d %s: { ", thr_id, s);
	for(i = 0; i < BUF_SIZE; i++) {
		printf("%d ", buf[i]);
	}
	printf("}\n");
	printf("---------------------------\n");
}

int create_socket()
{
	struct sockaddr_in	server_addr;
	int			server_socket;

	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	memset(&server_addr, '\0', sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) != 0) {
		printf("Unsuccessfully bound to socket...\n");
		exit(0);
	}
	listen(server_socket, BACKLOG);
	return server_socket;
}

void* wait_client(void* arg)
{
	struct sockaddr	client_addr;
	socklen_t	addr_size;
	info*		t_info;
	int		client_socket;
	int		i;

	t_info = (struct info*) arg;
	
	while(1) {
		client_socket = accept(t_info->server_socket, &client_addr, &addr_size);
		enq(t_info->q, t_info->id);
		pthread_cond_signal(&cond_queue);

		for (i = 0; i < DATA_SIZE; i += BUF_SIZE) {
			pthread_mutex_lock(&lock_work);
			while (active_id != t_info->id) {
				/* Wait for main thread to broadcast */
				pthread_cond_wait(&cond_work, &lock_work);
			}
			
			recv(client_socket, buf, BUF_SIZE * sizeof(int), 0);
			print_buf("received", t_info->id, buf);
			send(client_socket, sort_array(), BUF_SIZE * sizeof(int), 0);
			print_buf("sent", t_info->id, buf);

			/* If client has more packets to send, enqueue */
			if (i < DATA_SIZE - BUF_SIZE) {
				enq(t_info->q, t_info->id);
			}

			/* Unlock and signal the main thread */
			active_id = -1;
			pthread_cond_signal(&cond_queue);
			pthread_mutex_unlock(&lock_work);
		}
	}
}

void* shutdown_server(void* arg)
{
	char	str[100];
	bool*	quit;

	quit = (bool*) arg;
	while (!*quit) {
		printf("Enter 'exit' to shutdown server.\n");
		printf("--------------------------------\n");
		if (scanf("%s", str) == 1 && strcmp(str, "exit") == 0) {
			*quit = true;
			pthread_cond_signal(&cond_queue);
		}
	}
	return NULL;
}

int main()
{
	pthread_t	thr[T_SIZE + 1];
	queue*		q;
	info*		d;
	int		server_socket;
	int		i;
	bool		quit;

	q = queue_create();
	server_socket = create_socket();
	quit = false;

	/* Start worker threads */
	for (i = 0; i < T_SIZE; i++) {
		d = malloc(sizeof(info));
		d->id = i;
		d->server_socket = server_socket;
		d->q = q;
		pthread_create(&thr[i], NULL, wait_client, d);
		pthread_detach(thr[i]);
	}

	/* Start shutdown thread */
	pthread_create(&thr[T_SIZE], NULL, shutdown_server, &quit);
	pthread_detach(thr[T_SIZE]);

	/* Distribute work by signalling the threads in queue */
	printf("Listening for clients...\n");
	while (1) {
		pthread_mutex_lock(&lock_work);
		while (queue_empty(q) || active_id != -1) {
			pthread_cond_wait(&cond_queue, &lock_work);
			if (quit) {
				printf("Shutting down server...\n");
				for (i = 0; i < T_SIZE + 1; i++) {
					pthread_join(thr[i], NULL);
				}
				queue_destroy(q);
				return 0;
			}
		}
		active_id = deq(q);
		pthread_cond_broadcast(&cond_work);
		pthread_mutex_unlock(&lock_work);
	}
	return 0;
}
