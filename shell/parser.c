#include "words.h"
#include "parser.h"
#include "echoes.h"

int parse(struct cmdElem ** cmdTree)
{
	return 0;
}

int processParsingErrors(int status)
{
	if (status == PARSE_ST_ERROR_QUOTES)
	{
		echoError(ERROR_QUOTES);
		return 1;
	}

	return 0;
}
