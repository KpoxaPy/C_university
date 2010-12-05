#ifndef _ECHOES_H_
#define _ECHOES_H_

#include "main.h"

void initLogging(void);

void printUsage(void);
void printManual(void);
void printAbout(void);
void printDebug(void);
void printHelp(void);
void printVersion(void);

int error(char *, ...);
int info(char *, ...);
int debug(char *, ...);

#endif
