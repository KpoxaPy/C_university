#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "run.h"
#include "jobmanager.h"

typedef struct managedTask {
	Task * task;

	struct managedTask * next;
} managedTask;
typedef managedTask mTask;

struct TaskManager {
	mTask * first;

	int count;
} tman = {NULL, 0};


mTask * newMTask(void);
void delMTask(mTask **);

mTask * addTask(Task *);
void remTask(mTask *);

mTask * findTaskByJid(jid_t);

int checkInternalTask(Task * task)
{
	int status;

	if (task == NULL ||
		task->root == NULL ||
		task->root->cmdType != TCMD_SIMPLE ||
		task->root->cmd == NULL)
		return INTERNAL_COMMAND_CONTINUE;

	status = checkInternalCommands(task->root->cmd);

	if (status != INTERNAL_COMMAND_CONTINUE)
		delTask(&task);

	return status;
}

void runTask(Task * task)
{
	mTask * mtask;

	if (task == NULL)
		return;

	mtask = addTask(task);
}

mTask * findTaskByJid(jid_t jid)
{
	mTask * task = tman.first;

	while (task != NULL)
	{
		if (task->task->curJob == jid)
			return task;

		task = task->next;
	}
	
	return NULL;
}


mTask * newMTask(void)
{
	mTask * task = (mTask *)malloc(sizeof(mTask));

	if (task == NULL)
		return NULL;

	task->task = NULL;
	task->next = NULL;

	return task;
}

void delMTask(mTask ** task)
{
	mTask * c, * p;

	if (task == NULL || *task == NULL)
		return;

	c = tman.first;
	p = NULL;

	while (c != *task)
	{
		p = c;
		c = c->next;
	}

	if (p != NULL)
		p->next = c->next;
	else
		tman.first = c->next;

	delTask(&((*task)->task));
	free(*task);
	*task = NULL;
}


mTask * addTask(Task * task)
{
	mTask * mtask = newMTask();

	if (mtask == NULL)
		return NULL;

	mtask->task = task;
	mtask->next = tman.first;
	tman.first = mtask;
	++(tman.count);

	return mtask;
}

void remTask(mTask * task)
{
	mTask * ftask = tman.first,
		* ptask = NULL;

	while (ftask != NULL)
	{
		if (ftask == task)
		{
			if (ptask == NULL) /* If found mTask elem is first in task manager */
				tman.first = ftask->next;
			else
				ptask->next = ftask->next;

			delMTask(&ptask);
			--(tman.count);

			return;
		}

		ptask = ftask;
		ftask = ftask->next;
	}
}


void checkTasks()
{
	mTask * task;
	Task * t;

	/*
	 * Firstly we check background tasks: which is complete its job
	 * we either run other job from task or complete task.
	 */
	task = tman.first;
	while (task != NULL)
	{
		t = task->task;

		if (t->modeBG && t->curJob == 0)
		{
			getNextJob(t);
			if (t->cur != NULL)
			/* If task doesn't end */
			{
				t->curJob = addJob(t);
				launchJobByJid(t->curJob);
				makeBG(t->curJob, 0);
			}
			else
			/* otherwise */
			{
				mTask * tmp;
				tmp = task->next;

				echoTaskStatus(t, (t->curRet != 0 ? JM_MES_EXIT : JM_MES_COMPLETED));
				delMTask(&task);

				if (tmp == NULL)
					break;
				task = tmp;
			}
		}

		task = task->next;
	}

	/*
	 * Secondly we seek foreground task and if it was founded we run
	 * it and waiting for either it comlete all jobs or it stopped
	 * (so we do it background).
	 */
	task = tman.first;
	while (task != NULL && task->task->modeBG)
		task = task->next;

	/* If foreground task found */
	if (task != NULL)
	{
		t = task->task;

		getNextJob(t);

		while (t->cur != NULL)
		{
			t->curJob = addJob(t);
			launchJobByJid(t->curJob);
			makeFG(t->curJob, 0);
			if (t->modeBG)
				break;
			getNextJob(t);
		}
		/* If task went into BG */
		if (t->modeBG)
			; /* Do nothing */
		/* or was ended */
		else
			delMTask(&task);
	}

	checkJobs();
}
