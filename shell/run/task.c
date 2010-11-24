#include "task.h"

Task * newTask(void)
{
	Task * task = (Task *)malloc(sizeof(Task));

	if (task == NULL)
		return NULL;

	task->root = NULL;
	task->cur = NULL;
	task->modeBG = 0;

	return task;
}


void delTask(Task ** task)
{
	if (task == NULL || *task == NULL)
		return;

	delTCmd(&((*task)->root));
	free(*task);
	*task = NULL;
}


void echoTask(Task * task)
{
	if (task == NULL)
		return;

	char * str = getCmdString(task->root);
	printf("%s%s\n", str, (task->modeBG ? " &" : ""));
	free(str);
}

void echoTaskWide(Task * task)
{
	if (task == NULL)
		return;

	echoCmdTree(task->root);
}
