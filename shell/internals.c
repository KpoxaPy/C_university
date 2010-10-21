#include <unistd.h>
#include "internals.h"

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
