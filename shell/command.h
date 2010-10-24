#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "main.h"
#include "words.h"

struct command {
	char * file;
	int bg;
	int argc;
	char ** argv;
	struct wordlist * words;
};

struct command * genCommand(struct wordlist *);
void delCommand(struct command ** );
void echoCommand(struct command *);

#endif
