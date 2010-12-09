#ifndef _SERVER_H_
#define _SERVER_H_

#include "main.h"
#include "game.h"

void initServer(void);
void shutdownServer(void);

mGame * addGame(Game *);
void delGame(Game *);

void pollServer(void);
void checkCommands(void);
void processEvents(void);

#endif
