#ifndef CLIENTS_H
#define CLIENTS_H

#include <netinet/in.h>
#include "../utils/queue.h"

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
	int		buf_s;	/* Maximum buffer size (nbr of chars)	*/
};

/** Stores all necessary data for worker thread. */
struct Worker {
	Client*		client;	/* Points to original client	*/
	Sockaddr*	addr;	/* Open socket for sending data	*/
	socklen_t	addr_s;	/* Length of peer address	*/
	Queue*		q;	/* Queue containing test strings	*/
	int		socket;	/* Connected socket descriptor	*/
	int		id;	/* Worker id			*/
	char*		buf_rc;	/* Buffer for receiving data	*/
	char*		buf_sd;	/* Buffer for sending data	*/
};

int main(int argc, char* argv[]);

#endif
