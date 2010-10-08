#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
#include "buffer.h"

int parseCharToWord(char c, char ** str)
{
  static struct bufferlist buffer = {NULL, NULL, 0};
  static int isWord = 0;
  static int isInQuote = 0;
  static int isEscape = 0;
  int subjectCode = 0;
  *str = NULL;
  int doHandle = 1;

  // printf("{'%c' w%d q%d s%d-> ", c, isWord, isInQuote, isEscape);

  if (isEscape)
  {
    doHandle = 0;
    isEscape = 0;
    if (c == EOF)
      doHandle = 1;
    else if (c == '\n')
      subjectCode = -1;
    else
    {
      addChar(&buffer, c); 
      isWord = 1;
    }
  }

  if (doHandle)
    switch (c)
    {
      case EOF:
        if (isInQuote)
        {
          subjectCode = 1;
          break;
        }
      case '\n':
        if (isWord && !isInQuote)
        {
          isWord = 0;
          isInQuote = 0;
          *str = flushBuffer(&buffer);
        }
        if (isInQuote)
          subjectCode = -1;
        break;
      case '\\':
        isEscape = 1;
        break;
      case '\t':
      case ' ':
        if (!isInQuote && isWord)
        {
          *str = flushBuffer(&buffer);
          isWord = 0;
        }
        else if (isInQuote)
          addChar(&buffer, c);
        break;
      case '"':
        isInQuote = !isInQuote;
        if (isInQuote)
          isWord = 1;
        break;
      default:
        isWord = 1;
        addChar(&buffer, c);
        break;
    }
  // printf("w%d q%d s%d}", isWord, isInQuote, isEscape);

  return subjectCode;
}
