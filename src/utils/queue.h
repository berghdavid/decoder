#ifndef QUEUE_H
#define QUEUE_H

typedef struct Node Node;
typedef struct Queue Queue;

/**
 * @brief Fifo list element
 */
struct Node {
	Node*	next;
	char*	value;
};

/**
 * @brief Fifo queue
 */
struct Queue {
	Node*	head;	/* Dequeue this next	*/
	Node*	tail;	/* Enqueue after this	*/
	int	str_s;	/* Size of strings	*/
	int	size;	/* Total queue size	*/
};

/* Create a new empty queue */
Queue* queue_create(int str_s);

/* Add a new value to back of queue */
void enq(Queue* q, char* value);

int queue_empty(const Queue* q);

/* Remove and return value from front of queue */
char* deq(Queue* q);

/* Free a queue and all of its elements */
void queue_destroy(Queue* q);

#endif
