#ifndef _INTERNALS_H_
#define _INTERNALS_H_

#include "main.h"
#include "command.h"

#define INTERNAL_COMMAND_SUCCESS 0
#define INTERNAL_COMMAND_FAILURE 1
#define INTERNAL_COMMAND_CONTINUE 2

int checkInternalCommands(simpleCmd *);

int runExit();
int runCD(simpleCmd *);
/*
 *int runJobs(simpleCmd *);
 *int runJobsBG(simpleCmd *);
 *int runJobsFG(simpleCmd *);
 */

#endif
