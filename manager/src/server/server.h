#ifndef _SERVER_H_
#define _SERVER_H_

#include "main.h"
#include "game.h"

void initServer(void);
void shutdownServer(void);

mGame * addGame(Game *);
void delGame(Game *);
void freeGame(Game *);

mPlayer * addNilPlayer(Player *);
mPlayer * addPlayer(Game *, Player *);
void delNilPlayer(Player * );
void delPlayer(Player *);
void freePlayer(Player *, Game *);

void IntoGame(Game *, Player *);
void IntoNil(Player *);

Game * findGameByGid(int);
Player * findPlayerByPid(int);

void pollServer(void);
void checkCommands(void);
void processEvents(void);

#endif
