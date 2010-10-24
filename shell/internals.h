#ifndef _INTERNALS_H_
#define _INTERNALS_H_

#include "main.h"
#include "command.h"
#include "words.h"

int runEcho(struct programStatus * , struct wordlist * );
int runExit(struct programStatus * );
int runCD(struct programStatus * , struct command * );
int runJobs(struct programStatus * , struct command * );
int runJobsBG(struct programStatus * , struct command * );
int runJobsFG(struct programStatus * , struct command * );

#endif
