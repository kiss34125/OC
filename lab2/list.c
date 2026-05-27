/**
 * Various list operations
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "task.h"


// add a new task to the list of tasks
void insert(struct node **head, Task *newTask) {
    // add the new task to the list 
    struct node *newNode = malloc(sizeof(struct node));

    newNode->task = newTask;
    newNode->next = *head;
    *head = newNode;
}

// delete the selected task from the list
void delete(struct node **head, Task *task) {
    struct node *temp;
    struct node *prev;

    temp = *head;
    // special case - beginning of list
    if (strcmp(task->name,temp->task->name) == 0) {
        *head = (*head)->next;
    }
    else {
        // interior or last element in the list
        prev = *head;
        temp = temp->next;
        while (strcmp(task->name,temp->task->name) != 0) {
            prev = temp;
            temp = temp->next;
        }

        prev->next = temp->next;
    }
}

// traverse the list
void traverse(struct node *head) {
    struct node *temp;
    temp = head;

    while (temp != NULL) {
        printf("[%s] [%d] [%d]\n",temp->task->name, temp->task->priority, temp->task->burst);
        temp = temp->next;
    }
}

void insert_tail(struct node **head, Task *newTask) {
    struct node *newNode = malloc(sizeof(struct node));
    if (newNode == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    newNode->task = newTask;
    newNode->next = NULL;

    if (*head == NULL) {
        *head = newNode;
    } else {
        struct node *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}


void insert_priority(struct node **head, Task *task) {
    struct node *newNode = (struct node*)malloc(sizeof(struct node));
    newNode->task = task;
    newNode->next = NULL;

    if (*head == NULL || task->priority > (*head)->task->priority) { // Изменили знак >
        newNode->next = *head;
        *head = newNode;
    } else {
        struct node *current = *head;
        while (current->next != NULL && current->next->task->priority >= task->priority) { // Изменили знак >=
            //Если приоритеты равны, то вставляем в порядке очереди
            if(current->next->task->priority == task->priority){
                 while(current->next != NULL && current->next->task->priority == task->priority) {
                        current = current->next;
                 }
                 break;
            }
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }
}
