#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "threadpool.h"

#define QUEUE_SIZE 10
#define NUMBER_OF_THREADS 3

typedef struct 
{
    void (*function)(void *p);
    void *data;
}
task;

static task workqueue[QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;
static int queue_count = 0;

static pthread_t workers[NUMBER_OF_THREADS];
static pthread_mutex_t queue_mutex;
static sem_t available_tasks;
static int pool_stopping = 0;

static int enqueue(task t) 
{
    if (queue_count == QUEUE_SIZE) {
        return 1;
    }

    workqueue[queue_tail] = t;
    queue_tail = (queue_tail + 1) % QUEUE_SIZE;
    queue_count++;
    return 0;
}

static task dequeue() 
{
    task t;
    t.function = NULL;
    t.data = NULL;

    if (queue_count == 0) {
        return t;
    }

    t = workqueue[queue_head];
    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count--;
    return t;
}

void *worker(void *param)
{
    (void)param;

    while (1) {
        sem_wait(&available_tasks);
        pthread_mutex_lock(&queue_mutex);

        if (pool_stopping && queue_count == 0) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        task t = dequeue();
        pthread_mutex_unlock(&queue_mutex);

        if (t.function != NULL) {
            execute(t.function, t.data);
        }
    }

    pthread_exit(0);
}

void execute(void (*somefunction)(void *p), void *p)
{
    (*somefunction)(p);
}

int pool_submit(void (*somefunction)(void *p), void *p)
{
    task t;
    int result;

    t.function = somefunction;
    t.data = p;

    pthread_mutex_lock(&queue_mutex);

    if (pool_stopping) {
        pthread_mutex_unlock(&queue_mutex);
        return 1;
    }

    result = enqueue(t);
    pthread_mutex_unlock(&queue_mutex);

    if (result == 0) {
        sem_post(&available_tasks);
    }

    return result;
}

void pool_init(void)
{
    int i;
    queue_head = 0;
    queue_tail = 0;
    queue_count = 0;
    pool_stopping = 0;

    pthread_mutex_init(&queue_mutex, NULL);
    sem_init(&available_tasks, 0, 0);

    for (i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_create(&workers[i], NULL, worker, NULL);
    }
}

void pool_shutdown(void)
{
    int i;

    pthread_mutex_lock(&queue_mutex);
    pool_stopping = 1;
    pthread_mutex_unlock(&queue_mutex);

    for (i = 0; i < NUMBER_OF_THREADS; i++) {
        sem_post(&available_tasks);
    }

    for (i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_join(workers[i], NULL);
    }

    sem_destroy(&available_tasks);
    pthread_mutex_destroy(&queue_mutex);
}
