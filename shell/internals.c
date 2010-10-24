#include <unistd.h>
#include "internals.h"
#include "jobmanager.h"

int runEcho(struct programStatus * pstatus,
		struct wordlist * words)
{
	echoWordList(words);
	return INTERNAL_COMMAND_CONTINUE;
}


int runExit(struct programStatus * pstatus)
{
	return INTERNAL_COMMAND_BREAK;
}


int runCD(struct programStatus * pstatus,
		struct command * com)
{
	int status = INTERNAL_COMMAND_CONTINUE;

	if (com->argc < 3)
	{
		char * path;

		if (com->argc == 2)
			path = com->argv[1];
		else
			path = getenv("HOME");

		if (path == NULL)
			printf("No home directory!\n");
		else if (chdir(path))
			perror(strerror(errno));
	}
	else
		printf("Too many args!\n");

	return status;
}


int runJobs(struct programStatus * pstatus,
		struct command * com)
{
	if (com->argc == 1)
		echoJobList();
	else
		printf("Too many args!\n");

	return INTERNAL_COMMAND_CONTINUE;
}

int runJobsBG(struct programStatus * pstatus,
		struct command * com)
{
	if (com->argc == 2)
	{
		jid_t jid = atoi(com->argv[1]);
		if (jid > 0)
			setLastJid(jid);
	}

	return INTERNAL_COMMAND_CONTINUE;
}

int runJobsFG(struct programStatus * pstatus,
		struct command * com)
{
	if (com->argc == 2)
	{
		jid_t jid = atoi(com->argv[1]);
		if (jid > 0)
			makeFG(jid, NULL);	
	}
	else if (com->argc > 2)
		printf("Too many args!\n");

	return INTERNAL_COMMAND_CONTINUE;
}
