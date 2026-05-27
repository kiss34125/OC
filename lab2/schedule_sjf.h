#ifndef SCHEDULE_SJF_H
#define SCHEDULE_SJF_H

#include "list.h"
#include "task.h"
#include "cpu.h"

void add(char *name, int priority, int burst);
Task* pickNextTask(struct node **head);
void schedule();

#endif

