#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "queue.h"

Queue *create_queue(int capacity) {
    Queue *q = malloc(sizeof(Queue));
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->capacity = capacity;
    return q;
}

void enqueue(Queue *q, int val) {
    if (q->size > 0) {
        q->tail->next = create_queue_node(val);
        q->tail = q->tail->next;
    } else {
        q->head = create_queue_node(val);
        q->tail = q->head;
    }
    q->size++;
    //print_queue(q);
    return;
}

int dequeue(Queue *q) {
    if (q->size <= 0 || !q->head) {
        return -1;
    }
    int val = q->head->val;
    QueueNode *tmp = q->head;
    if (q->head->next) {
        q->head = q->head->next;
    }
    delete_queue_node(tmp);
    q->size--;
    if (q->size == 0) {
        q->tail = NULL;
        q->head = NULL;
    }
    //print_queue(q);
    return val;
}

int peek(Queue *q) {
    if (q->size <= 0 || !q->head) {
        return -1;
    }
    return q->head->val;
}

void delete_queue(Queue *q) {
    if (q) {
        if (q->head) {
            QueueNode *ptr = q->head;
            QueueNode *next;
            do {
                next = ptr->next;
                delete_queue_node(ptr);
                ptr = next;
            } while (next != NULL);
        }
        q->tail = NULL;
        q->head = NULL;
        free(q);
        q = NULL;
    }
}

void print_queue(Queue *q) {
    if (q) {
        if (q->head) {
            QueueNode *ptr = q->head;
            printf("Queue: ");
            do {
                printf("%d ", ptr->val);
                ptr = ptr->next;
            } while (ptr != NULL);
            printf("\n");
        }
    }
}

int full(Queue *q) {
    if (q->size >= q->capacity) {
        return 1;
    }
    return 0;
}

int empty(Queue *q) {
    if (q->size == 0) {
        return 1;
    }
    return 0;
}

/*

testqueue:		queue.o list.o
				$(CC) -o testqueue queue.o list.o

int main() {
    //time_t t;
    //srand((unsigned) time(&t));
    Queue *q = create_queue();
    for (int j=0;j<3;j++) {
        printf("Enqueuing:\n");
        for (int i=0;i<5;i++) {
            enqueue(q, rand() % 50);
        }
        print_queue(q);
        printf("Dequeuing:\n");
        for (int i=0; i<3;i++) {
            dequeue(q);
        }
        print_queue(q);
    }
    delete_queue(q);
}
*/
