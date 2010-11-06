#include "../words.h"
#include "parser.h"
#include "../echoes.h"

struct parseStatus {
	int tmp;
};

void initParseStatus(struct parseStatus *);


int parse(struct cmdElem ** cmdTree)
{
	struct parseStatus status;
	struct wordlist words;

	initParseStatus(&status);
	initWordList(&words);

	echoPromt(PROMT_DEFAULT);

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

void initParseStatus(struct parseStatus * status)
{
	status->tmp = 0;
}
