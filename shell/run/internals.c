#include "internals.h"
/*#include "jobmanager.h"*/

int checkInternalCommands(simpleCmd * cmd)
{
	int status = INTERNAL_COMMAND_CONTINUE;

	if (cmd->file != NULL)
	{
		if (strcmp(cmd->file, "exit") == 0)
			status = runExit();
		else if (strcmp(cmd->file, "cd") == 0)
			status = runCD(cmd);
		/*
		 *else if (strcmp(cmd->file, "jobs") == 0)
		 *  status = runJobs(cmd);
		 *else if (strcmp(cmd->file, "bg") == 0)
		 *  status = runJobsBG(cmd);
		 *else if (strcmp(cmd->file, "fg") == 0)
		 *  status = runJobsFG(cmd);
		 */
	}

	return status;
}

int runExit()
{
	endWork(EXIT_SUCCESS);

	return INTERNAL_COMMAND_SUCCESS;
}


int runCD(simpleCmd * cmd)
{
	int status = INTERNAL_COMMAND_FAILURE;

	if (cmd->argc < 3)
	{
		char * path;

		if (cmd->argc == 2)
			path = cmd->argv[1];
		else
			path = getenv("HOME");

		if (path == NULL)
			fprintf(stderr, "No home directory!\n");
		else if (chdir(path))
			perror(path);
		else
			status = INTERNAL_COMMAND_SUCCESS;
	}
	else
		fprintf(stderr, "Too many args!\n");

	return status;
}


/*
 *int runJobs(simpleCmd * cmd)
 *{
 *  if (cmd->argc == 1)
 *    echoJobList();
 *  else
 *    printf("Too many args!\n");
 *
 *  return INTERNAL_COMMAND_CONTINUE;
 *}
 *
 *int runJobsBG(simpleCmd * cmd)
 *{
 *  if (cmd->argc == 2)
 *  {
 *    jid_t jid = atoi(cmd->argv[1]);
 *    if (jid > 0)
 *      setLastJid(jid);
 *  }
 *
 *  return INTERNAL_COMMAND_CONTINUE;
 *}
 *
 *int runJobsFG(simpleCmd * cmd)
 *{
 *  if (cmd->argc == 2)
 *  {
 *    jid_t jid = atoi(cmd->argv[1]);
 *    if (jid > 0)
 *      makeFG(jid, NULL);	
 *  }
 *  else if (cmd->argc > 2)
 *    printf("Too many args!\n");
 *
 *  return INTERNAL_COMMAND_CONTINUE;
 *}
 */
