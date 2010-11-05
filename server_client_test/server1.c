#include <sys/times.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main()
{
	int lsock = socket(AF_INET, SOCK_STREAM, 0);
	int sd;
	struct sockaddr_in a;
	struct sockaddr * client;
	socklen_t client_len;

	a.sin_family = AF_INET;
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_port = htons(7777);

	if (bind(lsock, (struct sockaddr *)&a, sizeof(a)) == -1)
	{
		perror("bind");
		exit(1);
	}

	listen(lsock, 5);

	for(;;)
	{
		/*sd = accept(lsock, client, &client_len);*/
		sd = accept(lsock, NULL, NULL);

		if (!fork())
		{
			char buf[32];
			int rc;

			close(lsock);

			printf("Client connected...\n");
			printf("SD=%d\n", sd);
			
			// Doing something
			while ((rc = recv(sd, buf, sizeof(buf)-1, 0)) != 0)
			{
				if (rc > 0)
				{
					buf[rc] = '\0';
					printf("%s\n", buf);
				}
				else
				{
					perror("read");
					break;
				}
			}

			printf("Client disconnected...\n");
			
			exit(0);
		}

		close(sd);

		while(waitpid(-1, NULL, WNOHANG) > 0)
			;
	}

	return 0;
}
