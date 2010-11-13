#include <unistd.h>
#include <getopt.h>
#include "main.h"
#include "echoes.h"
#include "parser/parser.h"

struct programStatus prStatus;

void initProgram(int, char **, char **);
void printUsage(void);

int main (int argc, char ** argv, char ** envp)
{
	initProgram(argc, argv, envp);

	if (prStatus.justHelp)
	{
		printUsage();
		return EXIT_SUCCESS;
	}

	initParser();

	for(;;)
	{
		tCmd * cmd;

		echoPromt(PROMT_DEFAULT);
		prStatus.parse = parse(&cmd);

		if (prStatus.parse == PS_OK)
		{
			if (prStatus.justEcho && !prStatus.wideEcho)
			{
				char * str = getCmdString(cmd);
				printf("%s\n", str);
				free(str);
			}
			else if (prStatus.justEcho && prStatus.wideEcho)
				echoCmdTree(cmd);
			else
				/*processCmdTree(cmd);*/
				;
		}
		else if (prStatus.parse == PS_ERROR)
		{
			echoParserError();
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
	int opt, longOptInd;
	struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{0, 0, 0, 0}
	};

	prStatus.argc = argc;
	prStatus.argv = argv;
	prStatus.envp = envp;
	prStatus.pgid = getpgrp();
	prStatus.pid = getpid();

	prStatus.justHelp = 0;
	prStatus.justEcho = 0;
	prStatus.wideEcho = 0;
	prStatus.quiet = 0;
	prStatus.debug = 0;

	while (1)
	{
		opt = getopt_long(argc, argv, "dehqw", long_options, &longOptInd);

		if (opt == -1)
			break;

		switch (opt)
		{
			case 0:
				/* Attention! If long options more than one it
				 * is necessary to be there testing of what long
				 * option we get.
				 */
				/* option --help */
				prStatus.justHelp = 1;
				break;

			case 'd':
				prStatus.debug = 1;
				break;

			case 'e':
				prStatus.justEcho = 1;
				break;

			case 'h':
				prStatus.justHelp = 1;
				break;

			case 'q':
				prStatus.quiet = 1;
				break;

			case 'w':
				prStatus.justEcho = 1;
				prStatus.wideEcho = 1;
				break;

			default:
				printUsage();
				exit(EXIT_FAILURE);
		}
	}

	if (optind < argc)
	{
		fprintf(stderr, "%s: too many args\n", argv[0]);
		printUsage();
		exit(EXIT_FAILURE);
	}

	if (prStatus.debug)
	{
		if (prStatus.justEcho == 1)
			printf("Used justEcho option\n");
		if (prStatus.wideEcho == 1)
			printf("Used wideEcho option\n");
		if (prStatus.quiet == 1)
			printf("Used quiet option\n");
		if (prStatus.debug == 1)
			printf("Used debug option\n");
	}
}

void printUsage(void)
{
	static char help[] =
		"There're flags\n"
		"  -d			     debug mode -- more debugging output\n"
		"  -e			     just echo parsed strings, not execute anything\n"
		"  -h, --help		     just output this message\n"
		"  -q			     quiet work -- without promting\n"
		"  -w			     same as -e, but format of output more wide\n";

	fprintf(stderr, "Usage: %s [-dehqw]\n", prStatus.argv[0]);
	fprintf(stderr, "\n%s", help);
}
