#ifndef _MAIN_H_
#define _MAIN_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>

typedef int jid_t;

struct programStatus {
	int parse,
		quiet,
		justHelp,
		justEcho,
		wideEcho,
		debug;

	int argc;
	char ** argv;
	char ** envp;

	pid_t pid;
	pid_t pgid;

	struct termios tmodes;
	int terminal;
	int isInteractive;
};

extern struct programStatus prStatus;

void endWork(int);

#endif
