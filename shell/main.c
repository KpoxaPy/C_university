#include <unistd.h>
#include "main.h"
#include "echoes.h"
#include "parser/parser.h"

/*char testStr[] = "w1| w2	>> w3 && (abc ; def & noh);woe  && olo || salsa ; tekila &";*/

int main (int argc, char ** argv, char ** envp)
{
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

	initParser();

	for(;;)
	{
		int pStatus;
		tCmd * cmd;

		echoPromt(PROMT_DEFAULT);
		pStatus = parse(&cmd);

		if (pStatus == PS_OK)
		{
			echoCmdTree(cmd);
		}
		else if (pStatus == PS_ERROR)
		{
			break;
		}
		else if (pStatus == PS_EOF)
		{
			delTCmd(&cmd);
			break;
		}
	}

	clearParser();


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
