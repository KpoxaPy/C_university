#include <fcntl.h>
#include "task.h"
#include "../stuff/buffer.h"

int genArgVector(Task * task, lexList * list)
{
	Lex * lex;
	char ** argv;
	int i;
	int lc;

	if (list == NULL || task == NULL)
		return -1;

	lc = list->count;

	argv = (char **)malloc(sizeof(char *)*
		(lc + 1));

	if (argv == NULL)
		return -1;

	for (i = 0; i < lc; i++)
	{
		lex = getLex(list);

		if (lex == NULL)
			break;

		argv[i] = strdup(lex->str);
		delLex(lex);
	}

	argv[i] = NULL;

	task->argv = argv;
	task->argc = lc;

	return 0;
}

Task * newTask()
{
	Task * task;

	task = (Task *)malloc(sizeof(Task));

	if (task == NULL)
		return NULL;

	task->argc = 0;
	task->argv = NULL;

	return task;
}

void delTask(Task ** task)
{
	if (task != NULL && *task != NULL)
	{
		char ** pStr = (*task)->argv;

		if (pStr != NULL)
			while (*pStr != NULL)
				free(*pStr++);

		free((*task)->argv);
		free(*task);

		*task = NULL;
	}
}

void echoTaskString(Task * task)
{
	char ** argp;

	if (task == NULL)
		return;

	argp = task->argv;
	while (*argp != NULL)
		printf("[%s]\n", *argp++);
}
