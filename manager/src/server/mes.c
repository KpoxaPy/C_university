#include "mes.h"

Queue queue = {NULL, NULL, 0};

char * meststr[] = {
	"MEST_ZERO",
	"MEST_PLAYER_JOIN_SERVER",
	"MEST_PLAYER_LEAVE_SERVER",
	"MEST_PLAYER_CHANGES_NICK",

	"MEST_PLAYER_JOIN_GAME",
	"MEST_PLAYERS_STAT_GAME",
	"MEST_PLAYER_LEAVE_GAME",
	"MEST_GAME_OVER",

	"MEST_NEW_GAME",
	"MEST_PLAYER_JOINS_HALL",
	"MEST_PLAYER_LEAVES_HALL",

	"MEST_PLAYER_KICKED_FROM_GAME",
	"MEST_PLAYER_KICKED_FROM_SERVER",
	"MEST_PLAYER_PID",

	"MEST_COMMAND_UNKNOWN",
	"MEST_COMMAND_GET",
	"MEST_COMMAND_SET",
	"MEST_COMMAND_JOIN",
	"MEST_COMMAND_LEAVE",
	"MEST_COMMAND_NICK",
	"MEST_COMMAND_ADM",
	"MEST_COMMAND_GAMES",
	"MEST_COMMAND_PLAYERS",
	"MEST_COMMAND_CREATEGAME",
	"MEST_COMMAND_DELETEGAME",
	"MEST_COMMAND_PLAYER",
	"MEST_COMMAND_START",
	"MEST_COMMAND_TURN",
	"MEST_COMMAND_PROD",
	"MEST_COMMAND_MARKET",
	"MEST_COMMAND_BUILD",
	"MEST_COMMAND_BUY",
	"MEST_COMMAND_SELL",

	"MEST_START_GAME",
	"MEST_TURN_ENDED",
	"MEST_PLAYER_BANKRUPT",
	"MEST_PLAYER_WIN"
};

void sendMessage(Message * mes)
{
	mMessage * mm = (mMessage *)malloc(sizeof(mMessage));

	mm->mes = (Message *)malloc(sizeof(Message));
	memcpy(mm->mes, mes, sizeof(Message));

	mm->next = queue.in;
	mm->prev = NULL;
	if (queue.in != NULL)
		(queue.in)->prev = mm;
	else
		queue.out = mm;
	queue.in = mm;
	++queue.count;

	debug("Sent message %s from %s to %s\n", meststr[mes->type],
		(mes->sndr_t == O_SERVER ? "SERVER" : (mes->sndr_t == O_GAME ? "GAME" : "PLAYER")),
		(mes->rcvr_t == O_SERVER ? "SERVER" : (mes->rcvr_t == O_GAME ? "GAME" : "PLAYER")));
}

int getMessage(Message * mes)
{
	mMessage * mm;

	if (queue.count == 0)
		return -1;

	mm = queue.out;
	memcpy(mes, mm->mes, sizeof(Message));
	free(mm->mes);

	if (mm->prev != NULL)
		mm->prev->next = NULL;
	else
		queue.in = NULL;
	queue.out = mm->prev;
	free(mm);
	--queue.count;

	debug("Getted message %s from %s to %s\n", meststr[mes->type],
		(mes->sndr_t == O_SERVER ? "SERVER" : (mes->sndr_t == O_GAME ? "GAME" : "PLAYER")),
		(mes->rcvr_t == O_SERVER ? "SERVER" : (mes->rcvr_t == O_GAME ? "GAME" : "PLAYER")));

	return 0;
}
