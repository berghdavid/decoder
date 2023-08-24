#ifndef SERVER_H
#define SERVER_H

typedef struct Server Server;
typedef struct Worker Worker;
typedef struct sockaddr Sockaddr;

#include "queue.h"

/** Contains relevant server information */
struct Server {
	Worker**	worker;	/* Points to array of worker pointers	*/
	char*		host;	/* For example '127.0.0.1'		*/
	int		work_s;	/* Number of workers			*/
	int		port;	/* For example 5142			*/
	int		pend;	/* Max number of pending connections	*/	
	int		socket;	/* Socket connection			*/	
	int		buf_s;	/* Maximum buffer size (nbr of chars)	*/
};

/** Stores all necessary data for worker thread. */
struct Worker {
	Server*		server;	/* Points to original server	*/
	Sockaddr*	addr;	/* Open socket for sending data	*/
	socklen_t	addr_s;	/* Length of peer address	*/
	int		socket;	/* Connected socket descriptor	*/
	int		id;	/* Worker id			*/
	char*		buf;	/* Buffer for receiving data	*/
};

/**
 * Sorts and returns an array using insertion sort. 
 * Insertion sort is fast and easy for small arrays.
 */
int* sort_array();

/** Prints the received/sent array of ints.
 * 
 * @param s String which is either "received" or "sent".
 * @param thr_id Int ID of thread.
 * @param buf Array of ints to be printed.
*/
void print_buf(char s[], int thr_id, int buf[]);

Server* init_server(int argc, char* argv[]);

/** 
 * Waits for a client to connect to, then communicates
 * with the connected client.
 */
void* wait_client(void* arg);

/**
 * Reads user input and toggles quit to true if 'exit' is entered.
 * This joins all the threads and shutdowns the server.
 */
void* shutdown_server(void* arg);

int main();

#endif
