#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "client.h"

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

	s = getaddrinfo(cfg.host, cfg.port, &hints, &result);
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
	
	cfg.sfd = sfd;

	freeaddrinfo(result);
}

void unconnect()
{
	if (cfg.sfd == -1)
		return;

	shutdown(cfg.sfd, SHUT_RDWR);
	close(cfg.sfd);
	cfg.sfd = -1;
}

int defineTaskType(Task * t)
{
	int type = TASK_UNKNOWN;

	if (strcmp(t->argv[0], "get") == 0)
		type = TASK_GET;
	if (strcmp(t->argv[0], "set") == 0)
		type = TASK_SET;
	if (strcmp(t->argv[0], "join") == 0)
		type = TASK_JOIN;
	if (strcmp(t->argv[0], "leave") == 0)
		type = TASK_LEAVE;
	if (strcmp(t->argv[0], "nick") == 0)
		type = TASK_NICK;

	t->type = type;

	return type;
}

void sendCommand(Task * t)
{
	static char * comms[] = {
		"\01",
		"\02",
		"\03",
		"\04",
		"\05"
	};

	uint32_t i;

	if (t->argc == 1)
		switch (t->type)
		{
			case TASK_GET:
				write(cfg.sfd, comms[TASK_GET], 1);
				break;
			case TASK_LEAVE:
				write(cfg.sfd, comms[TASK_LEAVE], 1);
				break;
		}
	else if (t->argc == 2)
		switch (t->type)
		{
			case TASK_SET:
				i = htonl(atoi(t->argv[1]));
				write(cfg.sfd, comms[TASK_SET], 1);
				write(cfg.sfd, &i, sizeof(i));
				break;
			case TASK_JOIN:
				i = htonl(atoi(t->argv[1]));
				write(cfg.sfd, comms[TASK_JOIN], 1);
				write(cfg.sfd, &i, sizeof(i));
				break;
			case TASK_NICK:
				write(cfg.sfd, comms[TASK_NICK], 1);
				write(cfg.sfd, t->argv[1], strlen(t->argv[1])+1);
				break;
		}
}
