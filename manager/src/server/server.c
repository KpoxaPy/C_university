#include <sys/socket.h>
#include <netinet/ip.h>
#include "server.h"
#include "mes.h"

#define ANSWR_OK 0
#define ANSWR_ERROR 1
#define ANSWR_STAT 2
#define ANSWR_GAMES 3
#define ANSWR_PLAYERS 4
#define ANSWR_PLAYER 5
#define ANSWR_INFO 6

#define INFO_PLAYER_JOINS_SERVER 0
#define INFO_PLAYER_LEAVES_SERVER 1
#define INFO_PLAYER_CHANGES_NICK 2

#define INFO_NEW_GAME 3
#define INFO_PLAYER_LEAVES_HALL 4
#define INFO_PLAYER_JOINS_HALL 5
/*#define INFO_GAME_OVER 9*/

#define INFO_PLAYER_JOINS_GAME 6
#define INFO_PLAYERS_STAT_GAME 7
#define INFO_PLAYER_LEAVES_GAME 8
#define INFO_GAME_OVER 9

#define INFO_PLAYER_KICKED_FROM_GAME 10
#define INFO_PLAYER_KICKED_FROM_SERVER 11
#define INFO_PLAYER_PID 12

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
	"\05",
	"\06",
	"\07"
};

char * infos[] = {
	"\01",
	"\02",
	"\03",
	"\04",
	"\05",
	"\06",
	"\07",
	"\010",
	"\011",
	"\012",
	"\013",
	"\014",
	"\015"
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

Player * findPlayerByPid(int pid)
{
	mPlayer * mp;
	mGame * mg;

	for (mp = srv.players; mp != NULL; mp = mp->next)
		if (mp->player->pid == pid)
			return mp->player;

	for (mg = srv.games; mg != NULL; mg = mg->next)
		for (mp = mg->game->players; mp != NULL; mp = mp->next)
			if (mp->player->pid == pid)
				return mp->player;

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
	mes.sndr_t = O_SERVER;
	mes.rcvr_t = O_PLAYER;
	mes.rcvr.player = p;
	mes.type = MEST_PLAYER_PID;
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
	Player * sp = NULL;
	Game * sg = NULL;
	char * nick = NULL;
	Message nmes;

	if (mes->sndr_t == O_PLAYER)
	{
		sp = mes->sndr.player;
		nick = getNickname(sp);
	}
	if (mes->sndr_t == O_GAME)
		sg = mes->sndr.game;

	switch(mes->type)
	{
		case MEST_PLAYER_JOIN_SERVER:
			info("%s has joined on the server\n", nick);
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				mGame * mg;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_JOINS_SERVER], 1);
				i = htonl(sp->pid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in hall  */
				for (mp = srv.players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
				/*  players in games  */
				for (mg = srv.games; mg != NULL; mg = mg->next)
					for (mp = mg->game->players; mp != NULL; mp = mp->next)
						if (mp->player != sp)
							write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;

		case MEST_PLAYER_LEAVE_SERVER:
			info("%s has leaved the server\n", nick);
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				mGame * mg;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_LEAVES_SERVER], 1);
				i = htonl(sp->pid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in hall  */
				for (mp = srv.players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
				/*  players in games  */
				for (mg = srv.games; mg != NULL; mg = mg->next)
					for (mp = mg->game->players; mp != NULL; mp = mp->next)
						if (mp->player != sp)
							write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			freePlayer(sp);
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			break;

		case MEST_PLAYER_CHANGES_NICK:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				mGame * mg;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_CHANGES_NICK], 1);
				i = htonl(sp->pid);
				addnStr(msg, &i, sizeof(i));
				i = htonl(strlen(nick));
				addnStr(msg, &i, sizeof(i));
				i = strlen(nick);
				addnStr(msg, nick, i);

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in hall  */
				for (mp = srv.players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
				/*  players in games  */
				for (mg = srv.games; mg != NULL; mg = mg->next)
					for (mp = mg->game->players; mp != NULL; mp = mp->next)
						if (mp->player != sp)
							write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;

		case MEST_COMMAND_UNKNOWN:
			info("%s send unknown command and will be disconnected\n", nick);
			{
				char err = 255;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			nmes.sndr_t = O_SERVER;
			nmes.rcvr_t = O_GAME;
			nmes.rcvr.game = sp->game;
			if (sp->game != NULL)
				nmes.type = MEST_PLAYER_LEAVE_GAME;
			else
				nmes.type = MEST_PLAYER_LEAVES_HALL;
			nmes.len = 0;
			nmes.data = NULL;
			sendMessage(&nmes);
			nmes.sndr_t = O_SERVER;
			nmes.rcvr_t = O_PLAYER;
			nmes.rcvr.player = sp;
			nmes.type = MEST_PLAYER_KICKED_FROM_SERVER;
			nmes.len = 0;
			nmes.data = NULL;
			sendMessage(&nmes);
			break;

		case MEST_COMMAND_NICK:
			info("%s sets nick into %s\n", nick, mes->data);
			if (sp->nick != NULL)
				free(sp->nick);
			sp->nick = mes->data;
			write(sp->fd, answrs[ANSWR_OK], 1);
			nmes.sndr_t = O_PLAYER;
			nmes.sndr.player = sp;
			nmes.rcvr_t = O_SERVER;
			nmes.type = MEST_PLAYER_CHANGES_NICK;
			nmes.len = 0;
			nmes.data = NULL;
			sendMessage(&nmes);
			break;

		case MEST_COMMAND_ADM:
			info("%s trying to get Adm privilegies\n", nick);
			if (strcmp(cfg.passkey, mes->data) == 0)
				sp->adm = 1;

			if (sp->adm == 1)
			{
				info("%s grants its Adm privilegies\n", nick);
				write(sp->fd, answrs[ANSWR_OK], 1);
			}
			else
			{
				char err = 0;
				info("Attempt was failed\n");
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			free(mes->data);
			break;

		case MEST_COMMAND_JOIN:
			info("%s trying to join into game #%d\n", nick, *(uint32_t *)(mes->data));
			if (sp->game == NULL)
			{
				Game * g = findGameByGid(*(uint32_t *)(mes->data));
				if (g == NULL)
				{
					info("Not found game #%d\n", *(uint32_t *)(mes->data));
					char err = 1;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else
				{
					info("%s joins into game #%d\n", nick, *(uint32_t *)(mes->data));
					IntoGame(g, sp);
					write(sp->fd, answrs[ANSWR_OK], 1);
					nmes.sndr_t = O_PLAYER;
					nmes.sndr.player = sp;
					nmes.rcvr_t = O_GAME;
					nmes.rcvr.game = g;
					nmes.type = MEST_PLAYER_JOIN_GAME;
					nmes.len = 0;
					nmes.data = NULL;
					sendMessage(&nmes);
					nmes.sndr_t = O_PLAYER;
					nmes.sndr.player = sp;
					nmes.rcvr_t = O_GAME;
					nmes.rcvr.game = NULL;
					nmes.type = MEST_PLAYER_LEAVES_HALL;
					nmes.len = sizeof(g);
					nmes.data = g;
					sendMessage(&nmes);
					nmes.sndr_t = O_PLAYER;
					nmes.sndr.player = sp;
					nmes.rcvr_t = O_GAME;
					nmes.rcvr.game = g;
					nmes.type = MEST_PLAYERS_STAT_GAME;
					nmes.len = 0;
					nmes.data = NULL;
					sendMessage(&nmes);
				}
			}
			else
			{
				info("%s already in game #%d\n", sp->game->gid);
				char err = 0;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
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

				write(sp->fd, answrs[ANSWR_GAMES], 1);
				i = htonl(buf->count);
				write(sp->fd, &i, sizeof(i));
				i = buf->count;
				str = flushBuffer(buf);
				write(sp->fd, str, i);
				free(str);

				clearBuffer(buf);
				free(buf);
			}
			break;

		case MEST_COMMAND_PLAYERS:
			info("%s trying to get players list in game #%d\n", nick, *(uint32_t *)(mes->data));
			{
				int gid = *(uint32_t *)(mes->data);
				Game * g = NULL;

				if (gid != 0)
				{
					g = findGameByGid(gid);
					if (g == NULL)
					{
						char err = 0;
						info("Not found game #%d\n", *(uint32_t *)(mes->data));
						write(sp->fd, answrs[ANSWR_ERROR], 1);
						write(sp->fd, &err, 1);
					}
				}

				if (gid == 0 || g != NULL)
				{
					uint32_t i;
					char * n;
					mPlayer * mp;
					Buffer * buf = newBuffer();
					char * str;

					if (gid == 0)
						mp = srv.players;
					else
						mp = g->players;

					i = htonl(gid);
					addnStr(buf, &i, sizeof(i));
					if (gid == 0)
						i = htonl(srv.hallPlayers);
					else
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

					write(sp->fd, answrs[ANSWR_PLAYERS], 1);
					i = htonl(buf->count);
					write(sp->fd, &i, sizeof(i));
					i = buf->count;
					str = flushBuffer(buf);
					write(sp->fd, str, i);
					free(str);

					clearBuffer(buf);
					free(buf);
				}
			}
			break;

		case MEST_COMMAND_CREATEGAME:
			info("%s trying to create game\n", nick);
			if (!sp->adm)
			{
				char err = 0;
				info("%s not granted to create games\n", nick);
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else
			{
				Game * g = newGame();
				uint32_t i;
				g->gid = srv.nextGid++;
				addGame(g);
				i = htonl(g->gid);
				info("%s created game #%d\n", nick, g->gid);
				write(sp->fd, answrs[ANSWR_STAT], 1);
				write(sp->fd, &i, sizeof(i));
				nmes.sndr_t = O_PLAYER;
				nmes.sndr.player = sp;
				nmes.rcvr_t = O_GAME;
				nmes.rcvr.game = NULL;
				nmes.type = MEST_NEW_GAME;
				nmes.len = sizeof(g);
				nmes.data = g;
				sendMessage(&nmes);
			}
			break;

		case MEST_COMMAND_DELETEGAME:
			info("%s trying to delete game #%d\n", nick, *(uint32_t *)(mes->data));
			if (!sp->adm)
			{
				char err = 0;
				info("%s not granted to delete games\n", nick);
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else
			{
				Game * g = findGameByGid(*(uint32_t *)(mes->data));
				if (g == NULL)
				{
					char err = 1;
					info("Not found game #%d\n", *(uint32_t *)(mes->data));
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else
				{
					mPlayer * mp;
					info("%s deleted game #%d\n", nick, g->gid);
					write(sp->fd, answrs[ANSWR_OK], 1);

					for (mp = g->players; mp != NULL; mp = mp->next)
					{
						nmes.sndr_t = O_PLAYER;
						nmes.sndr.player = sp;
						nmes.rcvr_t = O_PLAYER;
						nmes.rcvr.player = mp->player;
						nmes.type = MEST_PLAYER_KICKED_FROM_GAME;
						nmes.len = sizeof(g);
						nmes.data = g;
						sendMessage(&nmes);
					}
					nmes.sndr_t = O_PLAYER;
					nmes.sndr.player = sp;
					nmes.rcvr_t = O_GAME;
					nmes.rcvr.game = g;
					nmes.type = MEST_GAME_OVER;
					nmes.len = 0;
					nmes.data = NULL;
					sendMessage(&nmes);
				}
			}
			break;

		case MEST_COMMAND_PLAYER:
			info("%s trying to get info about player #%d\n", nick, *(uint32_t *)(mes->data));
			{
				Player * np = findPlayerByPid(*(uint32_t *)(mes->data));
				if (np == NULL)
				{
					char err = 0;
					info("Not found player #%d\n", *(uint32_t *)(mes->data));
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else
				{
					uint32_t i;
					Buffer * buf = newBuffer();
					char * str;

					/*  #pid  */
					i = htonl(np->pid);
					addnStr(buf, &i, sizeof(i));

					/*  #game  */
					if (np->game == NULL)
						i = 0;
					else
						i = np->game->gid;
					i = htonl(i);
					addnStr(buf, &i, sizeof(i));

					/*  len NICK  */
					str = getNickname(np);
					i = htonl(strlen(str));
					addnStr(buf, &i, sizeof(i));
					addnStr(buf, str, strlen(str));

					/*  player len MES */
					write(sp->fd, answrs[ANSWR_PLAYER], 1);
					i = htonl(buf->count);
					write(sp->fd, &i, sizeof(i));
					i = buf->count;
					str = flushBuffer(buf);
					write(sp->fd, str, i);
					free(str);

					clearBuffer(buf);
					free(buf);
				}
			}
			break;
	}
}

void handleGameEvent(Game * g, Message * mes)
{
	Player * sp = NULL;
	Game * sg = NULL;
	char * nick = NULL;
	Message nmes;

	if (mes->sndr_t == O_PLAYER)
	{
		sp = mes->sndr.player;
		nick = getNickname(sp);
	}
	if (mes->sndr_t == O_GAME)
		sg = mes->sndr.game;

	switch(mes->type)
	{
		case MEST_PLAYER_JOIN_GAME:
			info("%s has joined into game\n", nick);
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				mGame * mg;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_JOINS_GAME], 1);
				i = htonl(sp->pid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in our game  */
				for (mg = srv.games; mg != NULL; mg = mg->next)
					if (mg->game == g)
					{
						for (mp = mg->game->players; mp != NULL; mp = mp->next)
							if (mp->player != sp)
								write(mp->player->fd, str, i);
						break;
					}
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;

		case MEST_PLAYER_LEAVE_GAME:
			info("%s has leaved game\n", nick);
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				mGame * mg;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_LEAVES_GAME], 1);
				i = htonl(sp->pid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in our game  */
				for (mg = srv.games; mg != NULL; mg = mg->next)
					if (mg->game == g)
					{
						for (mp = mg->game->players; mp != NULL; mp = mp->next)
							if (mp->player != sp)
								write(mp->player->fd, str, i);
						break;
					}
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;

		case MEST_PLAYER_JOINS_HALL:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_JOINS_HALL], 1);
				i = htonl(sp->pid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in hall  */
				for (mp = srv.players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;

		case MEST_PLAYER_LEAVES_HALL:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_LEAVES_HALL], 1);
				i = htonl(sp->pid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in hall  */
				for (mp = srv.players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;

		case MEST_NEW_GAME:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_NEW_GAME], 1);
				i = htonl(((Game *)(mes->data))->gid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in hall  */
				for (mp = srv.players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;

		case MEST_PLAYERS_STAT_GAME:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				mGame * mg;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYERS_STAT_GAME], 1);
				i = htonl(g->nPlayers);
				addnStr(msg, &i, sizeof(i));
				i = htonl(g->num);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in our game  */
				for (mg = srv.games; mg != NULL; mg = mg->next)
					if (mg->game == g)
					{
						for (mp = mg->game->players; mp != NULL; mp = mp->next)
							if (mp->player != sp)
								write(mp->player->fd, str, i);
						break;
					}
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;

		case MEST_COMMAND_GET:
			info("%s trying to get something\n", nick);
			if (g == NULL)
			{
				info("%s didn't get anything\n", nick);
				char err = 0;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else
			{
				uint32_t i = g->num;
				i = htonl(i);
				info("%s get %d\n", nick, g->num);
				write(sp->fd, answrs[ANSWR_STAT], 1);
				write(sp->fd, &i, sizeof(i));
			}
			break;

		case MEST_COMMAND_SET:
			info("%s trying to set something into %d\n", nick, *(uint32_t *)(mes->data));
			if (g == NULL)
			{
				info("%s not in game\n", nick, *(uint32_t *)(mes->data));
				char err = 0;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else
			{
				info("%s successfully sets something into %d\n", nick, *(uint32_t *)(mes->data));
				g->num = *(uint32_t *)(mes->data);
				write(sp->fd, answrs[ANSWR_OK], 1);
			}
			free(mes->data);
			break;

		case MEST_COMMAND_LEAVE:
			info("%s trying to leave us\n", nick);
			if (g == NULL)
			{
				info("%s not in game\n", nick);
				char err = 0;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else
			{
				info("%s leaves game #%d\n", nick, g->gid);
				IntoNil(sp);
				write(sp->fd, answrs[ANSWR_OK], 1);
				nmes.sndr_t = O_PLAYER;
				nmes.sndr.player = sp;
				nmes.rcvr_t = O_GAME;
				nmes.rcvr.game = g;
				nmes.type = MEST_PLAYER_LEAVE_GAME;
				nmes.len = 0;
				nmes.data = NULL;
				sendMessage(&nmes);
				nmes.sndr_t = O_PLAYER;
				nmes.sndr.player = sp;
				nmes.rcvr_t = O_GAME;
				nmes.rcvr.game = NULL;
				nmes.type = MEST_PLAYER_JOINS_HALL;
				nmes.len = sizeof(g);
				nmes.data = g;
				sendMessage(&nmes);
				nmes.sndr_t = O_PLAYER;
				nmes.sndr.player = sp;
				nmes.rcvr_t = O_GAME;
				nmes.rcvr.game = g;
				nmes.type = MEST_PLAYERS_STAT_GAME;
				nmes.len = 0;
				nmes.data = NULL;
				sendMessage(&nmes);
			}
			break;

		case MEST_GAME_OVER:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYERS_STAT_GAME], 1);
				i = htonl(g->gid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				/*  players in hall  */
				for (mp = srv.players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			freeGame(g);
			break;
	}
}

void handlePlayerEvent(Player * p, Message * mes)
{
	Player * sp = NULL;
	Game * sg = NULL;
	char * nick = NULL;
	/*Message nmes;*/

	if (mes->sndr_t == O_PLAYER)
	{
		sp = mes->sndr.player;
		nick = getNickname(sp);
	}
	if (mes->sndr_t == O_GAME)
		sg = mes->sndr.game;

	switch(mes->type)
	{
		case MEST_PLAYER_KICKED_FROM_GAME:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_KICKED_FROM_GAME], 1);

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				write(p->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			IntoNil(p);
			break;

		case MEST_PLAYER_KICKED_FROM_SERVER:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_KICKED_FROM_SERVER], 1);

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				write(p->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			freePlayer(p);
			break;

		case MEST_PLAYER_PID:
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_PID], 1);
				i = htonl(p->pid);
				addnStr(msg, &i, sizeof(i));

				/*  generating INFO part  */
				addnStr(info, answrs[ANSWR_INFO], 1);
				i = htonl(msg->count);
				addnStr(info, &i, sizeof(i));
				i = msg->count;
				str = flushBuffer(msg);
				addnStr(info, str, i);
				free(str);

				/*  sending info message  */
				i = info->count;
				str = flushBuffer(info);
				write(p->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			break;
	}
}
