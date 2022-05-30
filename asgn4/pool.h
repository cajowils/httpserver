#ifndef __POOL_H__
#define __POOL_H__

#include <pthread.h>
#include <sys/epoll.h>
#include "queue.h"
#include "dictionary.h"

typedef struct Pool Pool;

struct Pool {
    Queue *queue;
    Queue *process_queue;
    int running;
    int num_threads;
    pthread_t *threads;
    pthread_mutex_t mutex;
    pthread_mutex_t dict;
    pthread_cond_t cond;
    pthread_cond_t full;
    dictionary *D;

};

void initialze_pool(Pool *p, int num_threads, int queue_size);

int queues_full(Pool *p);

void destruct_pool(Pool *p);

#endif
