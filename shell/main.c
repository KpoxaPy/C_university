#include <unistd.h>
#include "main.h"
#include "echoes.h"
#include "parser/parser.h"

struct programStatus prStatus;

void initProgram(int, char **, char **);
void PrintUsage(void);

int main (int argc, char ** argv, char ** envp)
{
	initProgram(argc, argv, envp);

	initParser();

	for(;;)
	{
		tCmd * cmd;

		echoPromt(PROMT_DEFAULT);
		prStatus.parse = parse(&cmd);

		if (prStatus.parse == PS_OK)
		{
			if (prStatus.justEcho)
				echoCmdTree(cmd);
			else
				/*processCmdTree(cmd);*/
				;
		}
		else if (prStatus.parse == PS_ERROR)
		{
			/*processParsingErrors(prStatus.parse);*/
			echoParserError();
			break;
		}
		else if (prStatus.parse == PS_EOF)
		{
			delTCmd(&cmd);
			echoPromt(PROMT_LEAVING);
			break;
		}
	}

	clearParser();

	return EXIT_SUCCESS;
}

void initProgram(int argc, char ** argv, char ** envp)
{
	int opt;

	prStatus.argc = argc;
	prStatus.argv = argv;
	prStatus.envp = envp;
	prStatus.pgid = getpgrp();
	prStatus.pid = getpid();

	prStatus.justEcho = 0;
	prStatus.quiet = 0;

	while ((opt = getopt(argc, argv, "eq")) != -1)
	{
		switch (opt)
		{
			case 'e':
				prStatus.justEcho = 1;
				break;
			case 'q':
				prStatus.quiet = 1;
				break;
			default:
				PrintUsage();
				exit(EXIT_FAILURE);
		}
	}

	if (optind < argc)
	{
		fprintf(stderr, "%s: too many args\n", argv[0]);
		PrintUsage();
		exit(EXIT_FAILURE);
	}
}

void PrintUsage(void)
{
	static char help[] =
		"Flags mean:\n"
		"	-e	Just echo parsed strings, not execute anything\n"
		"	-q	Quiet work -- without promting\n";

	fprintf(stderr, "Usage: %s [-e] [-q]\n", prStatus.argv[0]);
	fprintf(stderr, "%s", help);
}
