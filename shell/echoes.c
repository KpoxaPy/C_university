#include <stdio.h>
#include "echoes.h"

const char promtFormat[]    = "@ ";
const char promtFormatExt[] = "> ";

void echoPromt(int num)
{
  switch (num)
  {
    case 1:
      printf(promtFormat);
      break;
    case 2:
      printf(promtFormatExt);
      break;
  }
}

void echoError(int errno)
{
  switch (errno)
  {
    case 1:
      printf("Error: unbalansed quotes!\n");
      break;
  }
}
