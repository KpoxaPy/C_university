#ifndef _MES_H_
#define _MES_H_

#include "main.h"
#include "game.h"

#define O_SERVER 0
#define O_GAME 1
#define O_PLAYER 2

#define MEST_ZERO 0
#define MEST_PLAYER_JOIN_SERVER 1
#define MEST_PLAYER_LEAVE_SERVER 2
#define MEST_PLAYER_CHANGES_NICK 3

#define MEST_PLAYER_JOIN_GAME 4
#define MEST_PLAYERS_STAT_GAME 5
#define MEST_PLAYER_LEAVE_GAME 6
#define MEST_GAME_OVER 7

#define MEST_NEW_GAME 8
#define MEST_PLAYER_JOINS_HALL 9
#define MEST_PLAYER_LEAVES_HALL 10

#define MEST_PLAYER_KICKED_FROM_GAME 11
#define MEST_PLAYER_KICKED_FROM_SERVER 12
#define MEST_PLAYER_PID 13

#define MEST_COMMAND_UNKNOWN 14
#define MEST_COMMAND_GET 15
#define MEST_COMMAND_SET 16
#define MEST_COMMAND_JOIN 17
#define MEST_COMMAND_LEAVE 18
#define MEST_COMMAND_NICK 19
#define MEST_COMMAND_ADM 20
#define MEST_COMMAND_GAMES 21
#define MEST_COMMAND_PLAYERS 22
#define MEST_COMMAND_CREATEGAME 23
#define MEST_COMMAND_DELETEGAME 24
#define MEST_COMMAND_PLAYER 25
#define MEST_COMMAND_START 26
#define MEST_COMMAND_TURN 27
#define MEST_COMMAND_PROD 28
#define MEST_COMMAND_MARKET 29
#define MEST_COMMAND_BUILD 30
#define MEST_COMMAND_BUY 31
#define MEST_COMMAND_SELL 32

#define MEST_START_GAME 33
#define MEST_TURN_ENDED 34
#define MEST_PLAYER_BANKRUPT 35
#define MEST_PLAYER_WIN 36

typedef struct message {
	int sndr_t;
	union sndr {
		Game * game;
		Player * player;
	} sndr;
	int rcvr_t;
	union rcvr {
		Game * game;
		Player * player;
	} rcvr;
	int type;
	int len;
	void * data;
} Message;

typedef struct managedMessage {
	Message * mes;
	struct managedMessage * next, * prev;
} mMessage;

typedef struct queue {
	mMessage * in, * out;
	int count;
} Queue;

extern Queue queue;

void sendMessage(Message *);
int getMessage(Message *);

#endif
