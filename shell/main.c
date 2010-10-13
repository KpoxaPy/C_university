#include <stdlib.h>
#include "words.h"
#include "parser.h"
#include "echoes.h"
#include "run.h"

int main (int argc, char ** argv, char ** envp)
{
	struct wordlist words = {NULL, NULL, 0};
	int parseStatus, internalStatus;
	struct command * command;

	while (1)
	{
		parseStatus = parse(&words);

		if (parseStatus == PARSE_ST_ERROR_QUOTES)
		{
			echoError(ERROR_QUOTES);
			break;
		}

		internalStatus = checkInternalCommands(&words);

		if (internalStatus == INTERNAL_COMMAND_BREAK)
			break;
		else if (internalStatus == INTERNAL_COMMAND_OK)
		{
			command = genCommand(&words);
			runFG(command);
			delCommand(&command);
		}
		/* else if internalStatus == INTERNAL_COMMAND_CONTINUE */

		/*echoWordList(&words);*/

		clearWordList(&words);

		if (parseStatus == PARSE_ST_EOF)
			break;
	}

	clearWordList(&words);

	return 0;
}
