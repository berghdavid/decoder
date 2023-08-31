#ifndef QUEUE_H
#define QUEUE_H

typedef struct node node;
typedef struct queue queue;

/**
 * @brief Fifo list element
 */
struct node {
	node*	next;
	int	value;
};

/**
 * @brief Fifo queue
 */
struct queue {
	pthread_mutex_t	lock;	/* Mutex for multithreaded queue access	*/
	node*		head;	/* Dequeue this next			*/
	node*		tail;	/* Enqueue after this			*/
};

/* Create a new empty queue */
queue* queue_create(void);

/* Add a new value to back of queue */
void enq(queue* q, int value);

int queue_empty(const queue* q);

/* Remove and return value from front of queue */
int deq(queue* q);

/* Print contents of queue on a single line, head first */
void queue_print(queue* q);

/* Free a queue and all of its elements */
void queue_destroy(queue* q);

#endif
