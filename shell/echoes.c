#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "echoes.h"

const char promtFormat[]    = "%s@ ";
const char promtFormatExt[] = "> ";

void echoPromt(int num)
{
	char * cwd;
	switch (num)
	{
		case PROMT_DEFAULT:
			cwd = getcwd(NULL, 0);
			printf(promtFormat, cwd);
			free(cwd);
			break;
		case PROMT_EXTENDED:
			printf(promtFormatExt);
			break;
	}
}

void echoError(int errno)
{
	switch (errno)
	{
		case ERROR_QUOTES:
			printf("Error: unbalansed quotes!\n");
			break;
	}
}
