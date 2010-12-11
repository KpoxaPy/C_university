#ifndef _MAIN_H_
#define _MAIN_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>

#define VERSION "0.01"
#define NAME "Manager game client"

struct programStatus {
	int help,
		version,
		debug;

	char * port;
	char * host;
	int sfd;

	int argc;
	char ** argv;
	char ** envp;

	int terminal;
	int isInteractive;
};

extern struct programStatus cfg;

void endWork(int);

#include "echoes.h"

#endif
