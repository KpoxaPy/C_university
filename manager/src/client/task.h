#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "lexlist.h"
#include "main.h"

#define TASK_UNKNOWN -1
#define TASK_GET 0
#define TASK_SET 1
#define TASK_JOIN 2
#define TASK_LEAVE 3
#define TASK_NICK 4

typedef struct task {
	int argc;
	char ** argv;

	int type;
} Task;

/* Support functions for command */
/* This function generates argv from list of
 * lexems.
 *
 * ATTENTION! After execution list will be
 * free. Strings that were in list of lexems
 * remain in memory, links on them will be
 * in vector of arguments.
 */
int genArgVector(Task *, lexList *);

/* Command actions */
Task * newTask();
void delTask(Task **);

char * getTaskString(Task *);

#endif
