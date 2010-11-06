#ifndef _PARSER_H_
#define _PARSER_H_

#include "../main.h"
#include "../command.h"

#define PARSE_ST_ERROR_QUOTES 1
#define PARSE_ST_OK 0
#define PARSE_ST_MORE -1
#define PARSE_ST_EOS -2
#define PARSE_ST_EOF -3

int parse(struct cmdElem **);

int processParsingErrors(int);

#endif
