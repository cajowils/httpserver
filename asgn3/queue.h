#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "list.h"

typedef struct Queue Queue;

struct Queue {
    QueueNode *head;
    QueueNode *tail;
    int size;
};

Queue *create_queue();

void enqueue(Queue *q, int val);

int dequeue(Queue *q);

int peek(Queue *q);

void delete_queue(Queue *q);

void print_queue(Queue *q);

#endif
