#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include "jobmanager.h"

#define JM_ST_NONE 0
#define JM_ST_RUNNING 1
#define JM_ST_CONTINUED 2
#define JM_ST_STOPPED 3
#define JM_ST_COMPLETED 4

#define JM_MES_NONE 0
#define JM_MES_RUNNING 1
#define JM_MES_CONTINUED 2
#define JM_MES_STOPPED 3
#define JM_MES_COMPLETED 4

#define JM_MESTG_IN 0
#define JM_MESTG_ERR 1

typedef struct process {
	simpleCmd * cmd;
	pid_t pid;

	int retStatus;
	int status;

	struct process * next;
} Process;

typedef struct job {
	jid_t jid;
	pid_t pgid;

	Task * task;
	Process * firstProc;

	int retStatus;
	int status;
	int notified;

	struct termios tmodes;
	int stdin, stdout, stderr;

	struct job * next;
	struct job * prev;
} Job;

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

char * stMessages[] = {
	"",
	"Running",
	"Continued",
	"Stopped",
	"Complited"
};

/* Managing jobs:
 *  - adding in manager;
 *  - deleting from manager.
 */
jid_t addJob(pid_t, Task *);
Job * newJob(void);
void delJob(Job *);
void delJobByJid(jid_t);

/* Properties */
int markProcessStatus(pid_t, int);
int jobIsStopped(Job *);
int jobIsCompleted(Job *);

/* Changing the last and the penult jid in the list. */
void setPenultByLastJid();
void setLastJid(jid_t);

/* Base executing and control functions */
void execCmd(Process *, pid_t, int, int, int, int);
void launchJob(Job *, int);
int makeFG(jid_t, int);
int makeBG(jid_t, int);
void waitJob(jid_t);

/* Periodicaly called functions to take actual
 * information about jobs */
void updateStatus(void);
void checkJobs(void);

/* Working with list of jobs */
Job * getJob(jid_t);
Job * getJobByGpid(pid_t);
Job * getJobByPid(pid_t, Process **);
pid_t getGpidByJid(jid_t);

/* Echoing jobs */
void echoJob(struct job * job, int, int);
void echoJobList(void);



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
void execCmd(Process * p, pid_t pgid,
	int infile, int outfile, int errfile,
	int foreground)
{
	pid_t pid;

	if (prStatus.isInteractive)
	{
		pid = getpid();
		if (pgid == 0)
			pgid = pid;
		setpgid(pid, pgid);

		if (foreground)
			tcsetpgrp(prStatus.terminal, pgid);

		signal (SIGINT, SIG_DFL);
		signal (SIGQUIT, SIG_DFL);
		signal (SIGTSTP, SIG_DFL);
		signal (SIGTTIN, SIG_DFL);
		signal (SIGTTOU, SIG_DFL);
		signal (SIGCHLD, SIG_DFL);
	}

	if (infile != STDIN_FILENO)
	{
		dup2(infile, STDIN_FILENO);
		close(infile);
	}
	if (outfile != STDOUT_FILENO)
	{
		dup2(outfile, STDOUT_FILENO);
		close(outfile);
	}
	if (errfile != STDERR_FILENO)
	{
		dup2(errfile, STDERR_FILENO);
		close(errfile);
	}

	execvp(p->cmd->file, p->cmd->argv);
	perror("exevp");
	exit(EXIT_FAILURE);
}

void launchJob(Job * j, int foreground)
{
	Process * p;
	pid_t pid;
	int pipev[2], infile, outfile;

	infile = j->stdin;

	for (p = j->firstProc; p; p = p->next)
	{
		/* Set up pipes, if need */
		if (p->next)
		{
			if (pipe(pipev) < 0)
			{
				perror("pipe");
				endWork(EXIT_FAILURE);
			}
			outfile = pipev[1];
		}
		else
			outfile = j->stdin;

		/* make child :) */
		if ((pid = fork()) == 0)
		/* baby */
			execCmd(p, j->pgid,
				infile, j->stderr, outfile,
				foreground);
		else if (pid > 0)
		{ /* happy parent */
			p->pid = pid;
			p->status = JM_ST_RUNNING;
			if (prStatus.isInteractive)
			{
				if (!j->pgid)
					j->pgid = pid;
				setpgid(pid, j->pgid);
			}
		}
		else
		{ /* failed to make child, poor parent :( */
			perror("fork");
			endWork(EXIT_FAILURE);
		}

		if (infile != STDIN_FILENO)
			close(infile);
		if (outfile != STDOUT_FILENO)
			close(outfile);
		infile = pipev[0];
	}

	j->status = JM_ST_RUNNING;

	if (!prStatus.isInteractive)
		waitJob(j->jid);
	else if (foreground)
		makeFG(j->jid, 0);
	else
		makeBG(j->jid, 0);
}

int makeFG(jid_t jid, int cont)
{
	Job * j = getJob(jid);

	if (j == NULL)
		return -1;

	tcsetpgrp(prStatus.terminal, j->pgid);

	if (cont)
	{
		tcsetattr(prStatus.terminal, TCSADRAIN, &j->tmodes);
		if (kill(-j->pgid, SIGCONT) < 0)
			perror("kill (SIGCONT)");
	}

	waitJob(jid);

	tcsetpgrp(prStatus.terminal, prStatus.pgid);

	tcgetattr(prStatus.terminal, &j->tmodes);
	tcsetattr(prStatus.terminal, TCSADRAIN, &prStatus.tmodes);

	return 1;
}

int makeBG(jid_t jid, int cont)
{
	Job * j = getJob(jid);

	if (j == NULL)
		return -1;

	if (cont)
		if (kill(-j->pgid, SIGCONT) < 0)
		{
			perror("kill (SIGCONT)");
			return -1;
		}

	return 0;
}

void waitJob(jid_t jid)
{
	Job * j = getJob(jid);
	int status;
	pid_t pid;

	if (j == NULL)
		return;

	do
		pid = waitpid(-j->pgid, &status, WUNTRACED);
	while (markProcessStatus(pid, status) &&
		!jobIsStopped(j) &&
		!jobIsCompleted(j));
}


/*------------------------------------------------------------------------*/
/* Managing jobs:
 *  - adding in manager;
 *  - deleting from manager.
 */
jid_t addJob(pid_t pid, Task * task)
{
	Job * job = newJob();

	job->jid = ++(manager.last_jid);
	job->task = task;
	
	++manager.count;

	return job->jid;
}

Job * newJob(void)
{
	Job * job = (Job *)malloc(sizeof(Job));

	if (job == NULL)
		return NULL;

	job->jid = 0;
	job->pgid = 0;
	job->task = NULL;
	job->firstProc = NULL;
	job->retStatus = 0;
	job->status = JM_ST_NONE;
	job->notified = 1;
	job->stdin = STDIN_FILENO;
	job->stderr = STDERR_FILENO;
	job->stdout = STDOUT_FILENO;

	job->next = NULL;
	job->prev = manager.last;

	if (manager.first == NULL)
		manager.first = job;

	if (manager.last != NULL)
		(manager.last)->next = job;

	manager.last = job;

	return job;
}

void delJob(Job * job)
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

	delTask(job->task);
	free(job);
}

void delJobByJid(jid_t jid)
{
	delJob(getJob(jid));
}


/*------------------------------------------------------------------------*/
/* Properties */
int markProcessStatus(pid_t pid, int status)
{
	Job * j;
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
	else if (pid == 0 && errno == ECHILD)
	/* Nothing interesting to renew */
		return -1;
	else
	/* Something weird happened */
	{
		perror("waitpid");
		return -1;
	}
}

int jobIsStopped(Job * j)
{
	Process * p;

	for (p = j->firstProc; p; p = p->next)
		if (p->status != JM_ST_STOPPED &&
			p->status != JM_ST_COMPLETED)
			return 0;

	j->status = JM_ST_STOPPED;
	return 1;
}

int jobIsCompleted(Job * j)
{
	Process * p;

	for (p = j->firstProc; p; p = p->next)
		if (p->status != JM_ST_COMPLETED)
			return 0;

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

void checkJobs(void)
{
	Job *j;

	updateStatus();

	for (j = manager.first; j; j = j->next)
		if (j->status == JM_ST_COMPLETED)
		{
			echoJob(j, JM_MESTG_ERR, JM_MES_COMPLETED);
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


/*------------------------------------------------------------------------*/
/* Working with list of jobs */
Job * getJob(jid_t jid)
{
	Job * job = manager.first;

	while (job != NULL)
	{
		if (job->jid == jid)
			return job;

		job = job->next;
	}

	return NULL;
}

Job * getJobByGpid(pid_t pgid)
{
	Job * job = manager.first;

	while (job != NULL)
	{
		if (job->pgid == pgid)
			return job;

		job = job->next;
	}

	return NULL;
}

Job * getJobByPid(pid_t pid, Process ** proc)
{
	Job * job = manager.first;
	Process * p;

	while (job != NULL)
	{
		p = job->firstProc;
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
	Job * j = getJob(jid);

	if (j != NULL)
		return j->pgid;
	else
		return 0;
}


/*------------------------------------------------------------------------*/
/* Echoing jobs */
void echoJobList(void)
{
	struct job * job = manager.first;

	while (job != NULL)
	{
		echoJob(job, JM_MESTG_IN, JM_MES_NONE);
		job = job->next;
	}
}

void echoJob(struct job * job, int where, int mes)
{
	char format[] = "[%d] %c %d%s %s\n";
	char order = (job->jid == manager.last_bg_jid ? '+' :
		(job->jid == manager.penult_bg_jid? '-' :
		' '));
	char * command = getCmdString(job->task->cur);

	if (where == 0) /* stdin */
		printf(format, job->jid, order, job->pgid, stMessages[mes], command);
	else /* stderr */
		fprintf(stderr, format, job->jid, order, job->pgid, stMessages[mes], command);

	free(command);
}


