#ifndef _GAME_H_
#define _GAME_H_

#include "main.h"
#include "../stuff/buffer.h"

typedef struct game {
	int num;

	struct managedPlayer * players;
} Game;

typedef struct managedGame {
	struct game * game;
	struct managedGame * next;
} mGame;

typedef struct player {
	int fd;
	int pid;

	Buffer * buf;
	struct game * game;
} Player;

typedef struct managedPlayer {
	struct player * player;
	struct managedPlayer * next;
} mPlayer;

Game * newGame(void);
void remGame(Game *);

Player * newPlayer(int);
void remPlayer(Player *);
void freePlayer(Player *);

void fetchData(Player *);
void checkPlayerOnCommand(Player *);

#endif
