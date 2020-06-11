#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "mtqueue.h"

struct mtqueue {
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    pthread_mutex_t mtx;

    int start; /* position to "read" */
    int end; /* position to "write" */
    int count; /* the current number of elements in the queue */
    int size; /* the maximum number of elements in the queue */

    int readers; /* The number of threads waiting to get elements from the queue */
    int writers; /* the current number of threads waiting to "write" ie. to put an element */
    int max_workers; /*  When max_writers == writers, all threads are waiting to "write"
                        from the queue and no-one will "read". Therefore, we should
                        terminate, otherwise we have a deadlock.. */

	int terminated; /* When a deadlock was detected or someone calls the terminate function */
    void** data;
};


mtqueue_t* mtq_init(int size, int max_workers){
    mtqueue_t* q = calloc(sizeof(mtqueue_t), 1);
    if (q == NULL)
        return NULL;
    q->data = calloc(sizeof(void*), size);
    if (q->data == NULL) {
        free(q);
        return NULL;
    }
    q->size = size;
    q->max_workers = max_workers;
    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);

    return q;
}

void mtq_destroy(mtqueue_t* q){
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
    pthread_mutex_destroy(&q->mtx);
    free(q->data);
    free(q);
}

mtq_info_t mtq_info(mtqueue_t* q){

    mtq_info_t info;
    pthread_mutex_lock(&q->mtx);
    info.readers = q->readers;
    info.writers = q->writers;
	info.max_workers = q->max_workers;
    info.size = q->size;
    info.count = q->count;
    pthread_mutex_unlock(&q->mtx);
    return info;
}

int mtq_has_terminated(mtqueue_t* q){
	int t;
    pthread_mutex_lock(&q->mtx);
	t = q->terminated;
    pthread_mutex_unlock(&q->mtx);
	return t;
}
void mtq_terminate(mtqueue_t* q){
    pthread_mutex_lock(&q->mtx);
	   q->terminated = 1;
	    pthread_cond_broadcast(&q->not_empty);//gia na ksekolisoun oti threads exoun kolisei
	    pthread_cond_broadcast(&q->not_full);//gia na ksekolisoun oti threads exoun kolisei
    pthread_mutex_unlock(&q->mtx);
}


int mtq_put(mtqueue_t* q, void* data){
    int status = MTQ_OK;
    unsigned int tid = (unsigned int) pthread_self();
    pthread_mutex_lock(&q->mtx);
    ++q->writers;
    while (!q->terminated && q->count == q->size && q->writers < q->max_workers)
        pthread_cond_wait(&q->not_full, &q->mtx);

	if (q->terminated)
		status = MTQ_TERMINATED;
	else if (q->count < q->size) {
        q->data[q->end] = data;
        q->end = (q->end+1) % q->size;
        ++q->count;
        --q->writers;
        pthread_cond_signal(&q->not_empty);
    }
    else {
        /* Here q->writers == q->max_workers and all threads are waiting to write
         * to a full queue. Therefore we have a deadlock, since no-one will
         * remove an element.. So we are going to return an error (MTQ_DEADLOCK)
         * and also wake-up all other "writers" to terminate too */
        printf("MTQ[%u]: DEADLOCK detected, %d writers (of max %d) waiting in a full queue!!\n",
                tid, q->writers, q->max_workers);
		q->terminated = 1;
        pthread_cond_broadcast(&q->not_full);
        status = MTQ_DEADLOCK;
    }
    pthread_mutex_unlock(&q->mtx);
    return status;
}

int mtq_get(mtqueue_t* q, void** data){
    int status = MTQ_OK;
	   *data = NULL;

    pthread_mutex_lock(&q->mtx);
    ++q->readers;
    while (!q->terminated && q->count == 0)
        pthread_cond_wait(&q->not_empty, &q->mtx);

	if (q->terminated)
		status = MTQ_TERMINATED;
	else {
        *data = q->data[q->start];
        q->start = (q->start+1) % q->size;
        --q->count;
        --q->readers;
        pthread_cond_signal(&q->not_full);
    }
    pthread_mutex_unlock(&q->mtx);

    return status;
}
