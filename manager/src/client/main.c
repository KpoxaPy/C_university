#include <getopt.h>
#include <signal.h>
#include <mcheck.h>
#include "main.h"
#include "client.h"

struct programStatus cfg;

void initShell(void);
void initOptions(void);
void initProgram(int, char **, char **);

void printUsage(void);
void printDebug(void);

int main (int argc, char ** argv, char ** envp)
{
	int stC, stS;
	initProgram(argc, argv, envp);

	echoPromt(PROMT_DEFAULT);

	for(;;)
	{
		pollClient();
		stC = checkClientOnCommand();
		stS = checkServerOnInfo();

		if (stC != -1 && !cli.waitingForResponse)
			echoPromt(PROMT_DEFAULT);

		if (stC == -1)
			break;

		if (stS == -1)
		{
			info("Server disconnected!\n");
			break;
		}
	}

	echoPromt(PROMT_LEAVING);

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

	cfg.debugLevel = DFL_DBG_LVL;

	mtrace();

	initOptions();

	printHelp();
	printVersion();

	initClient();

	printDebug();
}

void endWork(int status)
{
	unconnect();

	if (status == EXIT_SUCCESS)
		info("Client is switching off. Thanks for using!\n");
	else
		error("Client made fatal error. Aborting by problem.\n");

	muntrace();

	exit(status);
}
