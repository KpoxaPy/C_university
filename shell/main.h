#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct programStatus {
	short parse,
				internal,
				justEcho;
	int argc;
	char ** argv;
	char ** envp;
	pid_t pid;
	pid_t pgid;
};

#endif
