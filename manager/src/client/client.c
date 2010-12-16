#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "client.h"
#include "parser.h"

#define RESP_OK 1
#define RESP_ERROR 2
#define RESP_STAT 3
#define RESP_GAMES 4
#define RESP_PLAYERS 5
#define RESP_PLAYER 6
#define RESP_MARKET 7

struct clientStatus cli = {NULL, NULL, 0, 0, -1, 0, 0, 0, 0, 1, NULL, NULL};

void fetchDataFromServer(void);
void fetchDataFromClient(void);

int checkClientBufOnCommand(void);
int checkServerBufferOnInfo(void);

int defineTaskType(Task *);

void sendCommand(Task *);
void execCommand(Task *);

void processResponse(int, void *, int);
void processInfo(void *, int);

void initClient()
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s;
	int sfd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	s = getaddrinfo(cli.hoststr, cli.portstr, &hints, &result);
	if (s != 0)
	{
		error("getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sfd);
	}
	
	if (rp == NULL)
	{
		error("Couldn't connect\n");
		endWork(EXIT_FAILURE);
	}
	
	cli.sfd = sfd;
	cli.port = ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port);
	cli.host = (uint32_t)(((struct sockaddr_in *)rp->ai_addr)->sin_addr.s_addr);

	cli.sbuf = newBuffer();
	cli.cbuf = newBuffer();

	freeaddrinfo(result);
}

void unconnect()
{
	if (cli.sfd == -1)
		return;

	shutdown(cli.sfd, SHUT_RDWR);
	close(cli.sfd);
	cli.sfd = -1;

	if (cli.sbuf != NULL)
	{
		clearBuffer(cli.sbuf);
		free(cli.sbuf);
	}

	if (cli.cbuf != NULL)
	{
		clearBuffer(cli.cbuf);
		free(cli.cbuf);
	}
}

void pollClient()
{
	fd_set rfs;
	int sel;

	FD_ZERO(&rfs);
	FD_SET(cli.sfd, &rfs);
	if (!cli.waitingForResponse)
		FD_SET(STDIN_FILENO, &rfs);

	sel = select(max(cli.sfd, STDIN_FILENO)+1, &rfs, NULL, NULL, NULL);
	if (sel < 1)
		merror("select");

	if (FD_ISSET(cli.sfd, &rfs))
	{
		fetchDataFromServer();
		cli.pollStatus |= POLL_SERVER_NEW;
	}

	if (FD_ISSET(STDIN_FILENO, &rfs))
	{
		fetchDataFromClient();
		cli.pollStatus |= POLL_CLIENT_NEW;
	}
}

/* returns
 * -1 if got EOF or
 * 0 if not enough commands remain and no renew
 * 1 if not enough commands remain and renew */
int checkClientOnCommand()
{
	int st;

	if (cli.pollStatus & POLL_CLIENT_NEW)
		cli.pollStatus &= ~POLL_CLIENT_NEW;
	else
		return 0;

	if (cli.clientFin)
		return -1;

	while ((st = checkClientBufOnCommand()) == 1)
		debugl(9, "checkClientOnCommand: Check client buffer on command returned %d\n", st);
	debugl(9, "checkClientOnCommand: Check client buffer on command returned %d\n", st);

	if (cli.clientFin)
		return -1;

	return 1;
}

/* returns
 *  0 if not enough commands remain or
 *  1 if commands remain in buffer */
int checkClientBufOnCommand()
{
	int len = (cli.cbuf)->count;
	char * buf = flushBuffer(cli.cbuf);
	char * c;
	Task * t;
	int stParse;

	if (len == 0)
	{
		debugl(9, "checkClientBufOnCommand: No string to analyze\n");
		free(buf);
		return 0;
	}

	debugl(6, "checkClientBufOnCommand: Analyzing string with len=%d: ", len);
	for (c = buf; (c - buf) < len; c++)
		debugl(6, "%hhu ", *c);
	debugl(6, "\n");

	debugl(9, "checkClientBufOnCommand: Attempting to get string: ");
	c = buf;
	while (*c != '\012' && (c - buf) < len)
	{
		debugl(9, "%c(%hhu) ", *c, *c);
		c++;
	}
	debugl(9, "end:%c(%hhu) ss=%d\n", *c, *c, c - buf);

	if (*c != '\012' || (c - buf) == len)
	{
		addnStr(cli.cbuf, buf, len);
		free(buf);
		return 0;
	}
	else if ((c - buf + 1) < len)
	{
		addnStr(cli.cbuf, c + 1, len - (c - buf + 1));
	}

	debugl(9, "checkClientBufOnCommand: String got successfully!\n");

	delTask(&t);
	initParserByString(buf);
	stParse = parse(&t);
	clearParser();

	if (stParse == PS_OK)
	{
		defineTaskType(t);

		if (t->type != TASK_UNKNOWN)
		{
			if (t->type & TASKT_SERVER)
			{
				t->type &= ~TASKT_SERVER;
				sendCommand(t);
			}
			else if (t->type &TASKT_CLIENT)
			{
				t->type &= ~TASKT_CLIENT;
				execCommand(t);
			}
		}
		else
			error("Unknown command!\n");
	}
	else if (stParse == PS_ERROR)
		echoParserError();

	if ((c - buf + 1) == len)
		return 0;
	else
		return 1;
}

/* returns
 * -1 if got EOF or
 * 0 if not enough info remain and no renew
 * 1 if not enough info remain and renew */
int checkServerOnInfo()
{
	int st;

	if (cli.pollStatus & POLL_SERVER_NEW)
		cli.pollStatus &= ~POLL_SERVER_NEW;
	else
		return 0;

	if (cli.serverFin)
		return -1;

	while ((st = checkServerBufferOnInfo()) == 1)
		debugl(9, "checkServerOnInfo: Check server buffer on info returned %d\n", st);
	debugl(9, "checkServerOnInfo: Check server buffer on info returned %d\n", st);

	if (cli.serverFin)
		return -1;

	return 1;
}

/* check on command and return 1 if command got
 * or 0 if didn't */
int checkServerBufferOnInfo()
{
	int len = (cli.sbuf)->count;
	int slen = 0;
	char * buf = flushBuffer(cli.sbuf);
	char * c;
	char * sek = buf;
	uint32_t * pnum;

	if (len == 0)
	{
		debugl(9, "checkServerBufferOnInfo: No string to analyze\n");
		free(buf);
		return 0;
	}

	debugl(6, "checkServerBufferOnInfo: Analyzing string with len=%d: ", len);
	for (c = buf; (c - buf) < len; c++)
		debugl(6, "%hhu ", *c);
	debugl(6, "\n");

	switch (*sek++)
	{
		case 1:  /* ok */
			processResponse(RESP_OK, NULL, 0);
			if (len > 1)
				addnStr(cli.sbuf, sek, len - 1);
			free(buf);
			return 1;

		case 2:  /* error N */
			if (len < 2)
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}
			processResponse(RESP_ERROR, sek, 1);
			if (len > 2)
				addnStr(cli.sbuf, sek+1, len - 2);
			free(buf);
			return 1;

		case 3:  /* stat N */
			if (len < 5)
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}
			pnum = (uint32_t *)malloc(sizeof(uint32_t));
			*pnum = ntohl(*(uint32_t *)sek);
			processResponse(RESP_STAT, pnum, 4);
			free(pnum);
			if (len > 5)
				addnStr(cli.sbuf, sek+4, len - 5);
			free(buf);
			return 1;

		case 4:  /* games GAMES */
			if (len < 5)
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}

			slen = ntohl(*(uint32_t *)sek);
			if (len < (5 + slen))
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}
			
			processResponse(RESP_GAMES, sek+4, slen);

			if (len > (5 + slen))
				addnStr(cli.sbuf, sek+4+slen, len - 5 - slen);
			free(buf);
			return 1;

		case 5:  /* players PLAYERS */
			if (len < 5)
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}

			slen = ntohl(*(uint32_t *)sek);
			if (len < (5 + slen))
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}
			
			processResponse(RESP_PLAYERS, sek+4, slen);

			if (len > (5 + slen))
				addnStr(cli.sbuf, sek+4+slen, len - 5 - slen);
			free(buf);
			return 1;

		case 6:  /* player PLAYER */
			if (len < 5)
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}

			slen = ntohl(*(uint32_t *)sek);
			if (len < (5 + slen))
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}
			
			processResponse(RESP_PLAYER, sek+4, slen);

			if (len > (5 + slen))
				addnStr(cli.sbuf, sek+4+slen, len - 5 - slen);
			free(buf);
			return 1;

		case 7:  /* info MES */
			if (len < 5)
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}

			slen = ntohl(*(uint32_t *)sek);
			if (len < (5 + slen))
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}
			
			processInfo(sek+4, slen);

			if (len > (5 + slen))
				addnStr(cli.sbuf, sek+4+slen, len - 5 - slen);
			free(buf);
			return 1;

		case 8:  /* market Players Turn sellCount:minPrice buyCount:maxPrice */
			if (len < 25)
			{
				addnStr(cli.sbuf, buf, len);
				free(buf);
				return 0;
			}

			processResponse(RESP_MARKET, sek, 24);

			if (len > 25)
				addnStr(cli.sbuf, sek + 24, len - 25);
			free(buf);
			return 1;


		default:
			error("Recieved unknown sequence from server, abort!\n");
			cli.clientFin = 1;
			return 0;
	}
}

int defineTaskType(Task * t)
{
	int type = TASK_UNKNOWN;

	if (strcmp(t->argv[0], "get") == 0)
		type = TASKT_SERVER | TASK_GET;
	else if (strcmp(t->argv[0], "set") == 0)
		type = TASKT_SERVER | TASK_SET;
	else if (strcmp(t->argv[0], "join") == 0)
		type = TASKT_SERVER | TASK_JOIN;
	else if (strcmp(t->argv[0], "leave") == 0)
		type = TASKT_SERVER | TASK_LEAVE;
	else if (strcmp(t->argv[0], "nick") == 0)
		type = TASKT_SERVER | TASK_NICK;
	else if (strcmp(t->argv[0], "adm") == 0)
		type = TASKT_SERVER | TASK_ADM;
	else if (strcmp(t->argv[0], "games") == 0)
		type = TASKT_SERVER | TASK_GAMES;
	else if (strcmp(t->argv[0], "players") == 0)
		type = TASKT_SERVER | TASK_PLAYERS;
	else if (strcmp(t->argv[0], "creategame") == 0)
		type = TASKT_SERVER | TASK_CREATEGAME;
	else if (strcmp(t->argv[0], "deletegame") == 0)
		type = TASKT_SERVER | TASK_DELETEGAME;
	else if (strcmp(t->argv[0], "player") == 0)
		type = TASKT_SERVER | TASK_PLAYER;
	else if (strcmp(t->argv[0], "start") == 0)
		type = TASKT_SERVER | TASK_START;
	else if (strcmp(t->argv[0], "turn") == 0)
		type = TASKT_SERVER | TASK_TURN;
	else if (strcmp(t->argv[0], "prod") == 0)
		type = TASKT_SERVER | TASK_PROD;
	else if (strcmp(t->argv[0], "market") == 0)
		type = TASKT_SERVER | TASK_MARKET;
	else if (strcmp(t->argv[0], "build") == 0)
		type = TASKT_SERVER | TASK_BUILD;
	else if (strcmp(t->argv[0], "buy") == 0)
		type = TASKT_SERVER | TASK_BUY;
	else if (strcmp(t->argv[0], "sell") == 0)
		type = TASKT_SERVER | TASK_SELL;
	else if (strcmp(t->argv[0], "exit") == 0)
		type = TASKT_CLIENT | TASK_EXIT;

	t->type = type;

	return type;
}

void fetchDataFromServer()
{
	int n;
	char buf[32];

	if ((n = read(cli.sfd, buf, 31)) > 0)
	{
		char * c;

		buf[n] = '\0';

		debugl(6, "Getted string from server with n=%d: ", n);
		for (c = buf; (c - buf) < n; c++)
			debugl(6, "%hhu ", *c);
		debugl(6, "\n");

		addnStr(cli.sbuf, buf, n);
	}
	else if (n == 0)
		cli.serverFin = 1;
	else
		merror("read on server fd\n");
}

void fetchDataFromClient()
{
	int n;
	char buf[32];

	if ((n = read(STDIN_FILENO, buf, 31)) > 0)
	{
		char * c;

		buf[n] = '\0';

		debugl(6, "Getted string from input with n=%d: ", n);
		for (c = buf; (c - buf) < n; c++)
			debugl(6, "%hhu ", *c);
		debugl(6, "\n");

		addnStr(cli.cbuf, buf, n);
	}
	else if (n == 0)
		cli.clientFin = 1;
	else
		merror("read on standart input fd\n");
}

void sendCommand(Task * t)
{
	static char * comms[] = {
		"\01", /* TASK_GET */
		"\02", /* TASK_SET  */
		"\03", /* TASK_JOIN  */
		"\04", /* TASK_LEAVE  */
		"\05", /* TASK_NICK  */
		"\06", /* TASK_ADM  */
		"\07", /*  TASK_GAMES */
		"\010", /* TASK_PLAYERS  */
		"\011", /* TASK_CREATEGAME  */
		"\012", /* TASK_DELETEGAME  */
		"\013", /* TASK_PLAYER  */
		"\014", /* TASK_START  */
		"\015", /* TASK_TURN  */
		"\016", /* TASK_PROD  */
		"\017", /* TASK_MARKET  */
		"\020", /* TASK_BUILD  */
		"\021", /* TASK_SELL  */
		"\022", /* TASK_BUY  */
	};

	uint32_t i;

	cli.waitingForResponse = t->type | TASKT_SERVER;

	if (t->argc == 1)
		switch (t->type)
		{
			case TASK_GET:
			case TASK_LEAVE:
			case TASK_GAMES:
			case TASK_CREATEGAME:
			case TASK_START:
			case TASK_TURN:
			case TASK_MARKET:
				write(cli.sfd, comms[t->type], 1);
				return;
			case TASK_PLAYERS:
				write(cli.sfd, comms[TASK_PLAYERS], 1);
				i = htonl(0);
				write(cli.sfd, &i, sizeof(i));
			case TASK_PROD:
			case TASK_BUILD:
				write(cli.sfd, comms[t->type], 1);
				i = htonl(1);
				write(cli.sfd, &i, sizeof(i));
				return;
		}
	else if (t->argc == 2)
		switch (t->type)
		{
			case TASK_SET:
			case TASK_PLAYERS:
			case TASK_JOIN:
			case TASK_DELETEGAME:
			case TASK_PLAYER:
			case TASK_PROD:
			case TASK_BUILD:
				write(cli.sfd, comms[t->type], 1);
				i = htonl(atoi(t->argv[1]));
				write(cli.sfd, &i, sizeof(i));
				return;
			case TASK_NICK:
			case TASK_ADM:
				write(cli.sfd, comms[t->type], 1);
				i = htonl(strlen(t->argv[1]));
				write(cli.sfd, &i, sizeof(i));
				write(cli.sfd, t->argv[1], strlen(t->argv[1]));
				return;
		}
	else if (t->argc == 3)
		switch(t->type)
		{
			case TASK_BUY:
			case TASK_SELL:
				write(cli.sfd, comms[t->type], 1);
				i = htonl(atoi(t->argv[1]));
				write(cli.sfd, &i, sizeof(i));
				i = htonl(atoi(t->argv[2]));
				write(cli.sfd, &i, sizeof(i));
				return;
		}

	cli.waitingForResponse = 0;

	error("Command not proccesed!\n");
}

void execCommand(Task * t)
{
	if (t->argc == 1)
		switch (t->type)
		{
			case TASK_EXIT:
				cli.clientFin = 1;
				return;
		}
	error("Command not proccesed!\n");
}

void processResponse(int type, void * data, int len)
{
	if (!(cli.waitingForResponse & TASKT_SERVER))
	{
		cli.waitingForResponse = 0;
		return;
	}

	switch (cli.waitingForResponse & ~TASKT_SERVER)
	{
		case TASK_GET:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You don't in game yet!\n");
			}
			else if (type == RESP_STAT && len == 4)
			{
				uint32_t i = *(uint32_t *)data;

				info("Game variable is %d.\n", i);
			}
			break;

		case TASK_SET:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You don't in game yet!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_JOIN:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You already are in this game!\n");
				else if (c == 1)
					error("Game not found!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_LEAVE:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You don't in game yet!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_NICK:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("Invalid nickname!\n");
				else if (c == 1)
					error("Unavailable nickname -- there is other player with this nickname on the server!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_ADM:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("Failed!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_GAMES:
			if (type == RESP_GAMES && len >= 4)
			{
				int i = ntohl(*(uint32_t *)data);
				int n;
				struct {
					uint32_t gid;
					uint32_t players;
				} * p;

				debugl(9, "response: len = %d\n", len);
				debugl(9, "response: i = %d\n", i);

				if (i == 0)
				{
					info("There's no games on the server.\n");
					break;
				}

				if (len != (4 + i*8))
				{
					error("Data is corrupted!\n");
					break;
				}

				info("There'%s %d game%s on the server.\n", (i > 1 ? "re" : "s"), i, (i > 1 ? "s" : ""));
				for (n = 0, p = data + 4; n < i; n++, p++)
				{
					p->gid = ntohl(p->gid);
					p->players = ntohl(p->players);
					info("#%d: ", p->gid);
					if (p->players == 0)
						info("no players.\n");
					else if (p->players == 1)
						info("one player.\n");
					else
						info("%d players.\n", p->players);
				}
			}
			break;

		case TASK_PLAYERS:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("Room not found!\n");
			}
			else if (type == RESP_PLAYERS && len >= 8)
			{
				int gid = ntohl(*(uint32_t *)data);
				int num = ntohl(*(uint32_t *)(data + 4));
				char * c = data;
				int i;

				debugl(9, "response: len = %d\n", len);
				debugl(9, "response: gid = %d\n", gid);
				debugl(9, "response: num = %d\n", num);

				if (num == 0)
				{
					if (gid == 0)
						info("There's no players in the server's hall.\n");
					else
						info("There's no players in the game #%d.\n", gid);
					break;
				}

				c = data + 8;
				if (gid == 0)
					info("There'%s %d player%s in the server's hall.\n",
						(num > 1 ? "re" : "s"), num, (num > 1 ? "s" : ""), gid);
				else
					info("There'%s %d player%s in the game #%d.\n",
						(num > 1 ? "re" : "s"), num, (num > 1 ? "s" : ""), gid);
				for (i = 0; i < num; i++)
				{
					int pid = ntohl(*(uint32_t *)c); c += 4;
					int slen = ntohl(*(uint32_t *)c); c += 4;
					char * s = alloca(slen + 1);
					memcpy(s, c, slen);
					s[slen] = '\0';
					info("%d: #%d %s\n", i+1, pid, s);
					c += slen;
				}
			}
			break;

		case TASK_CREATEGAME:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You don't granted to create games on the server!\n");
			}
			else if (type == RESP_STAT && len == 4)
			{
				uint32_t i = *(uint32_t *)data;

				info("New game's GID is %d.\n", i);
			}
			break;

		case TASK_DELETEGAME:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You don't granted to delete games on the server!\n");
				else if (c == 1)
					error("Game not found!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_PLAYER:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("Player not found!\n");
			}
			else if (type == RESP_PLAYER && len >= 8)
			{
				int pid = ntohl(*(uint32_t *)data);
				int gid = ntohl(*(uint32_t *)(data + 4));
				int started = ntohl(*(uint32_t *)(data + 8));
				int mat = ntohl(*(uint32_t *)(data + 12));
				int prod = ntohl(*(uint32_t *)(data + 16));
				int bf = ntohl(*(uint32_t *)(data + 20));
				int cf = ntohl(*(uint32_t *)(data + 24));
				int money = ntohl(*(uint32_t *)(data + 28));
				int slen = ntohl(*(uint32_t *)(data + 32));
				char * s = alloca(slen + 1);
				memcpy(s, data + 36, slen);
				s[slen] = '\0';

				debugl(9, "response: len = %d\n", len);
				debugl(9, "response: pid = %d\n", pid);
				debugl(9, "response: gid = %d\n", gid);

				info("Player #%d: %s ", pid, s);
				if (gid == 0)
					info("in the hall\n");
				else
					info("in game #%d\n", gid);
				if (started)
				{
					info("Player have: %d money\n", money);
					info("Player have: %d products\n", prod);
					info("Player have: %d materials\n", mat);
					info("Player have: %d built factories\n", bf);
					info("Player have: %d building facs\n", cf);
				}
			}
			break;

		case TASK_START:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You don't granted to start games!\n");
				else if (c == 1)
					error("You aren't in the game!\n");
				else if (c == 2)
					error("There're no players!\n");
				else if (c == 3)
					error("Game is already started\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_TURN:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You aren't in the game!\n");
				else if (c == 1)
					error("You have turned already!\n");
				else if (c == 2)
					error("Game isn't started yet!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_PROD:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You aren't in the game!\n");
				else if (c == 1)
					error("Wrong value of production!\n");
				else if (c == 2)
					error("You haven't enought money!\n");
				else if (c == 3)
					error("You have turned already!\n");
				else if (c == 4)
					error("Game isn't started yet!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_MARKET:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You aren't in the game!\n");
				else if (c == 1)
					error("Game isn't started yet!\n");
			}
			else if (type == RESP_MARKET && len == 24)
			{
				int players = ntohl(*(uint32_t *)data);
				int turn = ntohl(*(uint32_t *)(data + 4));
				int sellC = ntohl(*(uint32_t *)(data + 8));
				int sellMinP = ntohl(*(uint32_t *)(data + 12));
				int buyC = ntohl(*(uint32_t *)(data + 16));
				int buyMaxP = ntohl(*(uint32_t *)(data + 20));

				info("Current month is %d%s\n", turn,
					(turn == 1 ? "st" :
					(turn == 2 ? "nd" :
					(turn == 3 ? "rd" : "th"))));
				info("Players in game %d\n", players);
				info("Bank sells %d materials with minimal price %d\n", sellC, sellMinP);
				info("Bank buys %d products with maximal price %d\n", buyC, buyMaxP);
			}
			break;

		case TASK_BUILD:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You aren't in the game!\n");
				else if (c == 1)
					error("Wrong value of factories!\n");
				else if (c == 2)
					error("You haven't enought money!\n");
				else if (c == 3)
					error("You have turned already!\n");
				else if (c == 4)
					error("Game isn't started yet!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_SELL:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You aren't in the game!\n");
				else if (c == 1)
					error("Price is too high!\n");
				else if (c == 2)
					error("You haven't enought goods to sell!\n");
				else if (c == 3)
					error("Wrong value of goods!\n");
				else if (c == 4)
					error("You have turned already!\n");
				else if (c == 5)
					error("Game isn't started yet!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;

		case TASK_BUY:
			if (type == RESP_ERROR && len == 1)
			{
				unsigned char c = *(char *)data;

				if (c == 0)
					error("You aren't in the game!\n");
				else if (c == 1)
					error("Price is too low!\n");
				else if (c == 2)
					error("You haven't enought money!\n");
				else if (c == 3)
					error("Wrong value of goods!\n");
				else if (c == 4)
					error("You have turned already!\n");
				else if (c == 5)
					error("Game isn't started yet!\n");
			}
			else if (type == RESP_OK && len == 0)
				debugl(4, "Ok!\n");
			break;
	}

	cli.waitingForResponse = 0;
}

void processInfo(void * data, int len)
{
	uint32_t i, j;
	char * str;

	if (cli.infoCarret == 1)
	{
		info("\n");
		cli.infoCarret = 0;
	}

	switch (*(char *)data)
	{
		case 1:
			i = ntohl(*(uint32_t *)(data+1));
			info("New player #%d on the server\n", i);
			break;
		case 2:
			i = ntohl(*(uint32_t *)(data+1));
			info("Player #%d has leaved the server\n", i);
			break;
		case 3:
			i = ntohl(*(uint32_t *)(data+1));
			j = ntohl(*(uint32_t *)(data+5));
			str = alloca(j + 1);
			memcpy(str, data+9, j);
			str[j] = '\0';
			info("Player #%d has changed his nickname to '%s'\n", i, str);
			break;
		case 4:
			i = ntohl(*(uint32_t *)(data+1));
			info("Created new game #%d\n", i);
			break;
		case 5:
			i = ntohl(*(uint32_t *)(data+1));
			info("Player #%d has leaved hall\n", i);
			break;
		case 6:
			i = ntohl(*(uint32_t *)(data+1));
			info("Player #%d has entered into hall\n", i);
			break;
		case 7:
			i = ntohl(*(uint32_t *)(data+1));
			info("Player #%d has joined into game\n", i);
			break;
		case 8:
			i = ntohl(*(uint32_t *)(data+1));
			j = ntohl(*(uint32_t *)(data+5));
			info("Player's game stat: #%d #%d\n", i, j);
			break;
		case 9:
			i = ntohl(*(uint32_t *)(data+1));
			info("Player #%d has leaved game\n", i);
			break;
		case 10:
			i = ntohl(*(uint32_t *)(data+1));
			info("Game #%d over\n", i);
			break;
		case 11:
			info("You have been kicked from game\n");
			break;
		case 12:
			info("You have been kicked from server\n");
			break;
		case 13:
			i = ntohl(*(uint32_t *)(data+1));
			info("Your pid = %d\n", i);
			break;
		case 14:
			info("Game has been started\n");
			break;
		case 15:
			i = ntohl(*(uint32_t *)(data+1));
			j = ntohl(*(uint32_t *)(data+5));
			info("There're players not turned yet: %d/%d\n", j, i);
			break;
		case 16:
			i = ntohl(*(uint32_t *)(data+1));
			info("Player #%d has turned\n", i);
			break;
		case 17:
			info("Turn was ended\n");
			{
				char * sek = data + 1;
				int turn =      ntohl(*(uint32_t *)(sek));
				int bidsCount = ntohl(*(uint32_t *)(sek + 4));
				int prod;
				int fac;
				int i;

				info("Game month #%d\n", turn);

				sek += 8;
				for (i = 0; i < bidsCount; i++)
				{
					int type = ntohl(*(uint32_t *)(sek));;
					int pid = ntohl(*(uint32_t *)(sek + 4));;
					int num = ntohl(*(uint32_t *)(sek + 8));;
					int price = ntohl(*(uint32_t *)(sek + 12));;
					info("%s bid from player #%d: ", (type == 1 ? "BUY" : "SELL"), pid);
					if (type == 1)
						info("sold %d material with price %d\n", num, price);
					else
						info("bought %d production with price %d\n", num, price);
					sek += 16;
				}
				prod = ntohl(*(uint32_t *)(sek));;
				if (prod > 0)
					info("Your factories producted %d good\n", prod, (prod > 1 ? "s" : ""));
				fac = ntohl(*(uint32_t *)(sek + 4));;
				if (fac > 0)
					info("There is builded %d factor%s\n", fac, (fac > 1 ? "ies": "y"));
			}
			break;
		case 18:
			i = ntohl(*(uint32_t *)(data+1));
			info("Player #%d is bankrupt!\n", i);
			break;
		case 19:
			i = ntohl(*(uint32_t *)(data+1));
			info("Player #%d is winner!\n", i);
			break;
	}
}
