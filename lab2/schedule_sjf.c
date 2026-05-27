#include "schedule_sjf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> // Для INT_MAX


struct node *head = NULL; // Инициализация глобального head

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

    insert(&head, newTask);
}

// Функция для выбора следующей задачи (SJF)
Task* pickNextTask(struct node **head) {
    if (*head == NULL) {
        return NULL;
    }

    struct node *shortestTaskNode = NULL;
    struct node *current = *head;
    int shortestBurst = INT_MAX;

    // Ищем задачу с наименьшим burst
    while (current != NULL) {
        if (current->task->burst < shortestBurst) {
            shortestBurst = current->task->burst;
            shortestTaskNode = current;
        }
        current = current->next;
    }

    if (shortestTaskNode == NULL) return NULL; //Это не должно произойти, но лучше добавить проверку

    Task *nextTask = shortestTaskNode->task;
    // Удаляем задачу из списка
    if (shortestTaskNode == *head) {
        *head = shortestTaskNode->next;
    } else {
        struct node *prev = *head;
        while (prev->next != shortestTaskNode) {
            prev = prev->next;
        }
        prev->next = shortestTaskNode->next;
    }
    free(shortestTaskNode);
    return nextTask;
}

void schedule() {
    while (head != NULL) {
        Task* nextTask = pickNextTask(&head);
        if (nextTask != NULL) {
            run(nextTask, nextTask->burst);
            free(nextTask->name);
            free(nextTask);
        }
    }
}

