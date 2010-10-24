#ifndef _BGMANAGER_H_
#define _BGMANAGER_H_

#include "main.h"
#include "command.h"
#include "run.h"

typedef int jid_t;

/* Managing jobs:
 *  - adding in manager;
 *  - checking and waiting;
 *  - changing of foreground group;
 */
jid_t addJob(pid_t, struct command *);
int makeFG(jid_t, int *);

/* Changing the last and the penult jid in the list. */
void setPenultByLastJid();
void setLastJid(jid_t);

void checkZombies();
int waitJid(jid_t);
int waitFG(pid_t);

/* Echoing jobs */
void echoJobList();

/* Search jid, pid and command by jid, pid */
pid_t searchPidByJid(jid_t);
struct command * searchCmdByJid(jid_t);

#endif
