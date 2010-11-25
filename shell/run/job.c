#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include "job.h"

void execCmd(Process *, pid_t, int, int, int, int);

Process * newProcess(void);
void delProcess(Process **);


Job * takeJob()
{
	Job * job = newJob();

	return job;
}

Job * newJob(void)
{
	Job * job = (Job *)malloc(sizeof(Job));

	if (job == NULL)
		return NULL;

	job->pgid = 0;
	job->firstProc = NULL;
	job->retStatus = 0;
	job->stdin = STDIN_FILENO;
	job->stderr = STDERR_FILENO;
	job->stdout = STDOUT_FILENO;

	return job;
}

void freeJob(Job * job)
{
	Process * p, * tmp;

	if (job == NULL)
		return;

	p = job->firstProc;

	while (p != NULL)
	{
		tmp = p->next;
		free(p);
		p = tmp;
	}

	free(job);
}

Job * fillJob(Job * job, tCmd * cmd)
{
	if (cmd->cmdType == TCMD_SIMPLE)
	{
		job->firstProc = newProcess();
		job->firstProc->cmd = cmd->cmd;
	
	}
	else if (cmd->cmdType == TCMD_PIPE)
	{
		tCmd * child = cmd->child;
		Process * lastp = NULL,
			* tmp;

		while (child != NULL)
		{
			tmp = newProcess();
			tmp->cmd = child->cmd;

			if (lastp == NULL)
				job->firstProc = tmp;
			else
				lastp->next = tmp;

			lastp = tmp;

			if (child->rel == NULL || child->rel->relType == TREL_END)
				child = NULL;
			else
				child = child->rel->next;
		}
	}

	return job;
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


	/*if (!prStatus.isInteractive)*/
		/*waitJob(j->jid);*/
	/*else if (foreground)*/
		/*makeFG(j->jid, 0);*/
	/*else*/
		/*makeBG(j->jid, 0);*/
}


Process * newProcess(void)
{
	Process * proc = (Process *)malloc(sizeof(Process));

	if (proc == NULL)
		return NULL;

	proc->cmd = NULL;
	proc->pid = 0;
	proc->retStatus = 0;
	proc->status = 0;
	proc->next = NULL;

	return proc;
}

void delProcess(Process ** proc)
{
	if (proc == NULL || *proc == NULL)
		return;

	free(*proc);
	*proc = NULL;
}
