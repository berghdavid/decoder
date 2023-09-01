#ifndef SERVER_H
#define SERVER_H

typedef struct Server Server;
typedef struct Worker Worker;
typedef struct sockaddr Sockaddr; /* Built in from <netinet/in.h> */
typedef struct sockaddr_in Sokadin; /* Built in from <netinet/in.h> */
typedef struct curl_slist CurlSlist; /* Built from <curl/curl.h> */

#include <curl/curl.h>
#include "../utils/parser.h"

/** Contains relevant server information */
struct Server {
	CURL*		curl;	/* Curl handle				*/
	CurlSlist*	slist;	/* Slist headers for curl		*/
	Worker**	worker;	/* Points to array of worker pointers	*/
	char*		host;	/* For example '127.0.0.1'		*/
	char*		forwrd;	/* Curl forwarding address		*/
	int		work_s;	/* Number of workers			*/
	int		port;	/* For example 5124			*/
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

/**
 * @brief Appends a key-value pair to where the cur* points to. Will only append
 * until the end* pointer and no further to prevent mem-leaks.
 * 
 * Returns how many chars were appended.
 */
int concat_json(char* cur, const char* end, char* key, char* val);

/**
 * @brief Builds the json which will be forwarded according to the format below:
 * 
 * {
 * 	"data": {
 * 		"signals": {
 * 			"param1": "value1",
 * 			"param2": "value2",
 * 			...
 * 		}
 * 	}
 * }
 * 
 */
void build_forward_req(Worker* data);

void reset_data(Worker* w);

Server* init_server(int argc, char* argv[]);

/**
 * ##<pack-len>,<ID>,<work-no>,A03,<date-time>\r\n
 */
void build_response(Worker* w);

int main(int argc, char* argv[]);

#endif
