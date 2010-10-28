#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "run.h"
#include "internals.h" 

int run(struct programStatus *, struct command * );
int checkInternalCommands(struct programStatus *, struct command *);

int runFG(struct programStatus *, struct command *);
int runBG(struct programStatus *, struct command *);

int processCommand(struct cmdElem * cmdTree)
{
/*
 *  status.internal = checkInternalCommands(&status, command);
 *
 *  if (status.internal == INTERNAL_COMMAND_BREAK)
 *    break;
 *  else if (status.internal == INTERNAL_COMMAND_OK)
 *    run(&status, command);
 *  [> else if internalStatus == INTERNAL_COMMAND_CONTINUE <]
 *
 *  if (!command->bg)
 *    delCommand(&command);
 */
	return INTERNAL_COMMAND_BREAK;
}

int run(struct programStatus * pstatus, struct command * com)
{
	if (!com->bg)
		return runFG(pstatus, com);
	else
		return runBG(pstatus, com);
}

int runFG(struct programStatus * pstatus, struct command * com)
{
	pid_t pid;
	int status;

	if ((pid = fork()) == 0)
	{
		if (setpgid(0, 0))
		{
			perror("Setpgid(0, 0) makes bad :(\n");
			exit(1);
		}

		execvp(com->file, com->argv);
		perror(com->file);
		exit(1);
	}

	signal(SIGTTOU, SIG_IGN);
	tcsetpgrp(0, pid);

	status = waitFG(pid);

	tcsetpgrp(0, pstatus->pgid);
	signal(SIGTTOU, SIG_DFL);

	return status;
}

int runBG(struct programStatus * pstatus, struct command * com)
{
	pid_t pid;
	jid_t jid;

	if ((pid = fork()) == 0)
	{
		if (setpgid(0, 0))
		{
			perror("Setpgid(0, 0) makes bad :(\n");
			exit(1);
		}

		execvp(com->file, com->argv);
		perror(com->file);
		exit(1);
	}

	// Adding proccess into manager
	jid = addJob(pid, com);	
	setLastJid(jid);	

	return 0;
}

int checkInternalCommands(struct programStatus * pstatus,
		struct command * com)
{
	int status = INTERNAL_COMMAND_OK;

	if (com->file != NULL)
	{
		if (pstatus->justEcho)
			status = runEcho(pstatus, com->words);
		else if (strcmp(com->file, "exit") == 0)
			status = runExit(pstatus);
		else if (strcmp(com->file, "cd") == 0)
			status = runCD(pstatus, com);
		else if (strcmp(com->file, "jobs") == 0)
			status = runJobs(pstatus, com);
		else if (strcmp(com->file, "bg") == 0)
			status = runJobsBG(pstatus, com);
		else if (strcmp(com->file, "fg") == 0)
			status = runJobsFG(pstatus, com);
	}

	return status;
}
