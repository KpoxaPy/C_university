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

	if (argc == 2 && strcmp(argv[1], "-e") == 0)
		status.justEcho = 1;
	else
		status.justEcho = 0;

	while (1)
	{
		status.parse = parse(&words);

		if (status.parse == PARSE_ST_ERROR_QUOTES)
		{
			echoError(ERROR_QUOTES);
			break;
		}

		status.internal = checkInternalCommands(&status, &words);

		if (status.internal == INTERNAL_COMMAND_BREAK)
			break;
		else if (status.internal == INTERNAL_COMMAND_OK)
		{
			command = genCommand(&words);
			if (command)
			{
				runFG(command);
				delCommand(&command);
			}
		}
		/* else if internalStatus == INTERNAL_COMMAND_CONTINUE */


		clearWordList(&words);

		if (status.parse == PARSE_ST_EOF)
			break;
	}

	clearWordList(&words);

	return 0;
}
