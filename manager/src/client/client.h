#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "main.h"
#include "task.h"

#define POLL_SERVER_NEW 1
#define POLL_CLIENT_NEW 2

struct clientStatus {
	char * portstr;
	char * hoststr;
	int port;
	int host;
	int sfd;

	int pollStatus;
	int clientFin;
	int serverFin;
	int waitingForResponse;

	Buffer * sbuf, /* server content buffer */
		* cbuf; /* stdin content buffer */
};

extern struct clientStatus cli;

void initClient(void);
void unconnect(void);

void pollClient(void);

int checkClientOnCommand(void);
int checkServerOnInfo(void);

#endif
