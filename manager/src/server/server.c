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
#define ANSWR_MARKET 7

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
#define INFO_START_GAME 13
#define INFO_NOT_TURNED_PLAYERS_LEFT 14
#define INFO_PLAYER_TURNED 15
#define INFO_TURN_ENDED 16
#define INFO_PLAYER_BANKRUPT 17
#define INFO_PLAYER_WIN 18

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
	"\07",
	"\010"
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
	"\015",
	"\016",
	"\017",
	"\020",
	"\021",
	"\022",
	"\023"
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

void freePlayer(Player * p, Game * g)
{
	if (p == NULL)
		return;

	if (p->game == g)
	{
		if (p->game == NULL)
			delNilPlayer(p);
		else
			delPlayer(p);
	}
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
			freePlayer(sp, sp->game);
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

					/* game characteristics */
					if (np->game->started)
					{
						i = htonl(1);
						addnStr(buf, &i, sizeof(i));
						i = htonl(np->mat);
						addnStr(buf, &i, sizeof(i));
						i = htonl(np->prod);
						addnStr(buf, &i, sizeof(i));
						i = htonl(np->bf);
						addnStr(buf, &i, sizeof(i));
						i = htonl(countFactories(np));
						addnStr(buf, &i, sizeof(i));
						i = htonl(np->money);
						addnStr(buf, &i, sizeof(i));
					}
					else
					{
						i = htonl(0);
						addnStr(buf, &i, sizeof(i));
						addnStr(buf, &i, sizeof(i));
						addnStr(buf, &i, sizeof(i));
						addnStr(buf, &i, sizeof(i));
						addnStr(buf, &i, sizeof(i));
						addnStr(buf, &i, sizeof(i));
					}

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
				for (mp = g->players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}

			if (g->num > 0 && g->num == g->nPlayers)
			{
				nmes.sndr_t = O_GAME;
				nmes.sndr.game = g;
				nmes.rcvr_t = O_GAME;
				nmes.rcvr.game = g;
				nmes.type = MEST_START_GAME;
				nmes.len = 0;
				nmes.data = NULL;
				sendMessage(&nmes);
			}
			break;

		case MEST_PLAYER_LEAVE_GAME:
			info("%s has leaved game\n", nick);
			info("Players on server/in hall: %d/%d\n", srv.nPlayers, srv.hallPlayers);
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
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
				for (mp = g->players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
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
				for (mp = g->players; mp != NULL; mp = mp->next)
					if (mp->player != sp)
						write(mp->player->fd, str, i);
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

				if (g->num > 0 && g->num <= g->nPlayers)
				{
					nmes.sndr_t = O_GAME;
					nmes.sndr.game = g;
					nmes.rcvr_t = O_GAME;
					nmes.rcvr.game = g;
					nmes.type = MEST_START_GAME;
					nmes.len = 0;
					nmes.data = NULL;
					sendMessage(&nmes);
				}
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

		case MEST_COMMAND_START:
			info("%s trying to start game\n", nick);
			if (!sp->adm)
			{
				info("%s not granted to start games\n", nick);
				char err = 0;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else if (g == NULL)
			{
				info("%s not in game\n", nick);
				char err = 1;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else if (g->started)
			{
				info("Game #%d is already started\n", g->gid);
				char err = 3;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else if (g->nPlayers == 0)
			{
				info("No players in game #%d\n", g->gid);
				char err = 2;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else
			{
				write(sp->fd, answrs[ANSWR_OK], 1);
				nmes.sndr_t = O_PLAYER;
				nmes.sndr.player = sp;
				nmes.rcvr_t = O_GAME;
				nmes.rcvr.game = g;
				nmes.type = MEST_START_GAME;
				nmes.len = 0;
				nmes.data = NULL;
				sendMessage(&nmes);
			}
			break;

		case MEST_COMMAND_TURN:
			info("%s trying to turn in game\n", nick);
			if (g == NULL)
			{
				info("%s not in game\n", nick);
				char err = 0;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else if (!g->started)
			{
				info("Game #%d isn't started\n", g->gid);
				char err = 2;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else if (sp->turned)
			{
				info("%s already turned\n", nick);
				char err = 1;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else
			{
				int ntp = 0; /* not turned players */
				mPlayer * mp;
				write(sp->fd, answrs[ANSWR_OK], 1);

				/* Semd info message about turned player */
				{
					Buffer * msg = newBuffer();
					Buffer * info = newBuffer();
					char * str;
					uint32_t i;

					/*  generating MES part  */
					addnStr(msg, infos[INFO_PLAYER_TURNED], 1);
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
					for (mp = g->players; mp != NULL; mp = mp->next)
						if (mp->player != sp)
							write(mp->player->fd, str, i);
					free(str);

					clearBuffer(msg);
					free(msg);
					clearBuffer(info);
					free(info);
				}

				sp->turned = 1;

				for (mp = g->players; mp != NULL; mp = mp->next)
					if (!mp->player->turned)
						++ntp;

				info("Turned players: %d/%d\n", g->nPlayers, ntp);

				if (ntp > 0)  /* Send info message about not turned players */
				{
					Buffer * msg = newBuffer();
					Buffer * info = newBuffer();
					char * str;
					uint32_t i;

					/*  generating MES part  */
					addnStr(msg, infos[INFO_NOT_TURNED_PLAYERS_LEFT], 1);
					i = htonl(g->nPlayers);
					addnStr(msg, &i, sizeof(i));
					i = htonl(ntp);
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
					for (mp = g->players; mp != NULL; mp = mp->next)
						write(mp->player->fd, str, i);
					free(str);

					clearBuffer(msg);
					free(msg);
					clearBuffer(info);
					free(info);
				}
				else
				{
					nmes.sndr_t = O_GAME;
					nmes.sndr.game = g;
					nmes.rcvr_t = O_GAME;
					nmes.rcvr.game = g;
					nmes.type = MEST_TURN_ENDED;
					nmes.len = 0;
					nmes.data = NULL;
					sendMessage(&nmes);
				}
			}
			break;

		case MEST_COMMAND_PROD:
			info("%s trying to product something in game\n", nick);
			{
				uint32_t n = *(uint32_t *)(mes->data);
				if (g == NULL)
				{
					info("%s not in game\n", nick);
					char err = 0;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (sp->turned)
				{
					info("%s already turned\n", nick);
					char err = 3;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (!g->started)
				{
					info("Game #%d isn't started\n", g->gid);
					char err = 4;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n < 1 || n > sp->bf)
				{
					info("Nonavail product value got\n");
					char err = 1;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n*(g->prodPrice) > sp->money || n*(g->prodStuff) > sp->mat)
				{
					info("Not enough resources to product\n");
					char err = 2;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else
				{
					write(sp->fd, answrs[ANSWR_OK], 1);

					sp->prodBid.c = n;
				}
			}
			free(mes->data);
			break;

		case MEST_COMMAND_MARKET:
			info("%s trying to take market information in game\n", nick);
			if (g == NULL)
			{
				info("%s not in game\n", nick);
				char err = 0;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else if (!g->started)
			{
				info("Game #%d isn't started\n", g->gid);
				char err = 1;
				write(sp->fd, answrs[ANSWR_ERROR], 1);
				write(sp->fd, &err, 1);
			}
			else
			{
				uint32_t i;
				char * str;
				Buffer * buf = newBuffer();

				i = htonl(g->nPlayers);
				addnStr(buf, &i, sizeof(i));

				i = htonl(g->turn);
				addnStr(buf, &i, sizeof(i));

				i = htonl(g->state.sellC);
				addnStr(buf, &i, sizeof(i));
				i = htonl(g->state.sellMinP);
				addnStr(buf, &i, sizeof(i));

				i = htonl(g->state.buyC);
				addnStr(buf, &i, sizeof(i));
				i = htonl(g->state.buyMaxP);
				addnStr(buf, &i, sizeof(i));

				write(sp->fd, answrs[ANSWR_MARKET], 1);
				i = buf->count;
				str = flushBuffer(buf);
				write(sp->fd, str, i);
				free(str);

				clearBuffer(buf);
				free(buf);
			}
			break;

		case MEST_COMMAND_BUILD:
			info("%s trying to build something in game\n", nick);
			{
				uint32_t n = *(uint32_t *)(mes->data);
				if (g == NULL)
				{
					info("%s not in game\n", nick);
					char err = 0;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (!g->started)
				{
					info("Game #%d isn't started\n", g->gid);
					char err = 4;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (sp->turned)
				{
					info("%s already turned\n", nick);
					char err = 3;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n < 1)
				{
					info("Nonavail build value got\n");
					char err = 1;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if ((g->bFacPrice/2) > sp->money)
				{
					info("Not enough resources to build\n");
					char err = 2;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else
				{
					write(sp->fd, answrs[ANSWR_OK], 1);

					addFactory(sp);
				}
			}
			free(mes->data);
			break;

		case MEST_COMMAND_SELL:
			info("%s trying to sell something in game\n", nick);
			{
				uint32_t n = *(uint32_t *)(mes->data);
				uint32_t p = *(uint32_t *)(mes->data + 4);
				if (g == NULL)
				{
					info("%s not in game\n", nick);
					char err = 0;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (!g->started)
				{
					info("Game #%d isn't started\n", g->gid);
					char err = 5;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (sp->turned)
				{
					info("%s already turned\n", nick);
					char err = 4;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n != 0 && p > g->state.buyMaxP)
				{
					info("Price that is offered by %s is too high\n", nick);
					char err = 1;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n > sp->prod)
				{
					info("%s don't have enough product\n", nick);
					char err = 2;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n > g->state.buyC)
				{
					info("%s offered too many product to sell\n", nick);
					char err = 3;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else
				{
					write(sp->fd, answrs[ANSWR_OK], 1);
					sp->sellBid.c = n;
					sp->sellBid.p = p;
				}
			}
			free(mes->data);
			break;

		case MEST_COMMAND_BUY:
			info("%s trying to buy something in game\n", nick);
			{
				uint32_t n = *(uint32_t *)(mes->data);
				uint32_t p = *(uint32_t *)(mes->data + 4);
				if (g == NULL)
				{
					info("%s not in game\n", nick);
					char err = 0;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (!g->started)
				{
					info("Game #%d isn't started\n", g->gid);
					char err = 5;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (sp->turned)
				{
					info("%s already turned\n", nick);
					char err = 4;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n != 0 && p < g->state.sellMinP)
				{
					info("Price that is offered by %s is too small\n", nick);
					char err = 1;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n*p > sp->money)
				{
					info("%s don't have enough money\n", nick);
					char err = 2;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else if (n > g->state.sellC)
				{
					info("%s offered too many material to buy\n", nick);
					char err = 3;
					write(sp->fd, answrs[ANSWR_ERROR], 1);
					write(sp->fd, &err, 1);
				}
				else
				{
					write(sp->fd, answrs[ANSWR_OK], 1);
					sp->buyBid.c = n;
					sp->buyBid.p = p;
				}
			}
			free(mes->data);
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

		case MEST_START_GAME:
			info("Game #%d started with %d players\n", g->gid, g->nPlayers);

			/* Send info message about game start */
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_START_GAME], 1);

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
				for (mp = g->players; mp != NULL; mp = mp->next)
					write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}

			startGame(g);
			break;

		case MEST_TURN_ENDED:
			info("New turn in game #%d\n", g->gid);

			{
				Turn * t;
				Buffer * msgshr = newBuffer();
				mPlayer * mp;
				Bid * b;
				char * strshr;
				int lenshr;
				uint32_t i;

				t = turnGame(g);

				/*  generating shared MES part  */
				addnStr(msgshr, infos[INFO_TURN_ENDED], 1);
				i = htonl(t->turn);
				addnStr(msgshr, &i, sizeof(i));
				i = htonl(countBids(t->sell) + countBids(t->buy));
				addnStr(msgshr, &i, sizeof(i));
				for (b = t->buy->f; b != NULL; b = b->next)
				{
					i = htonl(b->type);
					addnStr(msgshr, &i, sizeof(i));
					i = htonl(b->p->pid);
					addnStr(msgshr, &i, sizeof(i));
					i = htonl(b->count);
					addnStr(msgshr, &i, sizeof(i));
					i = htonl(b->price);
					addnStr(msgshr, &i, sizeof(i));
				}
				for (b = t->sell->f; b != NULL; b = b->next)
				{
					i = htonl(b->type);
					addnStr(msgshr, &i, sizeof(i));
					i = htonl(b->p->pid);
					addnStr(msgshr, &i, sizeof(i));
					i = htonl(b->count);
					addnStr(msgshr, &i, sizeof(i));
					i = htonl(b->price);
					addnStr(msgshr, &i, sizeof(i));
				}

				lenshr = msgshr->count;
				strshr = flushBuffer(msgshr);

				for (mp = g->players; mp != NULL; mp = mp->next)
				{
					Buffer * info = newBuffer();
					Buffer * msg = newBuffer();
					char * str;

					/*  generating private MES part  */
					addnStr(msg, strshr, lenshr);

					if (countBids(t->prod) == 0)
					{
						i = htonl(0);
						addnStr(msg, &i, sizeof(i));
					}
					else
					{
						for (b = t->prod->f; b != NULL && b->p != mp->player; b = b->next);
						if (b == NULL)
							i = htonl(0);
						else
							i = htonl(b->count);
						addnStr(msg, &i, sizeof(i));
					}

					if (countBids(t->fac) == 0)
					{
						i = htonl(0);
						addnStr(msg, &i, sizeof(i));
					}
					else
					{
						for (b = t->fac->f; b != NULL && b->p != mp->player; b = b->next);
						if (b == NULL)
							i = htonl(0);
						else
							i = htonl(b->count);
						addnStr(msg, &i, sizeof(i));
					}

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
					write(mp->player->fd, str, i);
					free(str);

					clearBuffer(msg);
					free(msg);
					clearBuffer(info);
					free(info);
				}

				free(strshr);

				clearBuffer(msgshr);
				free(msgshr);

				remTurn(t);
			}
			break;

		case MEST_PLAYER_BANKRUPT:
			sp = (Player *)(mes->data);
			info("%s had bankrupted in game #%d\n", getNickname(sp), g->gid);
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_BANKRUPT], 1);
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
				for (mp = g->players; mp != NULL; mp = mp->next)
					write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			nmes.sndr_t = O_PLAYER;
			nmes.sndr.player = sp;
			nmes.rcvr_t = O_GAME;
			nmes.rcvr.game = NULL;
			nmes.type = MEST_PLAYER_JOINS_HALL;
			nmes.len = 0;
			nmes.data = NULL;
			sendMessage(&nmes);
			IntoNil(sp);
			debug("%d in game#%d", getNickname(sp), (sp->game == NULL ? 0 : sp->game->gid));
			free(mes->data);
			break;

		case MEST_PLAYER_WIN:
			sp = (Player *)(mes->data);
			info("%s had win in game #%d\n", getNickname(sp), g->gid);
			{
				Buffer * msg = newBuffer();
				Buffer * info = newBuffer();
				mPlayer * mp;
				char * str;
				uint32_t i;

				/*  generating MES part  */
				addnStr(msg, infos[INFO_PLAYER_WIN], 1);
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
				for (mp = g->players; mp != NULL; mp = mp->next)
					write(mp->player->fd, str, i);
				free(str);

				clearBuffer(msg);
				free(msg);
				clearBuffer(info);
				free(info);
			}
			nmes.sndr_t = O_PLAYER;
			nmes.sndr.player = sp;
			nmes.rcvr_t = O_GAME;
			nmes.rcvr.game = NULL;
			nmes.type = MEST_PLAYER_JOINS_HALL;
			nmes.len = 0;
			nmes.data = NULL;
			sendMessage(&nmes);
			IntoNil(sp);
			debug("%d in game#%d", getNickname(sp), (sp->game == NULL ? 0 : sp->game->gid));
			free(mes->data);
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
			free(mes->data);
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
			freePlayer(p, p->game);
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
