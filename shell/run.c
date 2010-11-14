#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "run.h"
#include "internals.h"

pid_t execSimpleCommand(simpleCmd *, int);
pid_t execOutterCommand(simpleCmd *, int);

int runSimpleCommand(simpleCmd *, int *);
int runPipe(tCmd *, int *);

/*int runFG(simpleCmd *);*/
/*int runBG(simpleCmd *);*/

int processRedirectInput(simpleCmd *);
int processRedirectOutput(simpleCmd *);

void processCmdTree(tCmd * node)
{
/*
 *  status.internal = checkInternalCommands(&status, cmd);
 *
 *  if (status.internal == INTERNAL_COMMAND_BREAK)
 *    break;
 *  else if (status.internal == INTERNAL_COMMAND_OK)
 *    run(&status, cmdmand);
 *  [> else if internalStatus == INTERNAL_COMMAND_CONTINUE <]
 *
 *  if (!cmdmand->bg)
 *    delCommand(&cmdmand);
 */
	if (node == NULL)
		return;

	if (node->cmdType == TCMD_SIMPLE)
		runSimpleCommand(node->cmd, NULL);
	else if (node->cmdType == TCMD_PIPE)
		runPipe(node, NULL);
}

pid_t execSimpleCommand(simpleCmd * cmd, int pgid)
{
	int fdin = dup(0),
			fdout = dup(1);
	int internalStatus;
	int returnStatus = 0;

	if (cmd->pFDin == -1)
		returnStatus = processRedirectInput(cmd);
	else
	{
		dup2(cmd->pFDin, 0);
		close(cmd->pFDin);
	}

	if (cmd->pFDout == -1)
		returnStatus = processRedirectOutput(cmd);
	else
	{
		dup2(cmd->pFDout, 1);
		close(cmd->pFDout);
	}

	if (!returnStatus)
	{
		internalStatus = checkInternalCommands(cmd);

		if (internalStatus == INTERNAL_COMMAND_CONTINUE)
			returnStatus = execOutterCommand(cmd, pgid);
		else if (internalStatus == INTERNAL_COMMAND_FAILURE)
			returnStatus = -1;
		else if (internalStatus == INTERNAL_COMMAND_SUCCESS)
			returnStatus = 0;
	}

	dup2(fdin, 0);
	close(fdin);
	dup2(fdout, 1);
	close(fdout);

	return returnStatus;
}

pid_t execOutterCommand(simpleCmd * cmd, int pgid)
{
	pid_t pid;

	if ((pid = fork()) == 0)
	{
		if (setpgid(0, pgid))
		{
			perror("Setpgid(0, 0) makes bad :(\n");
			exit(EXIT_FAILURE);
		}

		execvp(cmd->file, cmd->argv);
		perror(cmd->file);
		exit(EXIT_FAILURE);
	}

	return pid;
}

int runSimpleCommand(simpleCmd * cmd, int * status)
{
	pid_t pid = execSimpleCommand(cmd, 0);
	int returnStatus = pid;

	if (pid > 0)
	{
		signal(SIGTTOU, SIG_IGN);
		tcsetpgrp(0, pid);

		waitpid(pid, status, 0);

		tcsetpgrp(0, prStatus.pgid);
		signal(SIGTTOU, SIG_DFL);

		if ((status != NULL && *status == 0) || status == NULL)
			returnStatus = 0;
	}

	return returnStatus;
}

int runPipe(tCmd * node, int * status)
{
	return 0;
}

int processRedirectInput(simpleCmd * cmd)
{
	if (cmd->rdrInputFile != NULL)
	{
		int fd;
		if ((fd = open(cmd->rdrInputFile, O_RDONLY)) == -1)
		{
			perror(cmd->rdrInputFile);
			return -1;
		}

		dup2(fd, 0);
		close(fd);
	}

	return 0;
}

int processRedirectOutput(simpleCmd * cmd)
{
	if (cmd->rdrOutputFile != NULL)
	{
		int fd;
		int flags = O_WRONLY | O_CREAT;
		int mode = 0644;

		if (cmd->rdrOutputAppend)
			flags |= O_APPEND;

		if ((fd = open(cmd->rdrOutputFile, flags, mode)) == -1)
		{
			perror(cmd->rdrOutputFile);
			return -1;
		}

		dup2(fd, 1);
		close(fd);
	}

	return 0;
}

/*
 *int run(simpleCmd * cmd)
 *{
 *  if (!cmd->bg)
 *    return runFG(pstatus, cmd);
 *  else
 *    return runBG(pstatus, cmd);
 *}
 */
/*int runFG(simpleCmd * cmd)
 *{
 *  pid_t pid;
 *  int status;
 *
 *  if ((pid = fork()) == 0)
 *  {
 *    if (setpgid(0, 0))
 *    {
 *      perror("Setpgid(0, 0) makes bad :(\n");
 *      exit(1);
 *    }
 *
 *    execvp(cmd->file, cmd->argv);
 *    perror(cmd->file);
 *    exit(1);
 *  }
 *
 *  signal(SIGTTOU, SIG_IGN);
 *  tcsetpgrp(0, pid);
 *
 *  status = waitFG(pid);
 *
 *  tcsetpgrp(0, pstatus->pgid);
 *  signal(SIGTTOU, SIG_DFL);
 *
 *  return status;
 *}
 *
 *int runBG(simpleCmd * cmd)
 *{
 *  pid_t pid;
 *  jid_t jid;
 *
 *  if ((pid = fork()) == 0)
 *  {
 *    if (setpgid(0, 0))
 *    {
 *      perror("Setpgid(0, 0) makes bad :(\n");
 *      exit(1);
 *    }
 *
 *    execvp(cmd->file, cmd->argv);
 *    perror(cmd->file);
 *    exit(1);
 *  }
 *
 *  // Adding proccess into manager
 *  jid = addJob(pid, cmd);	
 *  setLastJid(jid);	
 *
 *  return 0;
 *}
 *
 */
