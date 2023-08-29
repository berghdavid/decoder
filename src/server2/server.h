#ifndef SERVER_H
#define SERVER_H

typedef struct Server Server;
typedef struct Worker Worker;
typedef struct sockaddr Sockaddr; /* Built in from <netinet/in.h> */

#include "queue.h"
#include "parser.h"

/** Contains relevant server information */
struct Server {
	Worker**	worker;	/* Points to array of worker pointers	*/
	char*		host;	/* For example '127.0.0.1'		*/
	int		work_s;	/* Number of workers			*/
	int		port;	/* For example 5142			*/
	int		pend;	/* Max number of pending connections	*/	
	int		socket;	/* Socket connection			*/	
	int		buf_s;	/* Maximum buffer size (nbr of chars)	*/
	int		reuse;	/* Reuse the same port			*/
};

/** Stores all necessary data for worker thread. */
struct Worker {
	Server*		server;	/* Points to original server	*/
	Sockaddr*	addr;	/* Open socket for sending data	*/
	socklen_t	addr_s;	/* Length of peer address	*/
	Data*		data;	/* Received package data	*/
	int		socket;	/* Connected socket descriptor	*/
	int		id;	/* Worker id			*/
	char*		buf_rc;	/* Buffer for receiving data	*/
	char*		buf_sd;	/* Buffer for sending data	*/
};

void close_server(Server* server);

Server* init_server(int argc, char* argv[]);

/** 
 * Waits for a client to connect to, then communicates
 * with the connected client.
 */
void* wait_client(void* arg);

int main(int argc, char* argv[]);

#endif
