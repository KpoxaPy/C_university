#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include "jobmanager.h"

#define JM_MES_NONE 0
#define JM_MES_RUNNING 1
#define JM_MES_CONTINUED 2
#define JM_MES_STOPPED 3
#define JM_MES_COMPLETED 4
#define JM_MES_EXIT 5

#define JM_MESTG_IN 0
#define JM_MESTG_ERR 1

struct manager {
	mJob * first;
	mJob * last;
	int count;

	jid_t last_jid;
	jid_t last_bg_jid;
	jid_t penult_bg_jid;
};

/* Global manager to the shell */
struct manager manager =
	{NULL, NULL, 0, 0, 0, 0};

char * stMessages[] = {
	"",
	"Running",
	"Continued",
	"Stopped",
	"Done",
	"Exit"
};

mJob * newMJob(void);
void delMJob(mJob **);

/* Properties */
int markProcessStatus(pid_t, int);
int jobIsStopped(mJob *);
int jobIsCompleted(mJob *);

/* Periodicaly called functions to take actual
 * information about jobs */
void updateStatus(void);
void renewManager(void);

/* Echoing jobs */
void echoJob(mJob *, int, int);


/*------------------------------------------------------------------------*/
/*
 * Changing the last and the penult jid in the list.
 */

/*
 * Setting up new last jid.
 * If last jid > 0, last jid and penult jid just shift by new last jid,
 * otherwise, if jid = 0, this jid is excluded and penult jid (if it is
 * non zero) or last job's jid (if las job exists) or zero is new last jid.
 *
 * If you exclude the jid, you must call this AFTER you changed
 * jobs list.
 */
void setLastJid(jid_t jid)
{
	if (manager.last_bg_jid == jid)
		return;

	if (jid > 0)
	{
		if (getJob(jid) != NULL)
		{
			manager.penult_bg_jid = manager.last_bg_jid;
			manager.last_bg_jid = jid;
		}

		return;
	}

	/* if jid == 0 */
	if (manager.penult_bg_jid > 0)
		manager.last_bg_jid = manager.penult_bg_jid;
	else if (manager.last_jid != 0)
		manager.last_bg_jid = manager.last_jid;
	else
		manager.last_bg_jid = 0;

	setPenultByLastJid();
}

void setPenultByLastJid()
{
	if (manager.last_bg_jid == 0 ||
			(manager.first)->jid == manager.last_bg_jid)
		manager.penult_bg_jid = 0;
	else
		manager.penult_bg_jid = getJob(manager.last_bg_jid)->prev->jid;
}

/*------------------------------------------------------------------------*/
/*
 * Base executing functions.
 */
void launchJobByJid(jid_t jid)
{
	mJob * j = getJob(jid);

	if (j == NULL)
		return;

	launchJob(j->job, !j->task->modeBG);
}

void giveTC(mJob * j)
{
	tcsetpgrp(prStatus.terminal, j->job->pgid);
}

void takeBackTC(mJob * j)
{
	tcsetpgrp(prStatus.terminal, prStatus.pgid);

	tcgetattr(prStatus.terminal, &j->job->tmodes);
	tcsetattr(prStatus.terminal, TCSADRAIN, &prStatus.tmodes);
}

int makeFG(jid_t jid, int cont)
{
	mJob * j = getJob(jid);

	if (j == NULL)
		return -1;

	if (cont)
	{
		tcsetattr(prStatus.terminal, TCSADRAIN, &j->job->tmodes);
		if (kill(-j->job->pgid, SIGCONT) < 0)
			perror("kill (SIGCONT)");
	}

	giveTC(j);

	waitJob(jid);

	takeBackTC(j);

	return 1;
}

int makeBG(jid_t jid, int cont)
{
	mJob * j = getJob(jid);

	setLastJid(jid);

	if (j == NULL)
		return -1;

	if (cont)
		if (kill(-j->job->pgid, SIGCONT) < 0)
		{
			perror("kill (SIGCONT)");
			return -1;
		}

	return 0;
}

void waitJob(jid_t jid)
{
	mJob * j = getJob(jid);
	int status;
	pid_t pid;

	if (j == NULL)
		return;

	do
		pid = waitpid(-j->job->pgid, &status, WUNTRACED);
	while (!markProcessStatus(pid, status) &&
		!jobIsStopped(j) &&
		!jobIsCompleted(j));

	printf("J STATUS %d RETSTATUS %d\n", j->status, j->job->retStatus);

	if (j->status == JM_ST_STOPPED && j->job->retStatus == SIGSTOP)
	{
		makeBG(jid, 0);
	}
}


/*------------------------------------------------------------------------*/
/* Managing jobs:
 *  - adding in manager;
 *  - deleting from manager.
 */
mJob * newMJob(void)
{
	mJob * job = (mJob *)malloc(sizeof(mJob));

	if (job == NULL)
		return NULL;

	job->jid = 0;
	job->job = NULL;
	job->task = NULL;

	job->status = JM_ST_NONE;
	job->notified = 1;

	job->next = NULL;
	job->prev = NULL;

	return job;
}

jid_t addJob(Task * task)
{
	Job * job = takeJob();
	mJob * mjob = newMJob();

	mjob->jid = ++(manager.last_jid);
	mjob->job = fillJob(job, task->cur);
	mjob->task = task;

	mjob->prev = manager.last;

	if (manager.first == NULL)
		manager.first = mjob;

	if (manager.last != NULL)
		(manager.last)->next = mjob;

	manager.last = mjob;
	
	++manager.count;

	return mjob->jid;
}

void delJob(mJob * job)
{
	if (job == NULL)
		return;

	--(manager.count);

	if (job->jid == manager.last_jid)
	{
		if (job != manager.first)
			manager.last_jid = job->prev->jid;
		else
			manager.last_jid = 0;
	}
	
	if (job == manager.first)
		manager.first = job->next;
	else
		job->prev->next = job->next;

	if (job == manager.last)
		manager.last = job->prev;
	else
		job->next->prev = job->prev;

	freeJob(job->job);
}

void delJobByJid(jid_t jid)
{
	delJob(getJob(jid));
}


/*------------------------------------------------------------------------*/
/* Properties */
int markProcessStatus(pid_t pid, int status)
{
	mJob * j;
	Process * p;

	if (pid > 0)
	{
		j = getJobByPid(pid, &p);

		if (p != NULL)
		{
			int olds = j->status;
			if (WIFEXITED(status))
			{
				p->status = JM_ST_COMPLETED;
				p->retStatus = WEXITSTATUS(status);
			}
			else if (WIFSIGNALED(status))
			{
				p->status = JM_ST_COMPLETED;
				p->retStatus = WTERMSIG(status);
			}
			else if (WIFSTOPPED(status))
			{
				p->status = JM_ST_STOPPED;
				p->retStatus = WSTOPSIG(status);
			}
			else if (WIFCONTINUED(status))
			{
				p->status = JM_ST_RUNNING;
				j->status = JM_ST_RUNNING;
			}

			jobIsStopped(j);
			jobIsCompleted(j);

			j->notified = (olds == j->status);

			return 0;
		}
		else
			fprintf(stderr, "No child process %d\n", pid);

		return -1;
	}
	else
	/* Nothing interesting to renew */
	/* Something weird happened */
		return -1;
}

int jobIsStopped(mJob * j)
{
	Process * p;

	for (p = j->job->firstProc; p; p = p->next)
	{
		if (p->next == NULL)
			j->job->retStatus = p->retStatus;
		if (p->status != JM_ST_STOPPED &&
			p->status != JM_ST_COMPLETED)
		return 0;
	}

	j->status = JM_ST_STOPPED;
	return 1;
}

int jobIsCompleted(mJob * j)
{
	Process * p;

	for (p = j->job->firstProc; p; p = p->next)
	{
		if (p->next == NULL)
			j->job->retStatus = p->retStatus;
		if (p->status != JM_ST_COMPLETED)
			return 0;
	}

	j->status = JM_ST_COMPLETED;
	return 1;
}


/*------------------------------------------------------------------------*/
/* Periodicaly called functions to take actual
 * information about jobs */
void updateStatus(void)
{
	pid_t pid;
	int status;

	do
		pid = waitpid(WAIT_ANY, &status, WNOHANG | WUNTRACED | WCONTINUED);
	while (!markProcessStatus(pid, status));
}

void renewManager()
{
	mJob *j;

	for (j = manager.first; j; j = j->next)
		if (j->status == JM_ST_COMPLETED)
		{
			if (j->job->retStatus != 0)
				echoJob(j, JM_MESTG_ERR, JM_MES_EXIT);
			else
				echoJob(j, JM_MESTG_ERR, JM_MES_COMPLETED);
			j->task->curRet = j->job->retStatus;
			j->task->curJob = 0;
			delJob(j);
		}
		else if (j->status == JM_ST_STOPPED && !j->notified)
		{
			echoJob(j, JM_MESTG_IN, JM_MES_STOPPED);
			j->notified = 1;
		}
		else if (j->status == JM_ST_CONTINUED)
		{
			if (!j->notified)
			{
				echoJob(j, JM_MESTG_IN, JM_MES_CONTINUED);
				j->notified = 1;
			}
			j->status = JM_ST_RUNNING;
		}
}

void checkJobs(void)
{
	updateStatus();

	renewManager();
}


/*------------------------------------------------------------------------*/
/* Working with list of jobs */
mJob * getJob(jid_t jid)
{
	mJob * job = manager.first;

	while (job != NULL)
	{
		if (job->jid == jid)
			return job;

		job = job->next;
	}

	return NULL;
}

mJob * getJobByGpid(pid_t pgid)
{
	mJob * job = manager.first;

	while (job != NULL)
	{
		if (job->job->pgid == pgid)
			return job;

		job = job->next;
	}

	return NULL;
}

mJob * getJobByPid(pid_t pid, Process ** proc)
{
	mJob * job = manager.first;
	Process * p;

	while (job != NULL)
	{
		p = job->job->firstProc;
		while (p != NULL)
		{
			if (p->pid == pid)
			{
				if (proc != NULL)
					*proc = p;
				return job;
			}

			p = p->next;
		}

		job = job->next;
	}
	
	return NULL;
}

pid_t getGpidByJid(jid_t jid)
{
	mJob * j = getJob(jid);

	if (j != NULL)
		return j->job->pgid;
	else
		return 0;
}


/*------------------------------------------------------------------------*/
/* Echoing jobs */
void echoJobList(void)
{
	mJob * job = manager.first;

	while (job != NULL)
	{
		echoJob(job, JM_MESTG_IN, JM_MES_NONE);
		job = job->next;
	}
}

void echoJob(mJob* job, int where, int mes)
{
	char format[] = "[%d] %c %d %s\t%s\n";
	char formatExit[] = "[%d] %c %d %s %d\t%s\n";
	char order = (job->jid == manager.last_bg_jid ? '+' :
		(job->jid == manager.penult_bg_jid? '-' :
		' '));
	char * command = getCmdString(job->task->cur);

	if (mes == JM_MES_EXIT)
	{
		if (where == 0) /* stdin */
			printf(format, job->jid, order, job->job->pgid, stMessages[mes], command);
		else /* stderr */
			fprintf(stderr, format, job->jid, order, job->job->pgid, stMessages[mes], command);
	}
	else
	{
		if (where == 0) /* stdin */
			printf(formatExit, job->jid, order, job->job->pgid, stMessages[mes], job->job->retStatus, command);
		else /* stderr */
			fprintf(stderr, formatExit, job->jid, order, job->job->pgid, stMessages[mes], job->job->retStatus, command);
	}


	free(command);
}


