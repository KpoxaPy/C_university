#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include "run.h"

int runFG(struct command * com)
{
  int pid;
  int pid_status;

  if ((pid = fork()) == 0)
  {
    execvp(com->file, com->argv);
    perror(strerror(errno));
    exit(1);
  }

  while (wait(&pid_status) != -1)
    ;

  return pid_status;
}

struct command * genCommand(struct wordlist * words)
{
  struct command * cmd;

  if (words->count)
  {
    int i = 0;
    struct word * wtmp = words->first;
    char * tmpStr;

    cmd = (struct command *)malloc(sizeof(struct command));
    cmd->argv = (char **)malloc(sizeof(char *)*(words->count + 1));

    while (wtmp != NULL)
    {
      tmpStr = (char *)malloc(sizeof(char)*(wtmp->len));
      cmd->argv[i++] = strcpy(tmpStr, wtmp->str);
      wtmp = wtmp->next;
    }

    cmd->argv[i] = NULL;
    cmd->file = cmd->argv[0];
    cmd->bg = 0;
  }
  else
    return NULL;

  return cmd;
}

void delCommand(struct command ** com)
{
  if (com != NULL);
  {
    char ** pStr = (*com)->argv;

    while (*pStr != NULL)
      free(*pStr++);

    free((*com)->argv);
    free(*com);

    *com = NULL;
  }
}
