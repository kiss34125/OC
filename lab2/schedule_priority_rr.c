#include "schedule_priority_rr.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node *head = NULL;          // Голова очереди
int quantum = 10;                 // Квант времени

void add(char *name, int priority, int burst) {
    Task *newTask = (Task *)malloc(sizeof(Task));
    if (newTask == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    newTask->name = strdup(name);
    if (newTask->name == NULL) {
        perror("Memory allocation failed");
        free(newTask);
        exit(EXIT_FAILURE);
    }
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->remaining_burst = burst;
    insert_priority(&head, newTask); // Добавляем с учетом приоритета
}

// Функция для выбора следующей задачи (RR с приоритетом)
Task* pickNextTask(struct node **head) {
    if (*head == NULL) {
        return NULL;
    }
    struct node *current = *head;
    Task *nextTask = current->task;
    *head = current->next;
    free(current);
    return nextTask;
}

void schedule() {
    Task *nextTask;
    while (head != NULL) {
        nextTask = pickNextTask(&head);
        if (nextTask != NULL) {
            int run_time = (nextTask->remaining_burst > quantum) ? quantum : nextTask->remaining_burst;

            run(nextTask, run_time);
            nextTask->remaining_burst -= run_time;

            if (nextTask->remaining_burst > 0) {
                insert_priority(&head, nextTask); // Возвращаем с учетом приоритета
            } else {
                free(nextTask->name);
                free(nextTask);
            }
        }
    }
}


