#include <stdio.h>
#include "words.h"
#include "parser.h"
#include "echoes.h"

int main (int argc, char ** argv, char ** envp)
{
  struct wordlist words = {NULL, NULL, 0};
  int parseStatus;

  while (1)
  {
    parseStatus = parse(&words);

    if (parseStatus == PARSE_ST_ERROR_QUOTES)
    {
      echoError(ERROR_QUOTES);
      break;
    }

    echoWordList(&words);
    clearWordList(&words);

    if (parseStatus == PARSE_ST_EOF)
      break;
  }

  return 0;
}
