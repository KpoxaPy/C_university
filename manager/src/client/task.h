#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "lexlist.h"
#include "main.h"

#define TASK_UNKNOWN -1

#define TASKT_SERVER 64
#define TASK_GET 0
#define TASK_SET 1
#define TASK_JOIN 2
#define TASK_LEAVE 3
#define TASK_NICK 4
#define TASK_ADM 5
#define TASK_GAMES 6
#define TASK_PLAYERS 7
#define TASK_CREATEGAME 8
#define TASK_DELETEGAME 9
#define TASK_PLAYER 10
#define TASK_START 11
#define TASK_TURN 12
#define TASK_PROD 13
#define TASK_MARKET 14
#define TASK_BUILD 15
#define TASK_SELL 16
#define TASK_BUY 17

#define TASKT_CLIENT 128
#define TASK_EXIT 0

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
