#include <stdio.h>
#include "words.h"
#include "echoes.h"
#include "parser.h"

int main (int argc, char ** argv)
{
  struct wordlist words = {NULL, NULL, 0};
  int flagEOF = 0;
  int c;
  int returnCode;
  char * str;

  echoPromt(1);
  while (!flagEOF)
  {
    c = getchar();

    if (c == EOF)
      flagEOF = 1;

    returnCode = parseCharToWord(c, &str);

    if (str != NULL)
      addWord(&words, str);

    if (c == '\n' || c == EOF)
    {
      if (returnCode > 0)
      {
        echoError(returnCode);
        clearWordList(&words);
      }
      else if (!returnCode)
      {
        echoWordList(&words);
        clearWordList(&words);
      }
    }

    if (c == '\n')
    {
      if (returnCode == -1)
        echoPromt(2);
      else
        echoPromt(1);
    }
  }

  return 0;
}
