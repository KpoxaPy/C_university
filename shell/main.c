#include <unistd.h>
#include "main.h"
/*#include "command.h"*/
#include "parser/parser.h"
#include "parser/lexer.h"
/*#include "run.h"*/

int main (int argc, char ** argv, char ** envp)
{
	/*struct cmdElem * cmdTree;*/
	struct programStatus status;
	Lex * lex;

	status.argc = argc;
	status.argv = argv;
	status.envp = envp;
	status.pgid = getpgrp();
	status.pid = getpid();

	if (argc == 2 && strcmp(argv[1], "-e") == 0)
		status.justEcho = 1;
	else
		status.justEcho = 0;


	initLexer();

	for(;;)
	{
		lex = getlex();
		echoLex(lex);
		if (lex == NULL || lex->type == LEX_EOF)
		{
			if (lex->str != NULL)
				free(lex->str);
			free(lex);
			break;
		}
		if (lex->str != NULL)
			free(lex->str);
		free(lex);
	}

	clearLexer();

	/*while (1)*/
	/*{*/
		/*checkZombies();*/

		/*status.parse = parse(&cmdTree);*/

		/*if (processParsingErrors(status.parse))*/
			/*break;*/

		/*if (cmdTree)*/
			/*processCommand(&status, cmdTree);*/

		/*if (status.parse == PARSE_ST_EOF)*/
			/*break;*/
	/*}*/

	return 0;
}
