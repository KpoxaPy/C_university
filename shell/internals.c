#include <unistd.h>
#include "internals.h"

int runEcho(struct programStatus * pstatus,
		struct wordlist * words)
{
	echoWordList(words);
	return INTERNAL_COMMAND_CONTINUE;
}


int runExit(struct programStatus * pstatus,
		struct wordlist * words)
{
	return INTERNAL_COMMAND_BREAK;
}


int runCD(struct programStatus * pstatus,
		struct wordlist * words)
{
	int status = INTERNAL_COMMAND_CONTINUE;

	if (words->count < 3)
	{
		char * path;

		if (words->count == 2)
			path = words->first->next->str;
		else
			path = getenv("HOME");

		if (chdir(path))
			perror(strerror(errno));
	}
	else
		printf("Too many args!\n");

	return status;
}
