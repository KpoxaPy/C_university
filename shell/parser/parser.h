#ifndef _PARSER_H_
#define _PARSER_H_

#include "../command.h"

#define PS_ERROR -1
#define PS_OK 0
#define PS_EOL 1
#define PS_EOF 2

#define PE_NONE 0
#define PE_UNEXPECTED_END_OF_COMMAND 1
#define PE_NULL_POINTER 2
#define PE_EXPECTED_RPAREN 3
#define PE_EXPECTED_WORD 4
/*#define PE_*/

int parse(tCmd **);

void initParser();
void initParserByString(char *);
/*void initParserByFile(char *);*/
void clearParser();

#endif
