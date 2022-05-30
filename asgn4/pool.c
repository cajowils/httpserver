#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>

#include "pool.h"

//intitalizes an existing pool (for use with global variables)

//sets running to True, creates the work queue and the queue of pending connections

//intitalizes the threads and lock and cond var

void initialze_pool(Pool *p, int num_threads, int queue_size) {
    p->running = 1;
    p->queue = create_queue(queue_size);
    p->process_queue = create_queue(queue_size);
    p->num_threads = num_threads;
    p->threads = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);

    pthread_mutex_init(&p->mutex, NULL);
    pthread_mutex_init(&p->dict, NULL);

    pthread_cond_init(&p->cond, NULL);
    //pthread_cond_init(&p->full, NULL);

    return;
}

//checks whether both of the queues are at capacity

int queues_full(Pool *p) {
    if (p->queue->size + p->process_queue->size == p->queue->capacity) {
        return 1;
    }
    return 0;
}

//stops the threads by setting running to 0 and broadcasting all of the threads to be joined/freed

//frees/destroys all queues and locks/cond vars

void destruct_pool(Pool *p) {
    p->running = 0;
    pthread_cond_broadcast(&p->cond);

    for (int i = 0; i < p->num_threads; i++) {
        pthread_join(p->threads[i], NULL);
    }
    free(p->threads);
    delete_queue(p->queue);
    delete_queue(p->process_queue);

    pthread_mutex_destroy(&p->mutex);
    pthread_mutex_destroy(&p->dict);
    pthread_cond_destroy(&p->cond);

    return;
}
