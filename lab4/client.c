#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "threadpool.h"

struct data
{
    int a;
    int b;
};

void add(void *param)
{
    struct data *temp;
    temp = (struct data*)param;

    printf("thread %lu: %d + %d = %d\n",
           (unsigned long)pthread_self(),
           temp->a,
           temp->b,
           temp->a + temp->b);
    free(temp);
}

int main(void)
{
    int i;

    pool_init();

    for (i = 1; i <= 20; i++) {
        struct data *work = (struct data*)malloc(sizeof(struct data));
        if (work == NULL) {
            printf("memory allocation error\n");
            break;
        }

        work->a = i;
        work->b = i * 10;

        while (pool_submit(&add, work) != 0) {
            usleep(10000);
        }
    }

    pool_shutdown();
    return 0;
}
