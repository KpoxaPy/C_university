#ifndef _INTERNALS_H_
#define _INTERNALS_H_

#include "main.h"
#include "run.h"
#include "words.h"

int runEcho(struct programStatus * , struct wordlist * );
int runExit(struct programStatus * );
int runCD(struct programStatus * , struct command * );

#endif
