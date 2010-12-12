#ifndef _MAIN_H_
#define _MAIN_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "../stuff/buffer.h"

#define VERSION "0.1"
#define NAME "Manager game client"

#define DFL_DBG_LVL 3

#define max(x,y) ((x) > (y) ? (x) : (y))

struct programStatus {
	int help,
		version,
		debug,
		debugLevel;

	int argc;
	char ** argv;
	char ** envp;
};

extern struct programStatus cfg;

void endWork(int);

#include "echoes.h"

#endif
