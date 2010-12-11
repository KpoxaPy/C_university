#include <getopt.h>
#include <signal.h>
#include "main.h"
#include "echoes.h"
#include "lexer.h"
#include "parser.h"
#include "task.h"
#include "client.h"
#include <mcheck.h>

struct programStatus cfg;

void initShell(void);
void initOptions(void);
void initProgram(int, char **, char **);

void printUsage(void);
void printDebug(void);

int main (int argc, char ** argv, char ** envp)
{
	int stParse;
	Task * task = NULL;

	initProgram(argc, argv, envp);

	for(;;)
	{
		echoPromt(PROMT_DEFAULT);

		delTask(&task);
		stParse = parse(&task);

		if (stParse == PS_OK)
		{
			defineTaskType(task);

			if (task->type != TASK_UNKNOWN)
				sendCommand(task);
			else
				error("Unknown command!\n");
		}
		else if (stParse == PS_ERROR)
		{
			echoParserError();
		}
		else if (stParse == PS_EOF)
		{
			delTask(&task);
			echoPromt(PROMT_LEAVING);
			break;
		}
	}

	endWork(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

void initProgram(int argc, char ** argv, char ** envp)
{
	cfg.argc = argc;
	cfg.argv = argv;
	cfg.envp = envp;
	cfg.help = 0;
	cfg.debug = 0;
	cfg.version = 0;

	cfg.port = NULL;
	cfg.host = NULL;
	cfg.sfd = -1;

	mtrace();

	initOptions();

	printDebug();
	printHelp();
	printVersion();

	initParser();

	initClient();
}

void endWork(int status)
{
	clearParser();

	unconnect();

	if (status == EXIT_SUCCESS)
		info("Client is switching off. Thanks for using!\n");
	else
		error("Client made fatal error. Aborting by problem.\n");

	muntrace();

	exit(status);
}
