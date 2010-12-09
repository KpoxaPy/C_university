#include <stdarg.h>
#include <syslog.h>
#include <alloca.h>
#include <errno.h>
#include "echoes.h"

/*------------------------------------------------------------------*/
/*                       Logging functions                          */
/*------------------------------------------------------------------*/

void initLogging()
{
	if (!cfg.syslog)
		return;

	openlog(NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Server has been started...");
	closelog();
}

int debug(char * fmt, ...)
{
	va_list ap;
	int ret;

	if (!cfg.debug)
		return 0;

	va_start(ap, fmt);
	if (cfg.syslog)
	{
		openlog(NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
		vsyslog(LOG_DEBUG, fmt, ap);
		closelog();
		ret = 0;
	}
	else
		ret = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return ret;
}

int error(char * fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	if (cfg.syslog)
	{
		openlog(NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
		vsyslog(LOG_ERR, fmt, ap);
		closelog();
		ret = 0;
	}
	else
		ret = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return ret;
}

int merror(char * fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	if (cfg.syslog)
	{
		int len = strlen(fmt);
		char * mes = alloca(len+4);
		if (mes != NULL)
		{
			strcpy(fmt, mes);
			strcpy(": %m", mes+len-4);
		}
		else
			mes = fmt;

		openlog(NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
		vsyslog(LOG_ERR, mes, ap);
		closelog();
		ret = 0;
	}
	else
	{
		int i;
		i = vfprintf(stderr, fmt, ap);
		ret = fprintf(stderr, ": %s\n", strerror(errno));
		ret += i;
	}
	va_end(ap);

	return ret;
}

int info(char * fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	if (cfg.syslog)
	{
		openlog(NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
		vsyslog(LOG_INFO, fmt, ap);
		closelog();
		ret = 0;
	}
	else
	ret = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return ret;
}

/*------------------------------------------------------------------*/
/*                      Printing functions                          */
/*------------------------------------------------------------------*/

void printVersion()
{
	if (cfg.version)
	{
		printAbout();
		exit(EXIT_SUCCESS);
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

		debug("Passkey: \"%s\"\n", cfg.passkey);
	}
}

void printHelp()
{
	if (cfg.help)
	{
		printManual();
		exit(EXIT_SUCCESS);
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
	info("Usage: %s [-sdhv] -p [4m[22mport[0m [--key [4m[22mpasskey[0m]\n", cfg.argv[0]);
}


/*------------------------------------------------------------------*/
/*                        Manual section                            */
/*------------------------------------------------------------------*/

char * manualServerMode(void);
char * manualAdministration(void);

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
		"  --key [4m[22mpasskey[0m		     set passkey for remote administration\n";

	printUsage();
	info("\n%s\n%s\n%s", help, manualServerMode(), manualAdministration());
}

char * manualServerMode()
{
	static char ret[] =
		"SERVER MODE\n"
		"When game server is in server mode it can be switched off by sending signal SIGINT.\n";

	 return ret;
}

char * manualAdministration()
{
	static char ret[] =
		"ADMINISTRATION\n"
		"There is available remote administration. To get this capabilities you should know [4m[22mpasskey[0m.\n"
		"\n"
		"Passkey is setting when server is running or if it did't set this way it'll is set into default value \"" DEFAULT_PASSKEY "\".\n";

	 return ret;
}
