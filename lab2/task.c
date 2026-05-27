#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Task *create_task(const char *name, int priority, int burst, int arrival_order) {
    Task *task = (Task *)malloc(sizeof(Task));
    static int next_tid = 1;

    if (task == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    task->tid = next_tid++;
    strncpy(task->name, name, TASK_NAME_MAX - 1);
    task->name[TASK_NAME_MAX - 1] = '\0';
    task->priority = priority;
    task->burst = burst;
    task->remaining = burst;
    task->arrival_order = arrival_order;

    return task;
}

void destroy_task(Task *task) {
    free(task);
}
