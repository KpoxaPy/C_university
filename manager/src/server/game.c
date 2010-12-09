#include <sys/socket.h>
#include "game.h"
#include "mes.h"

Game * newGame()
{
	Game * g;

	if ((g = (Game *)malloc(sizeof(Game))) == NULL)
		return NULL;

	g->num = 0;
	g->players = NULL;

	return g;
}

void remGame(Game * g)
{
	if (g == NULL)
		return;

	while (g->players != NULL)
		freePlayer(g->players->player);

	free(g);
}


Player * newPlayer(int fd)
{
	Player * p;

	if ((p = (Player *)malloc(sizeof(Player))) == NULL)
		return NULL;

	p->fd = fd;
	p->game = NULL;
	p->buf = newBuffer();
	p->pid = 0;

	return p;
}

void remPlayer(Player * p)
{
	if (p == NULL)
		return;

	if (p->fd >= 0)
	{
		shutdown(p->fd, SHUT_RDWR);
		close(p->fd);
	}

	clearBuffer(p->buf);
	free(p->buf);

	free(p);
}


void fetchData(Player * p)
{
	int n;
	char buf[8];

	if ((n = read(p->fd, buf, 7)) > 0)
	{
		buf[n] = '\0';

		addStr(p->buf, buf);
		debug("From player getted string %s\n", buf);
	}
	else if (n == 0)
	{
		Message mes;
		mes.rcvr_t = RCVR_SERVER;
		mes.type = MEST_PLAYER_LEAVE_SERVER;
		mes.len = 0;
		mes.data = p;
		sendMessage(&mes);
	}
	else
		merror("read on fd=%d\n", p->fd);

	debug("From player read returned %d\n", n);
}

void checkPlayerOnCommand(Player * p)
{
}
