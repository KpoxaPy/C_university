#include "../stuff/echoes.h"
#include "task.h"
#include "jobmanager.h"

void goUp(Task *);
void goDown(Task *);
void checkRelation(Task *);


void goDown(Task * task)
{
	if (task == NULL)
		return;

	switch (task->cur->cmdType)
	{
		case TCMD_SCRIPT:
		case TCMD_LIST:
			task->cur = task->cur->child;
			goDown(task);
			break;
	}
}

void goUp(Task * task)
{
	while (task->cur->rel->relType != TREL_END)
		task->cur = task->cur->rel->next;
	task->cur = task->cur->rel->next;
	checkRelation(task);
}

void checkRelation(Task * task)
{
	if (task->cur->rel == NULL)
	{
		task->cur = NULL;
		return;
	}

	switch (task->cur->rel->relType)
	{
		case TREL_SCRIPT:
			task->cur = task->cur->rel->next;
			goDown(task);
			break;
		case TREL_OR:
			debug("IN OR Getted %d\n", task->curRet);
			if (task->curRet != 0)
			{
				task->cur = task->cur->rel->next;
				goDown(task);
			}
			else
				goUp(task);
			break;
		case TREL_AND:
			debug("IN AND Getted %d\n", task->curRet);
			if (task->curRet == 0)
			{
				task->cur = task->cur->rel->next;
				goDown(task);
			}
			else
				goUp(task);
			break;
		case TREL_END:
			goUp(task);
			break;
	}
}

void getNextJob(Task * task)
{
	if (task == NULL || task->cur == NULL)
		return;

	switch (task->cur->cmdType)
	{
		case TCMD_SCRIPT:
		case TCMD_LIST:
			goDown(task);
			break;
		case TCMD_PIPE:
		case TCMD_SIMPLE:
			if (!task->firstly)
				checkRelation(task);
			break;
	}

	if (task->firstly)
		task->firstly = 0;
}


Task * newTask(void)
{
	Task * task = (Task *)malloc(sizeof(Task));

	if (task == NULL)
		return NULL;

	task->root = NULL;
	task->cur = NULL;
	task->modeBG = 0;
	task->curRet = 0;
	task->curJob = 0;
	task->firstly = 1;

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

void echoTaskStatus(Task * task, int mes)
{
	char format[] = "%s\t%s\n";
	char formatExit[] = "%s %d\t%s\n";
	char * command = getCmdString(task->root);

	if (mes == JM_MES_EXIT)
		fprintf(stderr, formatExit, stMessages[JM_MES_EXIT], task->curRet, command);
	else
		fprintf(stderr, format, stMessages[mes], command);

	free(command);
}


void echoTaskWide(Task * task)
{
	if (task == NULL)
		return;

	echoCmdTree(task->root);
}
