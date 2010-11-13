#ifndef _ECHOES_H_
#define _ECHOES_H_

#define PROMT_DEFAULT 1
#define PROMT_EXTENDED 2
#define PROMT_LEAVING 3

#define ERROR_QUOTES 1
#define ERROR_AMP 2

#include "main.h"

void echoPromt(int);
void echoError(int);

void echoParserError(void);
void echoLexerError(void);

#endif
