#ifndef _TASK_H_
#define _TASK_H_

#include "../main.h"
#include "command.h"

typedef struct Task {
	int modeBG;
	int curRet;
	jid_t curJob;
	int firstly;

	tCmd * root;
	tCmd * cur;
} Task;

void getNextJob(Task *);

Task * newTask(void);
void delTask(Task **);

void echoTask(Task *);
void echoTaskStatus(Task *, int);
void echoTaskWide(Task *);

#endif
