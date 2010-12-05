#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define VERSION "0.01"
#define NAME "Manager game server"

#define DEFAULT_PASSKEY "msucmc"

struct programStatus {
	int help,
		version,
		debug,
		daemon,
		syslog;

	int port;
	char * passkey;

	int argc;
	char ** argv;
	char ** envp;
};

extern struct programStatus cfg;

void endWork(int);

#include "echoes.h"

#endif
