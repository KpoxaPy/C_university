#ifndef _RUN_H_
#define _RUN_H_

#include "../main.h"
#include "task.h"
#include "internals.h"

int checkInternalTask(Task *);
void runTask(Task *);
void checkTasks(void);

#endif
