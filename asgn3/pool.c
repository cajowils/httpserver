#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "pool.h"

void initialze_pool(Pool *p, int num_threads) {
    p->running = 1;
    p->queue = create_queue();
    p->num_threads = num_threads;
    p->threads = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);

    pthread_mutex_init(&p->mutex, NULL);
    pthread_cond_init(&p->cond, NULL);

    return;
}

void destruct_pool(Pool *p) {
    p->running = 0;
    delete_queue(p->queue);
    pthread_cond_broadcast(&p->cond);

    for (int i = 0; i < p->num_threads; i++) {
        pthread_join(p->threads[i], NULL);
    }
    free(p->threads);

    pthread_mutex_destroy(&p->mutex);
    pthread_cond_destroy(&p->cond);

    return;
}
