#ifndef __POOL_H__
#define __POOL_H__

#include "queue.h"
#include <pthread.h>

typedef struct Pool Pool;

struct Pool {
    Queue *queue;
    int running;
    int num_threads;
    pthread_t *threads;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

void initialze_pool(Pool *p, int num_threads);

void destruct_pool(Pool *p);

#endif
