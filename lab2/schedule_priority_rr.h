#ifndef SCHEDULE_PRIORITY_RR_H
#define SCHEDULE_PRIORITY_RR_H

#include "list.h"

// Функция выбора следующей задачи для Priority Round Robin
Task* priority_rr_pickNextTask(Task** head, int quantum); // quantum - квант времени

#endif

