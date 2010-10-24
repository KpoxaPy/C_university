#include <unistd.h>
#include "main.h"
#include "words.h"
#include "parser.h"
#include "echoes.h"
#include "run.h"

int main (int argc, char ** argv, char ** envp)
{
	struct wordlist words = {NULL, NULL, 0};
	struct command * command;
	struct programStatus status;

	status.argc = argc;
	status.argv = argv;
	status.envp = envp;
	status.pgid = getpgrp();
	status.pid = getpid();

	if (argc == 2 && strcmp(argv[1], "-e") == 0)
		status.justEcho = 1;
	else
		status.justEcho = 0;

	while (1)
	{
		checkZombies();

		status.parse = parse(&words);

		if (status.parse == PARSE_ST_ERROR_QUOTES)
		{
			echoError(ERROR_QUOTES);
			break;
		}
		/*
		 *else if (status.parse == PARSE_ST_ERROR_AMP)
		 *{
		 *  echoError(ERROR_AMP);
		 *  clearWordList(&words);
		 *  continue;
		 *}
		 */
		command = genCommand(&words);

		if (command)
		{
			status.internal = checkInternalCommands(&status, command);

			if (status.internal == INTERNAL_COMMAND_BREAK)
				break;
			else if (status.internal == INTERNAL_COMMAND_OK)
				run(&status, command);
			/* else if internalStatus == INTERNAL_COMMAND_CONTINUE */

			if (!command->bg)
				delCommand(&command);
		}

		clearWordList(&words);

		if (status.parse == PARSE_ST_EOF)
			break;
	}

	clearWordList(&words);

	return 0;
}
