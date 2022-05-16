#include "pool.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void initialze_pool(Pool *p, int num_threads) {
    p->running = 1;
    p->queue = create_queue();
    p->num_threads = num_threads;
    p->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    pthread_mutex_init(&p->mutex, NULL);
    pthread_cond_init(&p->cond, NULL);
    
    return;
}

void cleanup_pool(Pool *p) {
    p->running = 0;
    delete_queue(p->queue);
    pthread_cond_broadcast(&p->cond);

    /*for (int i=0;i<p->num_threads;i++) {
        pthread_mutex_unlock(&p->mutex);
        pthread_cond
    }*/

    for (int i=0;i<p->num_threads;i++) {
        pthread_join(p->threads[i], NULL);
    }
    free(p->threads);

    return;

}
