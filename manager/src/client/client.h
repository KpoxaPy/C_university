#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "main.h"
#include "task.h"

void initClient(void);
void unconnect(void);

int defineTaskType(Task *);

void sendCommand(Task *);

#endif
