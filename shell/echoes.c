#include <stdio.h>
#include "echoes.h"

const char promtFormat[]    = "@ ";
const char promtFormatExt[] = "> ";

void echoPromt(int num)
{
  switch (num)
  {
    case PROMT_DEFAULT:
      printf(promtFormat);
      break;
    case PROMT_EXTENDED:
      printf(promtFormatExt);
      break;
  }
}

void echoError(int errno)
{
  switch (errno)
  {
    case ERROR_QUOTES:
      printf("Error: unbalansed quotes!\n");
      break;
  }
}
