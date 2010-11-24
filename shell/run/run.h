#ifndef _RUN_H_
#define _RUN_H_

#include "../main.h"
#include "command.h"

typedef struct Task {
	tCmd * root;
	tCmd * cur;
} Task;

void processCmdTree(tCmd *);

#endif
