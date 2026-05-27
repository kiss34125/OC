#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>


#define MAX_THREADS 10
#define QUEUE_SIZE 20

typedef struct {
    void (*function)(void* p);
    void* data;
} task_t;

typedef struct {
    task_t queue[QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    sem_t semaphore;
} thread_pool_t;


thread_pool_t thread_pool;
pthread_t threads[MAX_THREADS];


void* worker(void* arg);
int enqueue(task_t task);
task_t dequeue();


int pool_init() {
    thread_pool.head = 0;
    thread_pool.tail = 0;
    thread_pool.count = 0;

    if (pthread_mutex_init(&thread_pool.mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        return 1;
    }

    if (sem_init(&thread_pool.semaphore, 0, 0) != 0) {
        perror("Semaphore initialization failed");
        pthread_mutex_destroy(&thread_pool.mutex);
        return 1;
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
            perror("Thread creation failed");

            for (int j = 0; j < i; j++) {
                pthread_cancel(threads[j]);
                pthread_join(threads[j], NULL);
            }

            pthread_mutex_destroy(&thread_pool.mutex);
            sem_destroy(&thread_pool.semaphore);
            return 1;
        }
    }
    return 0;
}

int pool_submit(void (*function)(void* p), void* p) {
    task_t task;
    task.function = function;
    task.data = p;

    if (enqueue(task) != 0) {
        return 1;
    }

    sem_post(&thread_pool.semaphore);
    return 0;
}

void pool_shutdown() {
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_cancel(threads[i]);
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL); 
    }

    pthread_mutex_destroy(&thread_pool.mutex);
    sem_destroy(&thread_pool.semaphore);

    printf("Thread pool shutdown complete.\n");
}


void* worker(void* arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while (1) {
        sem_wait(&thread_pool.semaphore);

        pthread_testcancel();

        pthread_mutex_lock(&thread_pool.mutex);
        task_t task = dequeue();
        pthread_mutex_unlock(&thread_pool.mutex);

        task.function(task.data);
    }
    return NULL;
}

int enqueue(task_t task) {
    pthread_mutex_lock(&thread_pool.mutex);

    if (thread_pool.count == QUEUE_SIZE) {
        pthread_mutex_unlock(&thread_pool.mutex);
        return 1;
    }

    thread_pool.queue[thread_pool.tail] = task;
    thread_pool.tail = (thread_pool.tail + 1) % QUEUE_SIZE;
    thread_pool.count++;

    pthread_mutex_unlock(&thread_pool.mutex);
    return 0; 
}

task_t dequeue() {
    task_t task;

    if (thread_pool.count == 0) {
        task.function = NULL;
        task.data = NULL;
        return task; 
    }

    task = thread_pool.queue[thread_pool.head];
    thread_pool.head = (thread_pool.head + 1) % QUEUE_SIZE;
    thread_pool.count--;

    return task;
}

void my_task(void* arg) {
    int task_id = *(int*)arg;
    printf("Thread %lu: Executing task %d\n", pthread_self(), task_id);
    sleep(1); 
}


int main() {
    int num_tasks = 10;

    if (pool_init() != 0) {
        fprintf(stderr, "Thread pool initialization failed.\n");
        return 1;
    }

    int* task_ids = malloc(num_tasks * sizeof(int));
    if (task_ids == NULL) {
        perror("Failed to allocate task IDs");
        pool_shutdown();
        return 1;
    }

    for (int i = 0; i < num_tasks; i++) {
        task_ids[i] = i;
        if (pool_submit(my_task, &task_ids[i]) != 0) {
            fprintf(stderr, "Failed to submit task %d\n", i);
        }
    }

    sleep(5);

    pool_shutdown();

    free(task_ids);
    return 0;
}