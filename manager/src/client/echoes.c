#include <stdarg.h>
#include <syslog.h>
#include <alloca.h>
#include <errno.h>
#include "echoes.h"
#include "client.h"
#include "lexer.h"
#include "parser.h"

const char promtFormat[]    = "@ ";
const char promtFormatExt[] = "> ";

void echoErrorLex(void);
void echoErrorSymbol(void);

void echoPromt(int num)
{
	switch (num)
	{
		case PROMT_DEFAULT:
			printf(promtFormat);
			fflush(stdout);
			break;
		case PROMT_EXTENDED:
			printf(promtFormatExt);
			break;
		case PROMT_LEAVING:
			printf("\n");
			break;
	}
}

void echoParserError(void)
{
	char errHead[] = "Parser error: ";

	switch (parserErrorNo)
	{
		case PE_UNEXPECTED_END_OF_COMMAND:
			fprintf(stderr, "%sUnexpected end of command\n", errHead);
			echoErrorLex();
			break;
		case PE_NULL_POINTER:
			fprintf(stderr, "%s(Internal) Parser getted null pointer to tCmd\n", errHead);
			echoErrorLex();
			break;
		case PE_EXPECTED_WORD:
			fprintf(stderr, "%sExpected word\n", errHead);
			echoErrorLex();
			break;
		case PE_LEXER_ERROR:
			echoLexerError();
			break;
		case PE_NONE:
		default:
			break;
	}
}

void echoLexerError(void)
{
	char errHead[] = "Lexer error: ";

	switch (lexerErrorNo)
	{
		case LE_UNEXPECTED_SYMBOL:
			fprintf(stderr, "%sUnexpected symbol\n", errHead);
			echoErrorSymbol();
			break;
		case LE_UNCLOSED_QUOTES:
			fprintf(stderr, "%sUnclosed quotes\n", errHead);
			echoErrorSymbol();
			break;
		case LE_UNKNOWN_LEXER_STATE:
			fprintf(stderr, "%s(Internal) Lexer went to unknown state\n", errHead);
			echoErrorSymbol();
			break;
		case LE_NONE:
		default:
			break;
	}
}

void echoErrorLex(void)
{
	if (errorLex == NULL)
	{
		fprintf(stderr, "There's no error lex.\n");
		return;
	}

	fprintf(stderr, "Error lex: %s", lexTypeStr[errorLex->type]);

	if (errorLex->str != NULL)
		fprintf(stderr, " \"%s\"\n", errorLex->str);
	else
		fprintf(stderr, "\n");
}

void echoErrorSymbol(void)
{
}

/*------------------------------------------------------------------*/
/*                       Logging functions                          */
/*------------------------------------------------------------------*/

int debug(char * fmt, ...)
{
	va_list ap;
	int ret;

	if (!cfg.debug)
		return 0;

	va_start(ap, fmt);
	ret = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return ret;
}

int debugl(int level, char * fmt, ...)
{
	va_list ap;
	int ret;

	if (!cfg.debug)
		return 0;

	if (level > cfg.debugLevel)
		return 0;

	va_start(ap, fmt);
	ret = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return ret;
}

int error(char * fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return ret;
}

int merror(char * fmt, ...)
{
	va_list ap;
	int ret;
	int i;

	va_start(ap, fmt);
	i = vfprintf(stderr, fmt, ap);
	ret = fprintf(stderr, ": %s\n", strerror(errno));
	ret += i;
	va_end(ap);

	return ret;
}

int info(char * fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
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
	int i;

	if (cfg.debug)
	{
		if (cfg.debug == 1)
			debug("Used debug option\n");

		debug("Requested server port: %s\n", cli.portstr);
		debug("Requested server host: %s\n", cli.hoststr);
		debug("Got server port: %d\n", cli.port);
		debug("Got server host: ");
			for (i = 0; i < 4; i++)
				debug("%hhu%s", ((char *)&(cli.host))[i], (i != 3 ? "." : ""));
		debug("\n");
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
		"Simple client for Manager game.\n"
		"Version v" VERSION ".\n"
		"Written by Andrew Ozmitel (aozmitel@gmail.com) in 2010.\n";

	info("%s", about);
}

void printUsage()
{
	info("Usage: %s [-dhv] -p [4m[22mport[0m -a [4m[22mhost[0m\n", cfg.argv[0]);
}


/*------------------------------------------------------------------*/
/*                        Manual section                            */
/*------------------------------------------------------------------*/

char * manualAdministration(void);

void printManual()
{
	static char help[] =
		"Simple client for Manager game.\n\n"
		"There're flags to use:\n"
		"  -p, --port [4m[22mport[0m	     server will be waiting for players on this [4m[22mport[0m\n"
		"  -s			     server mode -- server process will be demonized\n"
		"  -d			     debug mode -- more debugging output\n"
		"  -h, --help		     just output this message\n"
		"  -v, --version		     print version\n"
		"  --syslog		     use syslog for echoing\n"
		"  --key [4m[22mpasskey[0m		     set passkey for remote administration\n";

	printUsage();
	info("\n%s\n%s", help, manualAdministration());
}

char * manualAdministration()
{
	static char ret[] =
		"ADMINISTRATION\n"
		"There is available remote administration. To get this capabilities you should know [4m[22mpasskey[0m.\n"
		"\n"
		"Passkey was set when server has been started.\n";

	 return ret;
}
