#include <stdarg.h>
#include <syslog.h>
#include "echoes.h"

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
