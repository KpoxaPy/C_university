#ifndef _ECHOES_H_
#define _ECHOES_H_

#include "main.h"

#define PROMT_DEFAULT 1
#define PROMT_EXTENDED 2
#define PROMT_LEAVING 3

void echoPromt(int);
void echoError(int);

void echoLexerError(void);
void echoParserError(void);

void printUsage(void);
void printManual(void);
void printAbout(void);
void printDebug(void);
void printHelp(void);
void printVersion(void);

int error(char *, ...);
int merror(char *, ...);
int info(char *, ...);
int debug(char *, ...);

#endif
