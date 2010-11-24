#ifndef _TASK_H_
#define _TASK_H_

#include "../main.h"
#include "command.h"

typedef struct Task {
	int modeBG;

	tCmd * root;
	tCmd * cur;
} Task;

Task * newTask(void);
void delTask(Task **);

void echoTask(Task *);
void echoTaskWide(Task *);

#endif
