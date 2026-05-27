#include "schedule_priority.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

// Функция для выбора следующей задачи (Приоритетное планирование)
Task* pickNextTask(struct node **head) {
    if (*head == NULL) {
        return NULL;
    }

    struct node *highestPriorityTaskNode = NULL;
    struct node *current = *head;
    int highestPriority = -1; //Инициализируем минимальным возможным значением приоритета

    // Ищем задачу с наивысшим приоритетом
    while (current != NULL) {
        if (current->task->priority > highestPriority) {
            highestPriority = current->task->priority;
            highestPriorityTaskNode = current;
        }
        current = current->next;
    }

    if (highestPriorityTaskNode == NULL) return NULL; //Это не должно произойти, но лучше добавить проверку

    Task *nextTask = highestPriorityTaskNode->task;
    // Удаляем задачу из списка (аналогично SJF)
    if (highestPriorityTaskNode == *head) {
        *head = highestPriorityTaskNode->next;
    } else {
        struct node *prev = *head;
        while (prev->next != highestPriorityTaskNode) {
            prev = prev->next;
        }
        prev->next = highestPriorityTaskNode->next;
    }
    free(highestPriorityTaskNode);
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

