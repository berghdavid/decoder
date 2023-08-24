#ifndef CLIENTS_H
#define CLIENTS_H

#include <netinet/in.h>

typedef struct info info;

/* Used to store all necessary data with a pthread */
struct info
{
	struct sockaddr_in	server_addr;
	int*			data;
	int			id;
};

struct sockaddr_in get_server_addr();

/** Prints the received/sent array of ints.
 * 
 * @param s String which is either "received" or "sent".
 * @param thr_id Int ID of thread.
 * @param buf Array of ints to be printed.
*/
void print_buf(char s[], int id, int buf[]);

/**
 * Used to fill an array with integers before sending.
 * 
 * @param buf Array to be filled.
 * @param data Array to copy from. Size of array must
 * be greater than the given 'index' + 'BUF_SIZE'
 * @param index Determines the starting index in *buf 
 * from which the integers will be copied from.
 */
int fill_buf(int* buf, int data[], int index);

void* send_data(void* socket_desc);

int main();

#endif
