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
#define MEST_PLAYER_JOIN_GAME 3
#define MEST_PLAYER_LEAVE_GAME 4
#define MEST_COMMAND_UNKNOWN 5
#define MEST_COMMAND_GET 6
#define MEST_COMMAND_SET 7
#define MEST_COMMAND_JOIN 8
#define MEST_COMMAND_LEAVE 9
#define MEST_COMMAND_NICK 10

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
