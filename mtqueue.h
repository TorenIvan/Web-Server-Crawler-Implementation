#ifndef MTQUEUE_H
#define MTQUEUE_H

typedef struct mtqueue mtqueue_t;

typedef struct {
    int readers;
    int writers;
	int max_workers;
    int size;
    int count;
} mtq_info_t;

#define MTQ_OK 0
#define MTQ_DEADLOCK -1
#define MTQ_TERMINATED -2

mtqueue_t* mtq_init(int size, int max_workers);
void mtq_destroy(mtqueue_t*);
mtq_info_t mtq_info(mtqueue_t*);

int mtq_put(mtqueue_t* q, void* data);
int mtq_get(mtqueue_t* q, void** data);

int mtq_has_terminated(mtqueue_t* q);
void mtq_terminate(mtqueue_t* q);
#endif
