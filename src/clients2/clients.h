#ifndef CLIENTS_H
#define CLIENTS_H

#include <netinet/in.h>

typedef struct Client Client;
typedef struct Worker Worker;
typedef struct sockaddr_in SockaddrIn;
typedef struct sockaddr Sockaddr;

/** Contains relevant client information */
struct Client {
	Worker**	worker;	/* Points to array of worker pointers	*/
	SockaddrIn	sockad;	/* Points to array of worker pointers	*/
	pthread_t*	thr;	/* List of active threads		*/
	char*		host;	/* For example '127.0.0.1'		*/
	int		work_s;	/* Number of workers			*/
	int		port;	/* For example 5124			*/
	int		socket;	/* Socket connection			*/
	int		buf_s;	/* Maximum buffer size (nbr of chars)	*/
};

/** Stores all necessary data for worker thread. */
struct Worker {
	Client*		client;	/* Points to original client	*/
	Sockaddr*	addr;	/* Open socket for sending data	*/
	socklen_t	addr_s;	/* Length of peer address	*/
	int		socket;	/* Connected socket descriptor	*/
	int		id;	/* Worker id			*/
	char*		buf_rc;	/* Buffer for receiving data	*/
	char*		buf_sd;	/* Buffer for sending data	*/
};

SockaddrIn get_server_addr();

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
