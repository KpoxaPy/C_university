#ifndef _RUN_H_
#define _RUN_H_

#include "main.h"
#include "jobmanager.h"
#include "command.h"

#define INTERNAL_COMMAND_OK 0
#define INTERNAL_COMMAND_BREAK 1
#define INTERNAL_COMMAND_CONTINUE 2

int run(struct programStatus *, struct command * );
int checkInternalCommands(struct programStatus *, struct command *);

#endif
