#include <sys/socket.h>
#include <netinet/ip.h>
#include "server.h"

struct server {
	int ld;

	int started;
} srv;

void startServer(void);

void initServer()
{
	srv.ld = -1;
	srv.started = 0;

	startServer();
}

void startServer()
{
	struct sockaddr_in a;

	a.sin_family = AF_INET;
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_port = htons(cfg.port);

	srv.ld = socket(AF_INET, SOCK_STREAM, 0);

	if (bind(srv.ld, (struct sockaddr *)&a, sizeof(a)) == -1)
	{
		error("%m: bind");
		endWork(EXIT_FAILURE);
	}
	else
		debug("Successfuly binded on *:%d\n", cfg.port);

	if (listen(srv.ld, 5) == -1)
	{
		error("%m: listen");
		endWork(EXIT_FAILURE);
	}
	else
		debug("Starting listening...");

	srv.started = 1;
}

void shutdownServer()
{
	if (srv.started == 0)
		return;

	close(srv.ld);
}
