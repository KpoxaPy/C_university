#include <sys/socket.h>
#include <netinet/ip.h>
#include "server.h"
#include "mes.h"

struct server {
	int ld;

	mGame * games;
	mPlayer * players; /* non in game players */

	int nGames;
	int nPlayers;
	int hallPlayers;
	int nextPid;
	int started;
} srv;

void startServer(void);
void acceptPlayer(void);

void handleServerEvent(Message *);
void handleGameEvent(Game *, Message *);
void handlePlayerEvent(Player *, Message *);

void acceptPlayer();

void initServer()
{
	srv.ld = -1;
	srv.started = 0;

	srv.nGames = 0;
	srv.nPlayers = 0;
	srv.hallPlayers = 0;
	srv.games = NULL;
	srv.players = NULL;
	srv.nextPid = 1;

	startServer();
}

void startServer()
{
	struct sockaddr_in a;
	int opt;

	a.sin_family = AF_INET;
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_port = htons(cfg.port);

	if ((srv.ld = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		merror("socket");
		endWork(EXIT_FAILURE);
	}
	else
		debug("Successfuly openned socket\n");

	opt = 1;
	if (setsockopt(srv.ld, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
	{
		merror("sersockopt");
		endWork(EXIT_FAILURE);
	}
	else
		debug("Successfuly set sock option to reuse address\n");

	if (bind(srv.ld, (struct sockaddr *)&a, sizeof(a)) == -1)
	{
		merror("bind");
		endWork(EXIT_FAILURE);
	}
	else
		debug("Successfuly binded on *:%d\n", cfg.port);

	if (listen(srv.ld, 5) == -1)
	{
		merror("listen");
		endWork(EXIT_FAILURE);
	}
	else
		debug("Starting listening...\n");

	srv.started = 1;
}

void shutdownServer()
{
	if (srv.started == 0)
		return;

	shutdown(srv.ld, SHUT_RDWR);
	close(srv.ld);
	srv.ld = -1;
	srv.started = 0;
}

mGame * addGame(Game * game)
{
	mGame * mg;

	if (game == NULL)
		return NULL;

	if ((mg = (mGame *)malloc(sizeof(mGame))) == NULL)
		return NULL;

	mg->game = game;
	mg->next = srv.games;
	srv.games = mg;
	++srv.nGames;

	return mg;
}

void delGame(Game * game)
{
	mGame * mg, * mgp;

	if (game == NULL)
		return;

	mgp = NULL;
	mg = srv.games;

	while (mg != NULL)
	{
		if (mg->game == game)
			break;
		else
		{
			mgp = mg;
			mg = mg->next;
		}
	}
	
	if (mg != NULL)
	{
		if (mgp != NULL)
			mgp->next = mg->next;
		else
			srv.games = mg->next;

		--srv.nGames;
		free(mg);
	}
}

void freeGame(Game * g)
{
	delGame(g);
	remGame(g);
}

mPlayer * addNilPlayer(Player * p)
{
	mPlayer * mp;

	if (p == NULL)
		return NULL;

	if ((mp = (mPlayer *)malloc(sizeof(mPlayer))) == NULL)
		return NULL;

	p->game = NULL;
	mp->player = p;
	mp->next = srv.players;
	srv.players = mp;
	++srv.nPlayers;
	++srv.hallPlayers;

	return mp;
}

mPlayer * addPlayer(Game * g, Player * p)
{
	mPlayer * mp;

	if (p == NULL || g == NULL)
		return NULL;

	if ((mp = (mPlayer *)malloc(sizeof(mPlayer))) == NULL)
		return NULL;

	p->game = g;
	mp->player = p;
	mp->next = g->players;
	g->players = mp;
	++srv.nPlayers;

	return mp;
}

void delNilPlayer(Player * p)
{
	mPlayer * mp, * mpp;

	if (p == NULL)
		return;

	if (srv.players == NULL)
		return;

	mpp = NULL;
	mp = srv.players;

	while (mp != NULL)
	{
		if (mp->player == p)
			break;
		else
		{
			mpp = mp;
			mp = mp->next;
		}
	}
	
	if (mp != NULL)
	{
		if (mpp != NULL)
			mpp->next = mp->next;
		else
			srv.players = mp->next;

		--srv.nPlayers;
		--srv.hallPlayers;

		free(mp);
	}
}

void delPlayer(Player * p)
{
	mPlayer * mp, * mpp;

	if (p == NULL)
		return;

	if (p->game == NULL)
		return;

	mpp = NULL;
	mp = p->game->players;

	while (mp != NULL)
	{
		if (mp->player == p)
			break;
		else
		{
			mpp = mp;
			mp = mp->next;
		}
	}

	if (mp != NULL)
	{
		if (mpp != NULL)
			mpp->next = mp->next;
		else
			p->game->players = mp->next;
		--srv.nPlayers;

		free(mp);
	}
}

void freePlayer(Player * p)
{
	if (p == NULL)
		return;

	if (p->game == NULL)
		delNilPlayer(p);
	else
		delPlayer(p);
	remPlayer(p);
}

void IntoGame(Game * g, Player * p)
{
	if (p == NULL || g == NULL)
		return;

	if (p->game != NULL && p->game == g)
		return;

	if (p->game == NULL)
		delNilPlayer(p);
	else
		delPlayer(p);

	addPlayer(g, p);
}

void IntoNil(Player * p)
{
	if (p == NULL || p->game == NULL)
		return;

	delPlayer(p);

	addNilPlayer(p);
}

void acceptPlayer()
{
	int fd;
	Player * p;
	Message mes;

	if ((fd = accept(srv.ld, NULL, NULL)) == -1)
	{
		merror("accept");
		endWork(EXIT_FAILURE);
	}

	p = newPlayer(fd);
	p->pid = srv.nextPid++;
	addNilPlayer(p);

	mes.sndr_t = O_PLAYER;
	mes.sndr.player = p;
	mes.rcvr_t = O_SERVER;
	mes.type = MEST_PLAYER_JOIN_SERVER;
	mes.len = 0;
	mes.data = NULL;
	sendMessage(&mes);
}

void pollServer()
{
	fd_set rfs;
	int maxd = srv.ld;
	mGame * gm;
	mPlayer * pm;
	int sel;

	FD_ZERO(&rfs);
	FD_SET(srv.ld, &rfs);

	for (pm = srv.players; pm; pm = pm->next)
	{
		FD_SET(pm->player->fd, &rfs);
		maxd = max(maxd, pm->player->fd);
	}

	for (gm = srv.games; gm; gm = gm->next)
		for (pm = gm->game->players; pm; pm = pm->next)
		{
			FD_SET(pm->player->fd, &rfs);
			maxd = max(maxd, pm->player->fd);
		}

	sel = select(maxd+1, &rfs, NULL, NULL, NULL);
	if (sel < 1)
		merror("select");

	if (FD_ISSET(srv.ld, &rfs))
		acceptPlayer();

	for (pm = srv.players; pm; pm = pm->next)
		if (FD_ISSET(pm->player->fd, &rfs))
			fetchData(pm->player);

	for (gm = srv.games; gm; gm = gm->next)
		for (pm = gm->game->players; pm; pm = pm->next)
			if (FD_ISSET(pm->player->fd, &rfs))
				fetchData(pm->player);
}

void checkCommands()
{
	mGame * gm;
	mPlayer * pm;

	for (pm = srv.players; pm; pm = pm->next)
		checkPlayerOnCommand(pm->player);

	for (gm = srv.games; gm; gm = gm->next)
		for (pm = gm->game->players; pm; pm = pm->next)
			checkPlayerOnCommand(pm->player);
}

void processEvents()
{
	Message mes;

	while (queue.count > 0)
	{
		getMessage(&mes);

		if (mes.rcvr_t == O_SERVER)
			handleServerEvent(&mes);
		else if (mes.rcvr_t == O_GAME)
			handleGameEvent(mes.rcvr.game, &mes);
		else if (mes.rcvr_t == O_PLAYER)
			handlePlayerEvent(mes.rcvr.player, &mes);
	}
}

void handleServerEvent(Message * mes)
{
	Player * p = mes->sndr.player;
	char * nick = getNickname(p);

	switch(mes->type)
	{
		case MEST_PLAYER_JOIN_SERVER:
			info("%s join to server\n", nick);
			break;
		case MEST_PLAYER_LEAVE_SERVER:
			info("%s leave server\n", nick);
			freePlayer(p);
			break;
		case MEST_COMMAND_UNKNOWN:
			info("%s send unknown command and will disconnected\n", nick);
			freePlayer(p);
			break;
		case MEST_COMMAND_NICK:
			info("%s sets nick into %s\n", nick, mes->data);
			p->nick = mes->data;
			break;
		case MEST_COMMAND_JOIN:
			info("%s joins %d game\n", nick, *(int *)(mes->data));
			free(mes->data);
			break;
	}

	info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
}

void handleGameEvent(Game * g, Message * mes)
{
	Player * p = mes->sndr.player;
	char * nick = getNickname(p);

	switch(mes->type)
	{
		case MEST_PLAYER_JOIN_GAME:
			info("%s join to game\n", nick);
			break;
		case MEST_PLAYER_LEAVE_GAME:
			info("%s leave game\n", nick);
			break;
		case MEST_COMMAND_GET:
			info("%s sent GET command\n", nick);
			break;
		case MEST_COMMAND_SET:
			info("%s sent SET %d command\n", nick, *(int *)(mes->data));
			break;
		case MEST_COMMAND_LEAVE:
			info("%s sent LEAVE command\n", nick);
			break;
	}
}

void handlePlayerEvent(Player * p, Message * mes)
{
}
