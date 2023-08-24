#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include "clients.h"

#define PORT 5142	/* Use any available port			*/
#define BUF_SIZE 5	/* Size of buffer array				*/
#define DATA_SIZE 10	/* Total size of client array			*/
#define T_SIZE 3	/* Number of threads handling clients (N)	*/

pthread_mutex_t print_l = PTHREAD_MUTEX_INITIALIZER;

struct sockaddr_in get_server_addr()
{
	struct sockaddr_in server_addr;
	memset(&server_addr, '\0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	return server_addr;
}

void print_buf(char s[], int id, int buf[])
{
	int	i;
	
	pthread_mutex_lock(&print_l);
	printf("Thread %d %s: { ", id, s);
	for (i = 0; i < BUF_SIZE; i++) {
		printf("%d ", buf[i]);
	}
	printf("}\n");
	pthread_mutex_unlock(&print_l);
}

int fill_buf(int* buf, int data[], int index)
{
	int	i;

	for (i = 0; i < BUF_SIZE; i++) {
		*buf = data[index + i];
		buf++;
	}
	return 0;
}

void* send_data(void* arg)
{
	info*	t_info;
	int	buf[BUF_SIZE];
	int	i;
	int	cl_socket;
	
	t_info = (info*) arg;
	cl_socket = socket(PF_INET, SOCK_STREAM, 0);

	if (connect(cl_socket, (struct sockaddr*) &t_info->server_addr, 
		sizeof(t_info->server_addr)) != 0) {
		printf("Unsuccessful connection...\n");
		exit(0);
	}

	for (i = 0; i < DATA_SIZE; i += BUF_SIZE) {
		fill_buf(&buf[0], t_info->data, i);
		send(cl_socket, buf, sizeof(int) * BUF_SIZE, 0);
		print_buf("sent", t_info->id, buf);
		recv(cl_socket, buf, sizeof(int) * BUF_SIZE, 0);
		print_buf("received", t_info->id, buf);
	}
	return NULL;
}

int main()
{
	int data[T_SIZE][DATA_SIZE] = {
		{7, 5, 4, 1, 9, 8, 9, 7, 5, 4},
		{10, 50, 20, 10, 25, 76, 89, 20, 20, 50},
		{500, 400, 400, 100, 100, 700, 500, 400, 100, 900}
	};
	struct sockaddr_in	server_addr;
	pthread_t		thr[T_SIZE];
	info*			inf;
	int			i;

	/* Start 'T_SIZE' different client threads */
	server_addr = get_server_addr();
	for (i = 0; i < T_SIZE; i++) {
		inf = malloc(sizeof(info));
		inf->id = i;
		inf->data = &data[i][0];
		inf->server_addr = server_addr;
		pthread_create(&thr[i], NULL, send_data, inf);
	}

	/* Wait for all threads, then join them when finished */
	for (i = 0; i < T_SIZE; i++) {
		if (pthread_join(thr[i], NULL) != 0) {
			printf("Could not join with thread %d\n", i);
		}
	}
	return 0;
}
