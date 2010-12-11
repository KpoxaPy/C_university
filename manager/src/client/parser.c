#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"
#include "lexlist.h"
#include "task.h"

#define PT_STDIN 0
#define PT_STRING 1
/*#define PT_FILE 2*/

struct {
	int type;
	Lex * l;
	int needLex;

	const char * srcStr;
	/*const char * fileStr;*/
	lexList * list;
} pSt = {PT_STDIN, NULL, 1, NULL/*, NULL*/, NULL};

Lex * cur_l = NULL;

int parserErrorNo = PE_NONE;
Lex * errorLex = NULL;

void setParserError(int);
void resetParserError(void);

void waitLex(void);
int isWaitLex(void);
void cl(void);
int gl(void);
int glhard(void);

Task * pT(void);
Task * pC(void);

void parseItQuiet()
{
	lexItQuiet();
}

void parseItVerbose()
{
	lexItVerbose();
}

void initParser()
{
	pSt.type = PT_STDIN;

	cl();
	waitLex();

	pSt.srcStr = NULL;
	/*pSt.fileStr = NULL;*/

	if (pSt.list == NULL)
		pSt.list = newLexList();
	else
		clearLexList(pSt.list);

	initLexer();
}

void initParserByString(char * str)
{
	pSt.type = PT_STRING;

	cl();
	waitLex();

	pSt.srcStr = str;
	/*pSt.fileStr = NULL;*/

	if (pSt.list == NULL)
		pSt.list = newLexList();
	else
		clearLexList(pSt.list);

	initLexerByString(str);
}

/*
 *void initParserByFile(char * file)
 *{
 *}
 */

void clearParser()
{
	pSt.type = PT_STDIN;

	cl();
	waitLex();

	pSt.srcStr = NULL;
	/*pSt.fileStr = NULL;*/

	if (pSt.list != NULL)
	{
		clearLexList(pSt.list);
		free(pSt.list);
		pSt.list = NULL;
	}

	clearLexer();
}

void waitLex()
{
	pSt.needLex = 1;
}

int isWaitLex()
{
	return pSt.needLex;
}

void cl()
{
	if (pSt.l != NULL)
	{
		delLex(pSt.l);
		pSt.l = NULL;
		cur_l = NULL;
	}
}

int gl()
{
	pSt.l = getlex();
	cur_l = pSt.l;
	pSt.needLex = 0;

	if (cur_l == NULL)
	{
		parserErrorNo = PE_LEXER_ERROR;
		return 1;
	}

	return 0;
}

int glhard()
{
	cl();
	return gl();
}

void setParserError(int errType)
{
	char * str = NULL;
	parserErrorNo = errType;

	if (errorLex != NULL)
		delLex(errorLex);
	if (cur_l != NULL)
	{
		if(cur_l->str != NULL)
			str = strdup(cur_l->str);

		errorLex = consLex(cur_l->type, str);
	}
}

void resetParserError()
{
	parserErrorNo = PE_NONE;

	if (errorLex != NULL)
		delLex(errorLex);
}

int parse(Task ** task)
{
	int parseStatus;

	resetParserError();

	if (task == NULL)
	{
		parserErrorNo = PE_NULL_POINTER;
		return PS_ERROR;
	}

	if (isWaitLex())
		if (gl())
			return PS_ERROR;

	*task = pT();

	if (*task != NULL)
		parseStatus = PS_OK;
	else if (parserErrorNo == PE_NONE)
	{
		if (cur_l->type == LEX_EOL)
			parseStatus = PS_EOL;
		else if (cur_l->type == LEX_EOF)
			parseStatus = PS_EOF;
	}
	else
		parseStatus = PS_ERROR;

	if (parseStatus == PS_ERROR &&
		cur_l->type != LEX_EOL &&
		cur_l->type != LEX_EOF)
	{
		do
			if (glhard())
				break;
		while (cur_l->type != LEX_EOL &&
			cur_l->type != LEX_EOF);
	}
	
	cl();

	return parseStatus;
}

Task * pT()
{
	Task * task = NULL;

	if (cur_l->type == LEX_EOL)
	{
		waitLex();
		return NULL;
	}
	else if (cur_l->type == LEX_EOF)
		return NULL;

	task = pC();

	if (cur_l->type != LEX_EOL &&
		cur_l->type != LEX_EOF)
	{
		setParserError(PE_UNEXPECTED_END_OF_COMMAND);
		delTask(&task);
		return NULL;
	}

	waitLex();

	return task;
}

Task * pC()
{
	Task * smpl = newTask();

	clearLexList(pSt.list);

	if (cur_l->type != LEX_WORD)
	{
		setParserError(PE_EXPECTED_WORD);
		delTask(&smpl);
		return NULL;
	}

	addLex(pSt.list, pSt.l);

	for(;;)
	{
		if (gl())
		{
			clearLexList(pSt.list);
			delTask(&smpl);
			return NULL;
		}

		if (cur_l->type == LEX_WORD)
			addLex(pSt.list, pSt.l);
		else
			break;
	}

	genArgVector(smpl, pSt.list);

	return smpl;
}
