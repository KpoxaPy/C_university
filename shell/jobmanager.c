#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "jobmanager.h"

#define JOB_STATUS_FG 0
#define JOB_STATUS_BG 1
#define JOB_STATUS_STOPPED 2

struct job {
	pid_t pid;
	jid_t jid;
	int status;
	struct command * cmd;
	struct job * next;
	struct job * prev;
};

struct manager {
	struct job * first;
	struct job * last;
	int count;
	jid_t last_jid;
	jid_t last_bg_jid;
	jid_t penult_bg_jid;
};

/* Global manager to the shell */
struct manager manager =
	{NULL, NULL, 0, 0, 0, 0};

/* Changing the last and the penult jid in the list. */
/*void setPenultByLastJid();*/
/*void setLastJid(jid_t);*/

/* Working with list of jobs */
struct job * searchJobElem(jid_t);
struct job * searchJobElemByPid(pid_t);
/*pid_t searchPidByJid(jid_t);*/
/*struct command * searchCmdByJid(jid_t);*/

struct job * addJobElem();
void removeJobElem(struct job *);
void removeJobElemByJid(jid_t);

/* Echoing jobs */
void echoJob(struct job * job);
/*void echoJobList();*/


jid_t addJob(pid_t pid, struct command * cmd)
{
	struct job * job = addJobElem();

	job->pid = pid;
	job->jid = ++(manager.last_jid);
	job->cmd = cmd;
	
	++manager.count;

	return job->jid;
}

int makeFG(jid_t jid, int * pstatus)
{
	struct job * job = searchJobElem(jid);
	int status;

	if (job->pid == 0)
		return -1;

	setLastJid(jid);
	job->status = JOB_STATUS_FG;

	signal(SIGTTOU, SIG_IGN);
	tcsetpgrp(0, job->pid);

	kill(job->pid, SIGCONT);

	status = waitFG(job->pid);

	tcsetpgrp(0, getpgrp());
	signal(SIGTTOU, SIG_DFL);

	if (!WIFSTOPPED(status))
	{
		free(job->cmd);
		removeJobElem(job);
	}
	else
		job->status = JOB_STATUS_STOPPED;

	if (pstatus != NULL)
		*pstatus = status;

	return 0;
}


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
		if (searchJobElem(jid) != NULL)
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
		manager.penult_bg_jid = searchJobElem(manager.last_bg_jid)->prev->jid;
}


/*------------------------------------------------------------------------*/

void checkZombies()
{
	pid_t pid;
	int status;
	struct job * job;

	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
	{
		job = searchJobElemByPid(pid);

		if (WIFEXITED(status))
		{
			printf("[%d] was exited, status=%d\n", pid, WEXITSTATUS(status));
			free(job->cmd);
			removeJobElem(job);
		}
		else if (WIFSIGNALED(status))
		{
			printf("[%d] was killed by signal %d\n", pid, WTERMSIG(status));
			free(job->cmd);
			removeJobElem(job);
		}
		else if (WIFSTOPPED(status))
		{
			job->status = JOB_STATUS_STOPPED;
			printf("[%d] was stopped by signal %d\n", pid, WSTOPSIG(status));
		}
		else if (WIFCONTINUED(status))
			job->status = JOB_STATUS_BG;
	}
}

int waitJid(jid_t jid)
{
	while (wait(NULL) != -1)
		;

	return 0;
}

int waitFG(pid_t pid)
{
	int status;

	waitpid(pid, &status, WUNTRACED);

	return status;
}


/*------------------------------------------------------------------------*/

void echoJobList()
{
	struct job * job = manager.first;

	while (job != NULL)
	{
		echoJob(job);
		job = job->next;
	}
}

void echoJob(struct job * job)
{
	printf("[%d] %c %d%s %s\n", job->jid,
			                    (job->jid == manager.last_bg_jid ? '+' :
													 (job->jid == manager.penult_bg_jid? '-' :
														' ')),
													job->pid,
													(job->status == JOB_STATUS_STOPPED ?
													 "(stopped)" : ""),
													job->cmd->file);
}


/*------------------------------------------------------------------------*/
/*
 * Working with list of jobs in manager:
 *  - creating element;
 *  - searching element by jid;
 *  - removing element by jid and by pointer on list element
 */

struct job * addJobElem()
{
	struct job * job = (struct job *)malloc(sizeof(struct job));

	job->next = NULL;
	job->prev = manager.last;

	if (manager.first == NULL)
		manager.first = job;

	if (manager.last != NULL)
		(manager.last)->next = job;

	manager.last = job;

	return job;
}

struct job * searchJobElem(jid_t jid)
{
	struct job * job = manager.first,
						 * fjob = NULL;

	while (job != NULL)
	{
		if (job->jid == jid)
		{
			fjob = job;
			break;
		}

		job = job->next;
	}
	
	return fjob;
}

struct job * searchJobElemByPid(pid_t pid)
{
	struct job * job = manager.first,
						 * fjob = NULL;

	while (job != NULL)
	{
		if (job->pid == pid)
		{
			fjob = job;
			break;
		}

		job = job->next;
	}
	
	return fjob;
}

pid_t searchPidByJid(jid_t jid)
{
	struct job * job = searchJobElem(jid);

	if (job != NULL)
		return job->pid;

	return 0;
}

struct command * searchCmdByJid(jid_t jid)
{
	struct job * job = searchJobElem(jid);

	if (job != NULL)
		return job->cmd;

	return NULL;
}

void removeJobElem(struct job * job)
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

	free(job);
}

void removeJobElemByJid(jid_t jid)
{
	struct job * job = searchJobElem(jid);

	if (job != NULL)
		removeJobElem(job);
}
