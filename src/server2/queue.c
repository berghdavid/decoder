#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "queue.h"

queue* queue_create(void)
{
	struct queue *q;

	q = (struct queue *) malloc(sizeof(struct queue));
	q->head = q->tail = NULL;

	if(pthread_mutex_init(&q->lock, NULL) != 0) {
		printf("Error initializing queue lock.");
	}

	return q;
}

void enq(queue* q, int value)
{
	node*	e;

	e = malloc(sizeof(node));
	e->value = value;
	e->next = NULL;

	pthread_mutex_lock(&q->lock);
	if (q->head == NULL) {
		q->head = e;
	} else {
		q->tail->next = e;
	}
	q->tail = e;
	pthread_mutex_unlock(&q->lock);
}

int queue_empty(const queue* q)
{
	return q->head == NULL;
}

int deq(queue *q)
{
	int	ret;
	node*	e;

	pthread_mutex_lock(&q->lock);
	ret = q->head->value;

	e = q->head;
	q->head = e->next;
	free(e);
	pthread_mutex_unlock(&q->lock);
	return ret;
}

void queue_print(queue* q)
{
	node*	e;

	pthread_mutex_lock(&q->lock);
	e = q->head;
	while (e != NULL) {
		printf("%d ", e->value);
		e = e->next;
	}
	printf("\n");
	pthread_mutex_unlock(&q->lock);
}

void queue_destroy(queue *q)
{
	pthread_mutex_lock(&q->lock);
	while (!queue_empty(q)) {
		deq(q);
	}
	free(q);
	pthread_mutex_unlock(&q->lock);
}
