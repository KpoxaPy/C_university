#ifndef _MAIN_H_
#define _MAIN_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct programStatus {
	short parse,
				internal,
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
};

extern struct programStatus prStatus;

#endif
