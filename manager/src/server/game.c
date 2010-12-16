#include <sys/socket.h>
#include "game.h"
#include "mes.h"
#include "server.h"

int checkBufferOnCommand(Player *);

void buyAuction(Game *, BidList *);
void sellAuction(Game *, BidList *);

int isPlayerBankrupt(Player *);

Game * newGame()
{
	Game * g;

	if ((g = (Game *)malloc(sizeof(Game))) == NULL)
		return NULL;

	g->num = 0;
	g->gid = 0;
	g->nPlayers = 0;
	g->players = NULL;

	g->started = 0;
	g->turn = 1;

	setState(g, GAME_STATE_DFL);

	g->rMatPrice = GAME_DFL_MATERIAL_RENT_PRICE;
	g->rProdPrice = GAME_DFL_PRODUCT_RENT_PRICE;
	g->rFacPrice = GAME_DFL_FACTORY_RENT_PRICE;
	g->bFacPrice = GAME_DFL_FACTORY_BUILD_PRICE;
	g->bFacTime = GAME_DFL_FACTORY_BUILD_TIME;
	g->prodPrice = GAME_DFL_PRODUCT_PRICE;
	g->prodStuff = GAME_DFL_PRODUCT_STUFF;

	return g;
}

void remGame(Game * g)
{
	if (g == NULL)
		return;

	while (g->players != NULL)
		freePlayer(g->players->player, g);

	free(g);
}

void setState(Game * g, int s)
{
	static struct states {
		double matm;
		int matp;
		double prodm;
		int prodp;
	} states[] = {
		{1.0, 800, 3.0, 6500},
		{1.5, 650, 2.5, 6000},
		{2.0, 500, 2.0, 5500},
		{2.5, 400, 1.5, 5000},
		{3.0, 300, 1.0, 4500}
	};

	if (g == NULL)
		return;

	g->state.s = s;
	g->state.sellC = (int)(states[s].matm * g->nPlayers);
	g->state.sellMinP = states[s].matp;
	g->state.buyC = (int)(states[s].prodm * g->nPlayers);
	g->state.buyMaxP = states[s].prodp;
}

void changeState(Game * g)
{
	static int lvl[5][4] = {
		{4, 8, 10, 11},
		{3, 7, 10, 11},
		{1, 4, 8, 11},
		{1, 2, 5, 9},
		{1, 2, 4, 8}
	};
	int r;
	int s;

	if (g == NULL)
		return;

	r = 1 + (int)(12.0*rand()/(RAND_MAX+1.0));

	for (s = 0; s < 4 && lvl[g->state.s][s] < r; s++)
		;

	setState(g, s);
}

Factory * newFactory()
{
	Factory * f = (Factory *)malloc(sizeof(Factory));

	if (f == NULL)
		return NULL;

	f->age = 0;
	f->next = NULL;

	return f;
}

void remFactory(Factory * f)
{
	if (f == NULL)
		return;

	free(f);
}

void addFactory(Player * p)
{
	Factory * f;

	if (p == NULL || p->game == NULL)
		return;

	if ((f = newFactory()) == NULL)
		return;

	f->next = p->cf;
	p->cf = f;
	p->money -= p->game->bFacPrice/2;
}

void clearFactories(Player * p)
{
	Factory * f, *t;

	if (p == NULL)
		return;

	f = p->cf;
	while (f != NULL)
	{
		t = f;
		f = f->next;
		remFactory(t);
	}

	p->cf = NULL;
}

/* Increase ages of factories, clean up builded and return count of builded */
int checkFactories(Player * p)
{
	int n = 0;
	Factory * f, * t;

	if (p == NULL || p->game == NULL)
		return 0;

	f = p->cf;
	t = NULL;

	while (f != NULL)
	{
		if (f->age >= p->game->bFacTime)
		{
			n++;
			if (t == NULL)
				p->cf = f->next;
			else
				t->next = f->next;
			free(f);
		}
		else
		{
			f->age++;

			t = f;
		}
		f = t->next;
	}
	
	return n;
}

int countFactories(Player * p)
{
	int n = 0;
	Factory * f;

	if (p == NULL || p->game == NULL)
		return 0;

	f = p->cf;

	while (f != NULL)
	{
		++n;
		f = f->next;
	}
	
	return n;
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
	p->nick = NULL;
	p->dummynick = NULL;
	p->adm = 0;
	p->turned = 0;

	p->mat = 0;
	p->prod = 0;
	p->bf = 0;
	p->cf = NULL;
	p->money = 0;
	p->sellBid.c = 0;
	p->sellBid.p = 0;
	p->buyBid.c = 0;
	p->buyBid.p = 0;
	p->prodBid.c = 0;

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

	clearFactories(p);

	if (p->nick != NULL)
		free(p->nick);

	if (p->dummynick != NULL)
		free(p->dummynick);

	free(p);
}

char * getNickname(Player * p)
{
	if (p == NULL)
		return NULL;

	if (p->nick != NULL)
		return p->nick;

	if (p->dummynick == NULL)
	{
		char * str, *nstr;
		int size = 8;
		int n;

		str = (char *)malloc(size);
		if (str == NULL)
			return NULL;

		while (1)
		{
			n = snprintf(str, size, "Player %d", p->pid);

			if (n > -1 && n < size)
			{
				if (size - n > 1)
					str = realloc(str, n + 1);
				break;
			}
			else if (n > -1)
				size = n + 1;
			else
			{
				free(str);
				return NULL;
			}

			if ((nstr = (char *)realloc(str, size)) == NULL)
			{
				free(str);
				return NULL;
			}
			else
				str = nstr;
		}

		p->dummynick = str;
	}
	
	return p->dummynick;
}

Bid * newBid(void)
{
	Bid * b = (Bid *)malloc(sizeof(Bid));

	if (b == NULL)
		return NULL;

	b->type = BID_NONE;
	b->p = NULL;
	b->count = 0;
	b->price = 0;
	b->next = NULL;

	return b;
}

void remBid(Bid * b)
{
	if (b == NULL)
		return;

	free(b);
}

BidList * newBidList(int sorting)
{
	BidList * l = (BidList *)malloc(sizeof(BidList));

	if (l == NULL)
		return NULL;

	l->f = NULL;
	l->sorting = sorting;

	return l;
}

void remBidList(BidList * l)
{
	Bid * t, * p;

	if (l == NULL)
		return;

	t = l->f;
	while (t != NULL)
	{
		p = t->next;
		remBid(t);
		t = p;
	}
	
	free(l);
}

void delBidFromList(Bid * b, BidList * l)
{
	Bid * t, * p;

	if (b == NULL || l == NULL)
		return;

	p = NULL;
	t = l->f;

	while (t != b && t != NULL)
	{
		p = t;
		t = p->next;
	}

	if (t == NULL)
		return;

	if (p == NULL)
		l->f = b->next;
	else
		p->next = b->next;
	b->next = NULL;
}

void addBidToList(Bid * b, BidList * l)
{
	Bid * t, * p;
	if (b == NULL || l == NULL)
		return;

	p = NULL;
	t = l->f;

	while (t != NULL &&
		((l->sorting == BID_SORTING_ASC && b->price > t->price) ||
		 (l->sorting == BID_SORTING_DESC && b->price < t->price)))
	{
		p = t;
		t = p->next;
	}

	if (p == NULL)
	{
		b->next = l->f;
		l->f = b;
	}
	else
	{
		b->next = p->next;
		p->next = b;
	}
}

int countBids(BidList * l)
{
	Bid * b;
	int i;

	if (l == NULL)
		return 0;

	for (i = 0, b = l->f; b != NULL; i++, b = b->next);

	return i;
}

Turn * newTurn(void)
{
	Turn * t = (Turn *)malloc(sizeof(Turn));

	if (t == NULL)
		return NULL;

	t->turn = 0;
	t->state = 0;
	t->prod = t->fac = t->sell = t->buy = NULL;

	return t;
}

void remTurn(Turn * t)
{
	if (t == NULL)
		return;

	remBidList(t->prod);
	remBidList(t->fac);
	remBidList(t->sell);
	remBidList(t->buy);

	free(t);
}

void fetchData(Player * p)
{
	int n;
	char buf[8];

	if ((n = read(p->fd, buf, 7)) > 0)
	{
		char * c;
		char * nick = getNickname(p);

		buf[n] = '\0';

		debug("Getted string from %s with n=%d: ", nick, n);
		for (c = buf; (c - buf) < n; c++)
			debug("%hhu ", *c);
		debug("\n");

		addnStr(p->buf, buf, n);
	}
	else if (n == 0)
	{
		Message mes;
		if (p->game == NULL)
		{
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = NULL;
			mes.type = MEST_PLAYER_LEAVES_HALL;
			mes.len = 0;
			mes.data = NULL;
		}
		else
		{
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_PLAYER_LEAVE_GAME;
			mes.len = 0;
			mes.data = NULL;
		}
		sendMessage(&mes);
		mes.sndr_t = O_PLAYER;
		mes.sndr.player = p;
		mes.rcvr_t = O_SERVER;
		mes.type = MEST_PLAYER_LEAVE_SERVER;
		mes.len = 0;
		mes.data = NULL;
		sendMessage(&mes);
	}
	else
		merror("read on fd=%d\n", p->fd);
}

void checkPlayerOnCommand(Player * p)
{
	while(checkBufferOnCommand(p))
		;
}

/* check on command and return 1 if command got
 * or 0 if didn't */
int checkBufferOnCommand(Player * p)
{
	int len = p->buf->count;
	int slen = 0;
	char * buf = flushBuffer(p->buf);
	char * sek = buf;
	Message mes;
	int * pnum;
	char * c;

	memset(&mes, 0, sizeof(mes));

	debug("Analyzing string with len=%d: ", len);
	for (c = buf; (c - buf) < len; c++)
		debug("%hhu ", *c);
	debug("\n");

	if (len == 0)
	{
		free(buf);
		return 0;
	}

	switch (*sek++)
	{
		case 1: /* get */
			if (len > 1)
				addnStr(p->buf, sek, len - 1);
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_GET;
			mes.len = 0;
			mes.data = NULL;
			sendMessage(&mes);
			free(buf);
			return 1;

		case 2: /* set N */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_SET;
			mes.len = 4;
			pnum = (int *)malloc(mes.len);
			*pnum = ntohl(*(uint32_t *)sek);
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 5)
				addnStr(p->buf, sek+4, len - 5);
			free(buf);
			return 1;

		case 3: /* join N */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_JOIN;
			mes.len = 4;
			pnum = (int *)malloc(mes.len);
			*pnum = ntohl(*(uint32_t *)sek);
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 5)
				addnStr(p->buf, sek+4, len - 5);
			free(buf);
			return 1;

		case 4: /* leave */
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_LEAVE;
			mes.len = 0;
			mes.data = NULL;
			sendMessage(&mes);
			free(buf);
			return 1;

		case 5: /* nick len NICKNAME */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}

			slen = ntohl(*(uint32_t *)sek);
			if (len < (5 + slen))
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}

			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_NICK;
			mes.len = slen + 1;
			c = (char *)malloc(mes.len);
			memcpy(c, sek + 4, slen);
			c[slen] = '\0';
			mes.data = c;
			sendMessage(&mes);

			if (len > (5 + slen))
				addnStr(p->buf, sek+4+slen, len - 5 - slen);
			free(buf);
			return 1;

		case 6: /* adm len PASSWORD */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}

			slen = ntohl(*(uint32_t *)sek);
			if (len < (5 + slen))
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}

			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_ADM;
			mes.len = slen + 1;
			c = (char *)malloc(mes.len);
			memcpy(c, sek + 4, slen);
			c[slen] = '\0';
			mes.data = c;
			sendMessage(&mes);

			if (len > (5 + slen))
				addnStr(p->buf, sek+4+slen, len - 5 - slen);
			free(buf);
			return 1;

		case 7: /* games */
			if (len > 1)
				addnStr(p->buf, sek, len - 1);
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_GAMES;
			mes.len = 0;
			mes.data = NULL;
			sendMessage(&mes);
			free(buf);
			return 1;

		case 8: /* players N */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_PLAYERS;
			mes.len = 4;
			pnum = (int *)malloc(mes.len);
			*pnum = ntohl(*(uint32_t *)sek);
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 5)
				addnStr(p->buf, sek+4, len - 5);
			free(buf);
			return 1;

		case 9: /* creategame */
			if (len > 1)
				addnStr(p->buf, sek, len - 1);
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_CREATEGAME;
			mes.len = 0;
			mes.data = NULL;
			sendMessage(&mes);
			free(buf);
			return 1;

		case 10: /* deletegame N */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_DELETEGAME;
			mes.len = 4;
			pnum = (int *)malloc(mes.len);
			*pnum = ntohl(*(uint32_t *)sek);
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 5)
				addnStr(p->buf, sek+4, len - 5);
			free(buf);
			return 1;

		case 11: /* player N */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_PLAYER;
			mes.len = 4;
			pnum = (int *)malloc(mes.len);
			*pnum = ntohl(*(uint32_t *)sek);
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 5)
				addnStr(p->buf, sek+4, len - 5);
			free(buf);
			return 1;

		case 12: /* start */
			if (len > 1)
				addnStr(p->buf, sek, len - 1);
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_START;
			mes.len = 0;
			mes.data = NULL;
			sendMessage(&mes);
			free(buf);
			return 1;


		case 13: /* turn */
			if (len > 1)
				addnStr(p->buf, sek, len - 1);
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_TURN;
			mes.len = 0;
			mes.data = NULL;
			sendMessage(&mes);
			free(buf);
			return 1;

		case 14: /* prod N */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_PROD;
			mes.len = 4;
			pnum = (int *)malloc(mes.len);
			*pnum = ntohl(*(uint32_t *)sek);
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 5)
				addnStr(p->buf, sek+4, len - 5);
			free(buf);
			return 1;

		case 15: /* market */
			if (len > 1)
				addnStr(p->buf, sek, len - 1);
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_MARKET;
			mes.len = 0;
			mes.data = NULL;
			sendMessage(&mes);
			free(buf);
			return 1;

		case 16: /* build N */
			if (len < 5)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_BUILD;
			mes.len = 4;
			pnum = (int *)malloc(mes.len);
			*pnum = ntohl(*(uint32_t *)sek);
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 5)
				addnStr(p->buf, sek+4, len - 5);
			free(buf);
			return 1;

		case 17: /* sell N P */
			if (len < 9)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_SELL;
			mes.len = 8;
			pnum = (int *)malloc(mes.len);
			pnum[0] = ntohl(*(uint32_t *)sek);
			pnum[1] = ntohl(*(uint32_t *)(sek + 4));
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 9)
				addnStr(p->buf, sek + 8, len - 9);
			free(buf);
			return 1;

		case 18: /* buy N P */
			if (len < 9)
			{
				addnStr(p->buf, buf, len);
				free(buf);
				return 0;
			}
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = p->game;
			mes.type = MEST_COMMAND_BUY;
			mes.len = 8;
			pnum = (int *)malloc(mes.len);
			pnum[0] = ntohl(*(uint32_t *)sek);
			pnum[1] = ntohl(*(uint32_t *)(sek + 4));
			mes.data = pnum;
			sendMessage(&mes);
			if (len > 9)
				addnStr(p->buf, sek + 8, len - 9);
			free(buf);
			return 1;

		default:
			mes.sndr_t = O_PLAYER;
			mes.sndr.player = p;
			mes.rcvr_t = O_SERVER;
			mes.type = MEST_COMMAND_UNKNOWN;
			mes.len = len;
			mes.data = buf;
			sendMessage(&mes);
			return 0;
	}
}

void startGame(Game * g)
{
	mPlayer * mp;

	if (g == NULL)
		return;

	g->started = 1;
	g->turn = 1;
	g->num = g->nPlayers;

	setState(g, GAME_STATE_DFL);

	g->rMatPrice = GAME_DFL_MATERIAL_RENT_PRICE;
	g->rProdPrice = GAME_DFL_PRODUCT_RENT_PRICE;
	g->rFacPrice = GAME_DFL_FACTORY_RENT_PRICE;
	g->bFacPrice = GAME_DFL_FACTORY_BUILD_PRICE;
	g->bFacTime = GAME_DFL_FACTORY_BUILD_TIME;
	g->prodPrice = GAME_DFL_PRODUCT_PRICE;
	g->prodStuff = GAME_DFL_PRODUCT_STUFF;

	for (mp = g->players; mp != NULL; mp = mp->next)
	{
		mp->player->turned = 0;

		mp->player->mat = GAME_DFL_MATERIAL;
		mp->player->prod = GAME_DFL_PRODUCT;
		mp->player->bf = GAME_DFL_FACTORIES;
		clearFactories(mp->player);
		mp->player->money = GAME_DFL_MONEY;
		mp->player->sellBid.c = 0;
		mp->player->sellBid.p = 0;
		mp->player->buyBid.c = 0;
		mp->player->buyBid.p = 0;
		mp->player->prodBid.c = 0;
	}
}

Turn * turnGame(Game * g)
{
	Turn * t = newTurn();
	mPlayer * mp;
	Bid * b;
	int i;

	if (g == NULL || t == NULL)
		return NULL;

	t->turn = g->turn;
	t->buy = newBidList(BID_SORTING_DESC);
	t->sell = newBidList(BID_SORTING_ASC);
	t->prod = newBidList(BID_SORTING_DESC);
	t->fac = newBidList(BID_SORTING_DESC);

	for (mp = g->players; mp != NULL; mp = mp->next)
	{
		int costs = 0;
		int nfac = 0;
		Bid * b;

		mp->player->turned = 0;

		costs += g->rProdPrice * mp->player->prod;
		costs += g->rMatPrice * mp->player->mat;
		costs += g->rFacPrice * mp->player->bf;

		costs += g->prodPrice * mp->player->prodBid.c;
		mp->player->prod += mp->player->prodBid.c;
		mp->player->mat -= g->prodStuff * mp->player->prodBid.c;
		if (mp->player->prodBid.c > 0)
		{
			b = newBid();
			b->type = BID_PLPROD;
			b->p = mp->player;
			b->count = mp->player->prodBid.c;
			addBidToList(b, t->prod);
		}

		nfac = checkFactories(mp->player);
		costs += g->bFacPrice * nfac;
		mp->player->bf += nfac;
		if (nfac > 0)
		{
			b = newBid();
			b->type = BID_PLFAC;
			b->p = mp->player;
			b->count = nfac;
			addBidToList(b, t->fac);
		}

		mp->player->money -= costs;

		if (isPlayerBankrupt(mp->player))
		{
			mp->player->sellBid.c = 0;
			mp->player->buyBid.c = 0;
		}
	}

	buyAuction(g, t->buy);
	sellAuction(g, t->sell);

	for (mp = g->players; mp != NULL; mp = mp->next)
	{
		int i = 0;

		for (b = t->buy->f; b != NULL && b->p != mp->player; b = b->next);

		if (b != NULL)
		{
			mp->player->money -= b->count*b->price;
			mp->player->mat += b->count;
			i = 1;
		}

		for (b = t->sell->f; b != NULL && b->p != mp->player; b = b->next);

		if (b != NULL)
		{
			mp->player->money += b->count*b->price;
			mp->player->prod -= b->count;
			i = 1;
		}

		if (i)
			isPlayerBankrupt(mp->player);
	}

	g->turn++;
	changeState(g);
	t->state = g->state.s;

	for (i = 0, mp = g->players; mp != NULL; mp = mp->next)
		if (mp->player->money < 0)
			i++;

	if (i >= g->nPlayers - 1)
	{
		Message mes;

		if (i == g->nPlayers - 1)
		{
			mes.sndr_t = O_GAME;
			mes.sndr.game = g;
			mes.rcvr_t = O_GAME;
			mes.rcvr.game = g;
			mes.type = MEST_PLAYER_WIN;
			mes.len = sizeof(Player *);
			mes.data = g->players->player;
			sendMessage(&mes);
		}

		mes.sndr_t = O_GAME;
		mes.sndr.game = g;
		mes.rcvr_t = O_GAME;
		mes.rcvr.game = g;
		mes.type = MEST_GAME_OVER;
		mes.len = 0;
		mes.data = NULL;
		sendMessage(&mes);
	}

	return t;
}

void buyAuction(Game * g, BidList * list)
{
	mPlayer * mp;
	BidList * l = newBidList(BID_SORTING_DESC);
	Bid * b;

	int i;
	int p;

	if (g == NULL || l == NULL || list == NULL)
		return;

	for (mp = g->players; mp != NULL; mp = mp->next)
		if (mp->player->buyBid.c > 0)
		{
			b = newBid();
			b->type = BID_MAT;
			b->p = mp->player;
			b->count = mp->player->buyBid.c;
			b->price = mp->player->buyBid.p;
			addBidToList(b, l);
		}

	if (l->f == NULL)
		return;

	while (l->f != NULL && l->f->price >= g->state.sellMinP &&
		g->state.sellC > 0)
	{
		p = l->f->price;
		i = 1;

		for (b = l->f->next; b != NULL && b->price == p; b = b->next)
			++i;

		while (i > 0 && g->state.sellC > 0)
		{
			int r = 1 + (int)(((double)i)*rand()/(RAND_MAX+1.0));

			for (b = l->f; r > 1; r--, b = b->next);

			if (g->state.sellC >= b->count)
				g->state.sellC -= b->count;
			else
			{
				b->count = g->state.sellC;
				g->state.sellC = 0;
			}

			delBidFromList(b, l);
			addBidToList(b, list);

			i--;
		}

		for (; i > 0; i--)
		{
			delBidFromList(b = l->f, l);
			remBid(b);
		}

		g->state.sellMinP = 1 + p;
	}

	remBidList(l);
}

void sellAuction(Game * g, BidList * list)
{
	mPlayer * mp;
	BidList * l = newBidList(BID_SORTING_ASC);
	Bid * b;

	int i;
	int p;

	if (g == NULL || l == NULL || list == NULL)
		return;

	for (mp = g->players; mp != NULL; mp = mp->next)
		if (mp->player->sellBid.c > 0)
		{
			b = newBid();
			b->type = BID_PROD;
			b->p = mp->player;
			b->count = mp->player->sellBid.c;
			b->price = mp->player->sellBid.p;
			addBidToList(b, l);
		}

	if (l->f == NULL)
		return;

	while (l->f != NULL && l->f->price <= g->state.buyMaxP &&
		g->state.buyC > 0)
	{
		p = l->f->price;
		i = 1;

		for (b = l->f->next; b != NULL && b->price == p; b = b->next)
			++i;

		while (i > 0 && g->state.buyC > 0)
		{
			int r = 1 + (int)(((double)i)*rand()/(RAND_MAX+1.0));

			for (b = l->f; r > 1; r--, b = b->next);

			if (g->state.buyC >= b->count)
				g->state.buyC -= b->count;
			else
			{
				b->count = g->state.buyC;
				g->state.buyC = 0;
			}

			delBidFromList(b, l);
			addBidToList(b, list);

			i--;
		}

		for (; i > 0; i--)
		{
			delBidFromList(b = l->f, l);
			remBid(b);
		}

		g->state.buyMaxP = p - 1;
	}

	remBidList(l);
}

int isPlayerBankrupt(Player * p)
{
	if (p->money < 0)   /* bankrupt! */
	{
		Message mes;
		mes.sndr_t = O_GAME;
		mes.sndr.game = p->game;
		mes.rcvr_t = O_GAME;
		mes.rcvr.game = p->game;
		mes.type = MEST_PLAYER_BANKRUPT;
		mes.len = sizeof(Player *);
		mes.data = p;
		sendMessage(&mes);
		return 1;
	}
	return 0;
}
