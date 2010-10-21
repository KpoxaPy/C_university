#ifndef DEBUG
#define DEBUG

#include "main.h"
#include "words.h"

#define INTERNAL_COMMAND_OK 0
#define INTERNAL_COMMAND_BREAK 1
#define INTERNAL_COMMAND_CONTINUE 2

struct command {
	char * file;
	int bg;
	int argc;
	char ** argv;
	struct wordlist * words;
};

int run(struct command * );
struct command * genCommand(struct wordlist *);
void delCommand(struct command ** );
int checkInternalCommands(struct programStatus *, struct command *);

#endif
