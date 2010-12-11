#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


void printAiFamily(int);
void printAiType(int);
void printAiProto(int);

int main(int argc, char *argv[])
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int i, s;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s host port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	s = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (s != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		printf("ai_flags: %d\n", rp->ai_flags);
		printf("ai_family: "); printAiFamily(rp->ai_family); printf("\n");
		printf("ai_socktype: "); printAiType(rp->ai_socktype); printf("\n");
		printf("ai_protocol: "); printAiProto(rp->ai_protocol); printf("\n");
		printf("ai_addrlen: %d\n", rp->ai_addrlen);
		if (rp->ai_family == AF_INET)
		{
			printf("ai_addr:sin_family: "); printAiFamily(((struct sockaddr_in *)rp->ai_addr)->sin_family); printf("\n");
			printf("ai_addr:sin_port: %d\n", ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port));
			printf("ai_addr:sin_addr:s_addr: ");
			for (i = 0; i < 4; i++)
				printf("%hhu%s", ((char *)&(((struct sockaddr_in *)rp->ai_addr)->sin_addr.s_addr))[i], (i != 3 ? "." : ""));
			printf("\n");
		}
		if (rp->ai_next != NULL)
			printf("\n");
	}

	freeaddrinfo(result);

	return EXIT_SUCCESS;
}

void printAiFamily(int f)
{
	switch(f)
	{
		case AF_UNIX:
			printf("AF_UNIX");
			break;
		case AF_INET:
			printf("AF_INET");
			break;
		case AF_INET6:
			printf("AF_INET6");
			break;
		case AF_IPX:
			printf("AF_IPX");
			break;
		case AF_NETLINK:
			printf("AF_NETLINK");
			break;
		case AF_X25:
			printf("AF_X25");
			break;
		case AF_AX25:
			printf("AF_AX25");
			break;
		case AF_ATMPVC:
			printf("AF_ATMPVC");
			break;
		case AF_APPLETALK:
			printf("AF_APPLETALK");
			break;
		case AF_PACKET:
			printf("AF_PACKET");
			break;
		default:
			printf("UNKNOWN");
			break;
	}
}

void printAiType(int t)
{
	int i = 0;

	if (t & SOCK_STREAM)
		printf("%sSOCK_STREAM", (i != 0 ? " | " : ((i++), "")));
	if (t & SOCK_DGRAM)
		printf("%sSOCK_DGRAM", (i != 0 ? " | " : ((i++), "")));
	if (t & SOCK_SEQPACKET)
		printf("%sSOCK_SEQPACKET", (i != 0 ? " | " : ((i++), "")));
	if (t & SOCK_RAW)
		printf("%sSOCK_RAW", (i != 0 ? " | " : ((i++), "")));
	if (t & SOCK_RDM)
		printf("%sSOCK_RDM", (i != 0 ? " | " : ((i++), "")));
	if (t & SOCK_PACKET)
		printf("%sSOCK_PACKET", (i != 0 ? " | " : ((i++), "")));
	if (t & SOCK_NONBLOCK)
		printf("%sSOCK_NONBLOCK", (i != 0 ? " | " : ((i++), "")));
	if (t & SOCK_CLOEXEC)
		printf("%sSOCK_CLOEXEC", (i != 0 ? " | " : ((i++), "")));
}

void printAiProto(int p)
{
	struct protoent * pe;
	char ** sp;

	pe = getprotobynumber(p);

	printf("%s", pe->p_name);

	if (*(pe->p_aliases) != NULL)
		printf(" (");
	for (sp = pe->p_aliases; *sp != NULL; sp++)
		printf("%s%s", *sp, (*(sp+1) != NULL ? ", " : ""));
	if (*(pe->p_aliases) != NULL)
		printf(")");
}
