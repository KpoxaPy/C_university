#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <syslog.h>
#include "server.h"

struct programStatus cfg;

void initServer(void);
void daemonizeServer(void);
void initLogging(void);
void initOptions(void);
void initProgram(int, char **, char **);

void mySignal(int);

void printUsage(void);
void printManual(void);
void printAbout(void);
void printDebug(void);
void printHelp(void);
void printVersion(void);

int main (int argc, char ** argv, char ** envp)
{
	initProgram(argc, argv, envp);

	for(;;)
		;

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
	cfg.daemon = 0;
	cfg.version = 0;
	cfg.syslog = 0;

	cfg.port = 0;
	cfg.passkey = NULL;

	initOptions();

	printDebug();
	printHelp();
	printVersion();

	initServer();
}

void initServer()
{
	signal(SIGINT, mySignal);

	daemonizeServer();
	initLogging();
}

void daemonizeServer()
{
	int fd;
	struct rlimit flim;

	if (!cfg.daemon)
		return;

	if (getppid() != 1)
	{
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);

		if (fork() != 0)
			exit(EXIT_SUCCESS);

		setsid();
	}

	getrlimit(RLIMIT_NOFILE, &flim);
	for (fd = 0; fd < flim.rlim_max; fd++)
		close(fd);

	chdir("/");
}

void initLogging()
{
	if (!cfg.syslog)
		return;

	openlog(NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Server has been started...");
	closelog();
}

void endWork(int status)
{
	info("Server is switching off. Thanks for using!\n");
	exit(status);
}

void initOptions(void)
{
	int opt, longOptInd;
	struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"port", 1, 0, 'p'},
		{"syslog", 0, 0, 0},
		{"key", 1, 0, 0},
		{0, 0, 0, 0}
	};

	while (1)
	{
		opt = getopt_long(cfg.argc, cfg.argv,
			"dhvp:s", long_options, &longOptInd);

		if (opt == -1)
			break;

		switch (opt)
		{
			case 0:
				switch (longOptInd)
				{
					case 2: /*  --syslog  */
						cfg.syslog = 1;
						break;
					case 3: /* --key  */
						cfg.passkey = optarg;
				}
				break;

			case 'd':
				cfg.debug = 1;
				break;

			case 'h':
				cfg.help = 1;
				break;

			case 's':
				cfg.daemon = 1;
				break;

			case 'p':
				cfg.port = atoi(optarg);
				if (cfg.port == 0)
					cfg.port = -1;
				break;

			case 'v':
				cfg.version = 1;
				break;

			default:
				printUsage();
				exit(EXIT_FAILURE);
		}
	}

	if (optind < cfg.argc)
	{
		error("%s: too many args\n", cfg.argv[0]);
		printUsage();
		exit(EXIT_FAILURE);
	}

	/* If we will daemonize server we need to echoing in syslog */
	if (cfg.daemon && !cfg.syslog)
		cfg.syslog = 1;

	if (cfg.port <= 0)
	{
		if (cfg.port < 0)
			error("%s: port is wrong, aborted.\n", cfg.argv[0]);
		else
			error("%s: port is unset, aborted.\n", cfg.argv[0]);
		printUsage();
		exit(EXIT_FAILURE);
	}

	if (cfg.passkey == NULL)
	{
		error("%s: passkey doesn't set manually so passkey is set on default value (see help to know it)\n", cfg.argv[0]);
		cfg.passkey = DEFAULT_PASSKEY;
	}
}

/*------------------------------------------------------------------*/

void mySignal(int sig)
{
	switch (sig)
	{
		case SIGINT:
			info("Server got SIGINT.\n");
			endWork(EXIT_SUCCESS);
			break;
	}
}

/*------------------------------------------------------------------*/
/* Printing functions */

void printVersion()
{
	if (cfg.version)
	{
		printAbout();
		endWork(EXIT_SUCCESS);
	}
}

void printDebug()
{
	if (cfg.debug)
	{
		if (cfg.debug == 1)
			debug("Used debug option\n");
		if (cfg.daemon == 1)
			debug("Used server mode option\n");
		if (cfg.syslog == 1)
			debug("Echoing will be done into syslog\n");

		debug("Server port: %d\n", cfg.port);

		debug("Passkey is \"%s\"\n", cfg.passkey);
	}
}

void printHelp()
{
	if (cfg.help)
	{
		printManual();
		endWork(EXIT_SUCCESS);
	}
}

void printAbout()
{
	static char about[] =
		"Simple server for Manager game.\n"
		"Version v" VERSION ".\n"
		"Written by Andrew Ozmitel (aozmitel@gmail.com) in 2010.\n";

	info("%s", about);
}

void printUsage()
{
	info("Usage: %s [-sdhv] -p [4m[22mport[0m\n", cfg.argv[0]);
}

void printManual()
{
	static char help[] =
		"Simple server for Manager game.\n\n"
		"There're flags to use:\n"
		"  -p, --port [4m[22mport[0m	     server will be waiting for players on this [4m[22mport[0m\n"
		"  -s			     server mode -- server process will be demonized\n"
		"  -d			     debug mode -- more debugging output\n"
		"  -h, --help		     just output this message\n"
		"  -v, --version		     print version\n"
		"  --syslog		     use syslog for echoing\n"
		"\n"
		"SERVER MODE\n"
		"When game server is in server mode it can be switched off by sending signal SIGINT.\n";

	printUsage();
	info("\n%s", help);
}
