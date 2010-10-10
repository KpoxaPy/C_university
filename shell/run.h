#ifndef DEBUG
#define DEBUG

#include "words.h"

struct command {
  char * file;
  int bg;
  char ** argv;
};

int runFG(struct command * );
struct command * genCommand(struct wordlist *);
void delCommand(struct command ** );

#endif
