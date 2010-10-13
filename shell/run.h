#ifndef DEBUG
#define DEBUG

#include "words.h"

#define INTERNAL_COMMAND_OK 0
#define INTERNAL_COMMAND_BREAK 1
#define INTERNAL_COMMAND_CONTINUE 2

struct command {
	char * file;
	int bg;
	char ** argv;
};

int runFG(struct command * );
struct command * genCommand(struct wordlist *);
void delCommand(struct command ** );
int checkInternalCommands(struct wordlist *);

#endif
