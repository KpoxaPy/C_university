#include <unistd.h>
#include "main.h"
#include "echoes.h"
#include "parser/lexer.h"
#include "parser/lexlist.h"

/*char testStr[] = "w1| w2	>> w3 && (abc ; def & noh);woe  && olo || salsa ; tekila &";*/

int main (int argc, char ** argv, char ** envp)
{
	struct programStatus status;
	Lex * lex;
	lexList * list;

	status.argc = argc;
	status.argv = argv;
	status.envp = envp;
	status.pgid = getpgrp();
	status.pid = getpid();

	if (argc == 2 && strcmp(argv[1], "-e") == 0)
		status.justEcho = 1;
	else
		status.justEcho = 0;


	/*initLexerByString(testStr);*/
	initLexer();
	list = newLexList();

	for (;;)
	{
		clearLexList(list);

		echoPromt(PROMT_DEFAULT);

		for (;;)
		{
			lex = getlex();

			if (lex != NULL)
				addLex(list, lex);
			else
				break;

			if (lex->type == LEX_EOF || lex->type == LEX_EOL)
				break;
		}

		if (lex != NULL)
			echoLexList(list);
		else
			printf("Inner error!\n");

		if(lex == NULL || lex->type == LEX_EOF)
			break;
	}
	clearLexList(list);

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
