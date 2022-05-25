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

void requeue(Queue *q, QueueNode *qn) {
    if (q->size > 0) {
        q->tail->next = qn;
        q->tail = q->tail->next;
    } else {
        q->head = qn;
        q->tail = q->head;
    }
    q->size++;
    return;
}

void enqueue(Queue *q, int connfd) {
    if (q->size > 0) {
        q->tail->next = create_queue_node(connfd);
        q->tail = q->tail->next;
    } else {
        q->head = create_queue_node(connfd);
        ;
        q->tail = q->head;
    }
    q->size++;
    //print_queue(q);
    return;
}

QueueNode *dequeue(Queue *q) {
    if (!q || q->size <= 0 || !q->head) {
        return NULL;
    }
    QueueNode *qn = q->head;
    if (q->head->next) {
        q->head = q->head->next;
    }
    q->size--;
    if (q->size == 0) {
        q->tail = NULL;
        q->head = NULL;
    }
    //print_queue(q);
    return qn;
}

QueueNode *find(Queue *q, int connfd) {
    if (!q || q->size <= 0 || !q->head) {
        return NULL;
    }
    QueueNode *ptr = q->head;
    QueueNode *prev = NULL;
    while (ptr != NULL) {
        if (ptr->val == connfd) {
            //delete ptr from queue
            if (prev == NULL) {
                q->head = ptr->next;
            } else {
                prev->next = ptr->next;
            }
            if (ptr->next == NULL) {
                q->tail = prev;
            }
            ptr->next = NULL;

            q->size--;
            return ptr;
        }
        prev = ptr;
        ptr = ptr->next;
    }
    return NULL;
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
    Queue *q = create_queue(128);
    Queue *q2 = create_queue(128);


    printf("Enqueuing:\n");
    for (int i=0;i<100000;i++) {
        enqueue(q, rand() % 5000);
    }
    print_queue(q);
    print_queue(q2);
    printf("Removing:\n");
    QueueNode *qn;
    for (int i=0; i<10000;i++) {
        if((qn = find(q, i))==NULL) {
            enqueue(q2, i);
        }
        else {
            printf("Requeing\n");
            requeue(q2, qn);
        }
        
    }
    print_queue(q);
    print_queue(q2);

    
}*/
