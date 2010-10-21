#include <sys/wait.h>
#include <unistd.h>
#include "run.h"
#include "internals.h"

int runFG(struct command *);
int runBG(struct command *);

int run(struct command * com)
{
	if (!com->bg)
		return runFG(com);
	else
		return runBG(com);
}

int runFG(struct command * com)
{
	pid_t pid;
	int pid_status;

	if ((pid = fork()) == 0)
	{
		execvp(com->file, com->argv);
		perror(com->file);
		exit(1);
	}

	while (wait(&pid_status) != -1)
		;

	return pid_status;
}

int runBG(struct command * com)
{
	pid_t pid;

	if ((pid = fork()) == 0)
	{
		execvp(com->file, com->argv);
		perror(com->file);
		exit(1);
	}

	// Adding bg proccess into bg-manager

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
	}

	return status;
}

struct command * genCommand(struct wordlist * words)
{
	struct command * cmd;

	if (words->count)
	{
		int i = 0;
		int bg;
		struct word * wtmp = words->first;
		char * tmpStr;

		// Analyzing for background execution
		wtmp = words->first;
		bg = 0;
		while (wtmp != NULL)
		{
			if (!bg && strcmp(wtmp->str, "&") == 0)
				bg = 1;
			else if (bg)
				return NULL;
			
			wtmp = wtmp->next;
		}

		wtmp = words->first;
		cmd = (struct command *)malloc(sizeof(struct command));
		cmd->argv = (char **)malloc(sizeof(char *)*(words->count + 1));

		while (wtmp != NULL)
		{
			if (strcmp(wtmp->str, "&") != 0)
			{
				tmpStr = (char *)malloc(sizeof(char)*(wtmp->len));
				cmd->argv[i++] = strcpy(tmpStr, wtmp->str);
			}
			else
				break;

			wtmp = wtmp->next;
		}

		cmd->argc = i;
		cmd->argv[i] = NULL;
		cmd->file = cmd->argv[0];
		cmd->bg = bg;
		cmd->words = words;
	}
	else
		return NULL;

	return cmd;
}

void delCommand(struct command ** com)
{
	if (com != NULL && *com != NULL)
	{
		char ** pStr = (*com)->argv;

		while (*pStr != NULL)
			free(*pStr++);

		free((*com)->argv);
		free(*com);

		*com = NULL;
	}
}
