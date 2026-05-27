#include "schedule_fcfs.h"
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
    insert_tail(&head, newTask); // Используем insert_tail для добавления в конец
}

// Функция для выбора следующей задачи (FCFS)
Task* pickNextTask(struct node **head) {
    if (*head == NULL) {
        return NULL;
    }

    struct node *nextNode = *head;
    Task* nextTask = nextNode->task;
    *head = (*head)->next; // Удаляем из головы списка, но НЕ освобождаем память пока
    return nextTask;
}

void schedule() {
    struct node *current = head; // Инициализируем текущий узел для освобождения памяти

    while (head != NULL) {
        Task* nextTask = pickNextTask(&head);
        if (nextTask != NULL) {
            run(nextTask, nextTask->burst); // Вызов функции run() из cpu.c

            // Освобождаем память после выполнения задачи
            free(nextTask->name);
            free(nextTask);

            // Освобождаем память использованного узла
            if(current != NULL) {
              struct node *temp = current;
              current = current->next; // Переходим к следующему узлу
              free(temp);
            }

        }
    }
}


/*#include "schedule_fcfs.h"
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

// Функция для выбора следующей задачи (FCFS)
Task* pickNextTask(struct node **head) {
    if (*head == NULL) {
        return NULL;
    }

    struct node *nextNode = *head;
    Task* nextTask = nextNode->task;
    *head = (*head)->next;
    free(nextNode);
    return nextTask;
}

void schedule() {
    while (head != NULL) {
        Task* nextTask = pickNextTask(&head);
        if (nextTask != NULL) {
            run(nextTask, -1); // Вызов функции run() из cpu.c
            free(nextTask->name);
            free(nextTask);
        }
    }
}*/

