#include <string.h>
#include "words.h"
#include "parser.h"
#include "echoes.h"
#include "run.h"

int main (int argc, char ** argv, char ** envp)
{
  struct wordlist words = {NULL, NULL, 0};
  int parseStatus;
  struct command * command;

  while (1)
  {
    parseStatus = parse(&words);

    if (parseStatus == PARSE_ST_ERROR_QUOTES)
    {
      echoError(ERROR_QUOTES);
      break;
    }

    if (words.first != NULL && strcmp(words.first->str, "exit") == 0)
      break;

    command = genCommand(&words);
    runFG(command);
    delCommand(&command);

    clearWordList(&words);

    if (parseStatus == PARSE_ST_EOF)
      break;
  }

  clearWordList(&words);

  return 0;
}
