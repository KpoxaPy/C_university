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

struct clientStatus cli = {NULL, NULL, 0, 0, -1, 0, 0, 0, 0, NULL, NULL};

void fetchDataFromServer(void);
void fetchDataFromClient(void);

int checkClientBufOnCommand(void);
int checkServerBufferOnInfo(void);

int defineTaskType(Task *);

void sendCommand(Task *);
void execCommand(Task *);

void processResponse(int, void *, int);

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
		"\01",
		"\02",
		"\03",
		"\04",
		"\05",
		"\06",
		"\07",
		"\010",
		"\011",
		"\012"
	};

	uint32_t i;

	cli.waitingForResponse = t->type | TASKT_SERVER;

	if (t->argc == 1)
		switch (t->type)
		{
			case TASK_GET:
				write(cli.sfd, comms[TASK_GET], 1);
				return;
			case TASK_LEAVE:
				write(cli.sfd, comms[TASK_LEAVE], 1);
				return;
			case TASK_GAMES:
				write(cli.sfd, comms[TASK_GAMES], 1);
				return;
			case TASK_CREATEGAME:
				write(cli.sfd, comms[TASK_CREATEGAME], 1);
				return;
		}
	else if (t->argc == 2)
		switch (t->type)
		{
			case TASK_SET:
				i = htonl(atoi(t->argv[1]));
				write(cli.sfd, comms[TASK_SET], 1);
				write(cli.sfd, &i, sizeof(i));
				return;
			case TASK_JOIN:
				i = htonl(atoi(t->argv[1]));
				write(cli.sfd, comms[TASK_JOIN], 1);
				write(cli.sfd, &i, sizeof(i));
				return;
			case TASK_NICK:
				write(cli.sfd, comms[TASK_NICK], 1);
				i = htonl(strlen(t->argv[1]));
				write(cli.sfd, &i, sizeof(i));
				write(cli.sfd, t->argv[1], strlen(t->argv[1]));
				return;
			case TASK_ADM:
				write(cli.sfd, comms[TASK_ADM], 1);
				i = htonl(strlen(t->argv[1]));
				write(cli.sfd, &i, sizeof(i));
				write(cli.sfd, t->argv[1], strlen(t->argv[1]));
				return;
			case TASK_PLAYERS:
				i = htonl(atoi(t->argv[1]));
				write(cli.sfd, comms[TASK_PLAYERS], 1);
				write(cli.sfd, &i, sizeof(i));
				return;
			case TASK_DELETEGAME:
				i = htonl(atoi(t->argv[1]));
				write(cli.sfd, comms[TASK_DELETEGAME], 1);
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
			if (type == RESP_PLAYERS && len >= 8)
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
					info("There's no players in the game #%d.\n", gid);
					break;
				}

				c = data + 8;
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
	}

	cli.waitingForResponse = 0;
}
