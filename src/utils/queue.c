#include <stdlib.h>
#include <string.h>
#include "queue.h"

Queue* queue_create(int str_s)
{
	Queue*	q;

	q = malloc(sizeof(Queue));
	q->head = q->tail = NULL;
	q->str_s = str_s;
	q->size = 0;
	return q;
}

void enq(Queue* q, char* value)
{
	Node*	e;

	e = malloc(sizeof(Node));
	e->value = calloc(q->str_s, sizeof(char));
	strcpy(e->value, value);
	e->next = NULL;

	if (q->head == NULL) {
		q->head = e;
	} else {
		q->tail->next = e;
	}
	q->tail = e;
	q->size++;
}

int queue_empty(const Queue* q)
{
	return q->head == NULL;
}

char* deq(Queue *q)
{
	char*	ret;
	Node*	e;

	if (q->head != NULL) {
		ret = q->head->value;
	} else {
		return NULL;
	}
	e = q->head;
	q->head = e->next;
	free(e);
	q->size--;
	return ret;
}

void queue_destroy(Queue *q)
{
	while (!queue_empty(q)) {
		free(deq(q));
	}
	free(q);
}
