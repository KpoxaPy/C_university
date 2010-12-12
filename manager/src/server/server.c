#include <sys/socket.h>
#include <netinet/ip.h>
#include "server.h"
#include "mes.h"

#define ANSWR_OK 0
#define ANSWR_ERROR 1
#define ANSWR_STAT 2
#define ANSWR_GAMES 3
#define ANSWR_PLAYERS 4

struct server {
	int ld;

	mGame * games;
	mPlayer * players; /* non in game players */

	int nGames;
	int nPlayers;
	int hallPlayers;
	int nextPid;
	int nextGid;
	int started;
} srv;

char * answrs[] = {
	"\01",
	"\02",
	"\03",
	"\04",
	"\05"
};

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
	srv.nextGid = 1;

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
	++g->nPlayers;
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
		--p->game->nPlayers;
		p->game = NULL;

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

Game * findGameByGid(int gid)
{
	mGame * t = srv.games;

	while (t != NULL)
	{
		if (t->game->gid == gid)
			break;
		t = t->next;
	}

	if (t != NULL)
		return t->game;
	else
		return NULL;
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
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			break;

		case MEST_PLAYER_LEAVE_SERVER:
			info("%s leave server\n", nick);
			freePlayer(p);
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			break;

		case MEST_COMMAND_UNKNOWN:
			info("%s send unknown command and will be disconnected\n", nick);
			{
				char err = 255;
				write(p->fd, answrs[ANSWR_ERROR], 1);
				write(p->fd, &err, 1);
			}
			freePlayer(p);
			break;

		case MEST_COMMAND_NICK:
			info("%s sets nick into %s\n", nick, mes->data);
			if (p->nick != NULL)
				free(p->nick);
			p->nick = mes->data;
			write(p->fd, answrs[ANSWR_OK], 1);
			break;

		case MEST_COMMAND_ADM:
			info("%s trying to get Adm privilegies\n", nick);
			if (strcmp(cfg.passkey, mes->data) == 0)
				p->adm = 1;

			if (p->adm == 1)
			{
				info("%s grants its Adm privilegies\n", nick);
				write(p->fd, answrs[ANSWR_OK], 1);
			}
			else
			{
				char err = 0;
				info("Attempt was failed\n");
				write(p->fd, answrs[ANSWR_ERROR], 1);
				write(p->fd, &err, 1);
			}
			free(mes->data);
			break;

		case MEST_COMMAND_JOIN:
			info("%s trying to join into game #%d\n", nick, *(uint32_t *)(mes->data));
			if (p->game == NULL)
			{
				Game * g = findGameByGid(*(uint32_t *)(mes->data));
				if (g == NULL)
				{
					info("Not found game #%d\n", *(uint32_t *)(mes->data));
					char err = 1;
					write(p->fd, answrs[ANSWR_ERROR], 1);
					write(p->fd, &err, 1);
				}
				else
				{
					Message newmes;
					info("%s joins into game #%d\n", nick, *(uint32_t *)(mes->data));
					IntoGame(g, p);
					write(p->fd, answrs[ANSWR_OK], 1);
					newmes.sndr_t = O_PLAYER;
					newmes.sndr.player = p;
					newmes.rcvr_t = O_GAME;
					newmes.rcvr.game = g;
					newmes.type = MEST_PLAYER_JOIN_GAME;
					newmes.len = 0;
					newmes.data = NULL;
					sendMessage(&newmes);
				}
			}
			else
			{
				info("%s already in game #%d\n", p->game->gid);
				char err = 0;
				write(p->fd, answrs[ANSWR_ERROR], 1);
				write(p->fd, &err, 1);
			}
			free(mes->data);
			break;

		case MEST_COMMAND_GAMES:
			info("%s trying to get games list\n", nick);
			{
				uint32_t i;
				mGame * mg = srv.games;
				Buffer * buf = newBuffer();
				char * str;

				i = htonl(srv.nGames);
				addnStr(buf, &i, sizeof(i));
				while (mg != NULL)
				{
					i = htonl(mg->game->gid);
					addnStr(buf, &i, sizeof(i));
					i = htonl(mg->game->nPlayers);
					addnStr(buf, &i, sizeof(i));
					mg = mg->next;
				}

				write(p->fd, answrs[ANSWR_GAMES], 1);
				i = htonl(buf->count);
				write(p->fd, &i, sizeof(i));
				i = buf->count;
				str = flushBuffer(buf);
				write(p->fd, str, i);
				free(str);

				clearBuffer(buf);
				free(buf);
			}
			break;

		case MEST_COMMAND_PLAYERS:
			info("%s trying to get players list in game #%d\n", nick, *(uint32_t *)(mes->data));
			{
				Game * g = findGameByGid(*(uint32_t *)(mes->data));
				if (g == NULL)
				{
					char err = 0;
					info("Not found game #%d\n", *(uint32_t *)(mes->data));
					write(p->fd, answrs[ANSWR_ERROR], 1);
					write(p->fd, &err, 1);
				}
				else
				{
					uint32_t i;
					char * n;
					mPlayer * mp = g->players;
					Buffer * buf = newBuffer();
					char * str;

					i = htonl(g->gid);
					addnStr(buf, &i, sizeof(i));
					i = htonl(g->nPlayers);
					addnStr(buf, &i, sizeof(i));
					while (mp != NULL)
					{
						i = htonl(mp->player->pid);
						addnStr(buf, &i, sizeof(i));
						n = getNickname(mp->player);
						i = htonl(strlen(n));
						addnStr(buf, &i, sizeof(i));
						addnStr(buf, n, strlen(n));
						mp = mp->next;
					}

					write(p->fd, answrs[ANSWR_PLAYERS], 1);
					i = htonl(buf->count);
					write(p->fd, &i, sizeof(i));
					i = buf->count;
					str = flushBuffer(buf);
					write(p->fd, str, i);
					free(str);

					clearBuffer(buf);
					free(buf);
				}
			}
			break;

		case MEST_COMMAND_CREATEGAME:
			info("%s trying to create game\n", nick);
			if (!p->adm)
			{
				char err = 0;
				info("%s not granted to create games\n", nick);
				write(p->fd, answrs[ANSWR_ERROR], 1);
				write(p->fd, &err, 1);
			}
			else
			{
				Game * g = newGame();
				uint32_t i;
				g->gid = srv.nextGid++;
				addGame(g);
				i = htonl(g->gid);
				info("%s created game #%d\n", nick, g->gid);
				write(p->fd, answrs[ANSWR_STAT], 1);
				write(p->fd, &i, sizeof(i));
			}
			break;

		case MEST_COMMAND_DELETEGAME:
			info("%s trying to delete game #%d\n", nick, *(uint32_t *)(mes->data));
			if (!p->adm)
			{
				char err = 0;
				info("%s not granted to delete games\n", nick);
				write(p->fd, answrs[ANSWR_ERROR], 1);
				write(p->fd, &err, 1);
			}
			else
			{
				Game * g = findGameByGid(*(uint32_t *)(mes->data));
				if (g == NULL)
				{
					char err = 1;
					info("Not found game #%d\n", *(uint32_t *)(mes->data));
					write(p->fd, answrs[ANSWR_ERROR], 1);
					write(p->fd, &err, 1);
				}
				else
				{
					info("%s deleted game #%d\n", nick, g->gid);
					write(p->fd, answrs[ANSWR_OK], 1);
					freeGame(g);
				}
			}
			break;
	}
}

void handleGameEvent(Game * g, Message * mes)
{
	Player * p = mes->sndr.player;
	char * nick = getNickname(p);

	switch(mes->type)
	{
		case MEST_PLAYER_JOIN_GAME:
			info("%s join to game\n", nick);
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			break;
		case MEST_PLAYER_LEAVE_GAME:
			info("%s leave game\n", nick);
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			break;
		case MEST_COMMAND_GET:
			info("%s trying to get something\n", nick);
			if (g == NULL)
			{
				info("%s didn't get anything\n", nick);
				char err = 0;
				write(p->fd, answrs[ANSWR_ERROR], 1);
				write(p->fd, &err, 1);
			}
			else
			{
				uint32_t i = g->num;
				i = htonl(i);
				info("%s get %d\n", nick, g->num);
				write(p->fd, answrs[ANSWR_STAT], 1);
				write(p->fd, &i, sizeof(i));
			}
			break;
		case MEST_COMMAND_SET:
			info("%s trying to set something into %d\n", nick, *(uint32_t *)(mes->data));
			if (g == NULL)
			{
				info("%s not in game\n", nick, *(uint32_t *)(mes->data));
				char err = 0;
				write(p->fd, answrs[ANSWR_ERROR], 1);
				write(p->fd, &err, 1);
			}
			else
			{
				info("%s successfully sets something into %d\n", nick, *(uint32_t *)(mes->data));
				g->num = *(uint32_t *)(mes->data);
				write(p->fd, answrs[ANSWR_OK], 1);
			}
			free(mes->data);
			break;
		case MEST_COMMAND_LEAVE:
			info("%s trying to leave us\n", nick);
			if (g == NULL)
			{
				info("%s not in game\n", nick);
				char err = 0;
				write(p->fd, answrs[ANSWR_ERROR], 1);
				write(p->fd, &err, 1);
			}
			else
			{
				info("%s leaves game #%d\n", nick, g->gid);
				Message newmes;
				IntoNil(p);
				write(p->fd, answrs[ANSWR_OK], 1);
				newmes.sndr_t = O_PLAYER;
				newmes.sndr.player = p;
				newmes.rcvr_t = O_GAME;
				newmes.rcvr.game = g;
				newmes.type = MEST_PLAYER_LEAVE_GAME;
				newmes.len = 0;
				newmes.data = NULL;
				sendMessage(&newmes);
			}
			break;
	}
}

void handlePlayerEvent(Player * p, Message * mes)
{
}
