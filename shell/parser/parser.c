#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"
#include "lexlist.h"
#include "../run/command.h"
#include "../run/task.h"

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
void resetParserError();

void waitLex();
int isWaitLex();
void cl();
int gl();
int glhard();

Task * pT();
tCmd * pS();
tCmd * pL();
tCmd * puL();
tCmd * pP();
tCmd * pC();

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

	return parseStatus;
}

Task * pT()
{
	Task * task = NULL;
	tCmd * cmd;

	if (cur_l->type == LEX_EOL)
	{
		waitLex();
		return NULL;
	}
	else if (cur_l->type == LEX_EOF)
		return NULL;

	cmd = pS();

	if (cmd == NULL)
		return NULL;

	task = newTask();

	if (cur_l->type == LEX_BG)
	{
		if (glhard())
		{
			delTask(&task);
			delTCmd(&cmd);
			return NULL;
		}
		task->modeBG = 1;
	}

	if (cur_l->type != LEX_EOL &&
		cur_l->type != LEX_EOF)
	{
		setParserError(PE_UNEXPECTED_END_OF_COMMAND);
		delTask(&task);
		delTCmd(&cmd);
		return NULL;
	}

	task->root = cmd;
	task->cur = cmd;

	waitLex();

	return task;
}

tCmd * pS()
{
	tCmd * cmd, *pcmd;
	int iteration = 0;

	cmd = pL();

	if (cmd == NULL)
		return NULL;

	while (cur_l->type == LEX_SCRIPT)
	{
		tCmd * tmp;
		int type = TREL_SCRIPT;
		if (glhard())
		{
			delTCmd(&cmd);
			return NULL;
		}

		tmp = pL();

		if (tmp == NULL)
		{
			delTCmd(&cmd);
			return NULL;
		}

		if (!iteration)
		{
			pcmd = tmp;
			tmp = genTCmd(NULL);
			tmp->cmdType = TCMD_SCRIPT;
			genRelation(cmd, type, pcmd);
			tmp->child = cmd;
			cmd = tmp;
			iteration = 1;
		}
		else
		{
			genRelation(pcmd, type, tmp);
			pcmd = tmp;
		}
	}

	if (iteration)
		genRelation(pcmd, TREL_END, cmd);

	return cmd;
}

tCmd * pL()
{
	tCmd * cmd, *pcmd;
	int iteration = 0;

	cmd = puL();

	if (cmd == NULL)
		return NULL;

	while (cur_l->type == LEX_AND ||
		cur_l->type == LEX_OR)
	{
		tCmd * tmp;
		int type = (cur_l->type == LEX_AND ?
			TREL_AND : TREL_OR);
		if (glhard())
		{
			delTCmd(&cmd);
			return NULL;
		}

		tmp = puL();

		if (tmp == NULL)
		{
			delTCmd(&cmd);
			return NULL;
		}

		if (!iteration)
		{
			pcmd = tmp;
			tmp = genTCmd(NULL);
			tmp->cmdType = TCMD_LIST;
			genRelation(cmd, type, pcmd);
			tmp->child = cmd;
			cmd = tmp;
			iteration = 1;
		}
		else
		{
			genRelation(pcmd, type, tmp);
			pcmd = tmp;
		}
	}

	if (iteration)
		genRelation(pcmd, TREL_END, cmd);

	return cmd;
}

tCmd * puL()
{
	tCmd * cmd;

	if (cur_l->type == LEX_LPAREN)
	{
		if (glhard())
			return NULL;

		cmd = pS();

		if (cmd == NULL)
			return NULL;

		if (cur_l->type != LEX_RPAREN)
		{
			setParserError(PE_EXPECTED_RPAREN);
			delTCmd(&cmd);
			return NULL;
		}
		if (glhard())
		{
			delTCmd(&cmd);
			return NULL;
		}
	}
	else
	{
		cmd = pP();

		if (cmd == NULL)
			return NULL;
	}

	return cmd;
}

tCmd * pP()
{
	tCmd * cmd, *pcmd;
	int iteration = 0;

	cmd = pC();

	if (cmd == NULL)
		return NULL;

	while (cur_l->type == LEX_PIPE)
	{
		tCmd * tmp;
		int type = TREL_PIPE;
		if (glhard())
		{
			delTCmd(&cmd);
			return NULL;
		}

		tmp = pC();

		if (tmp == NULL)
		{
			delTCmd(&cmd);
			return NULL;
		}

		if (!iteration)
		{
			pcmd = tmp;
			tmp = genTCmd(NULL);
			tmp->cmdType = TCMD_PIPE;
			genRelation(cmd, type, pcmd);
			tmp->child = cmd;
			cmd = tmp;
			iteration = 1;
		}
		else
		{
			genRelation(pcmd, type, tmp);
			pcmd = tmp;
		}
	}

	if (iteration)
		genRelation(pcmd, TREL_END, cmd);

	return cmd;
}

tCmd * pC()
{
	tCmd * cmd;
	simpleCmd * smpl = newCommand();

	clearLexList(pSt.list);

	if (cur_l->type != LEX_WORD)
	{
		setParserError(PE_EXPECTED_WORD);
		delCommand(&smpl);
		return NULL;
	}

	addLex(pSt.list, pSt.l);

	for(;;)
	{
		if (gl())
		{
			clearLexList(pSt.list);
			delCommand(&smpl);
			return NULL;
		}

		if (cur_l->type == LEX_WORD)
			addLex(pSt.list, pSt.l);
		else if (cur_l->type == LEX_REDIRECT_INPUT ||
			cur_l->type == LEX_REDIRECT_OUTPUT ||
			cur_l->type == LEX_REDIRECT_OUTPUT_APPEND)
		{
			int type = cur_l->type;
			if (glhard())
			{
				clearLexList(pSt.list);
				delCommand(&smpl);
				return NULL;
			}

			if (cur_l->type != LEX_WORD)
			{
				setParserError(PE_EXPECTED_WORD);
				if(smpl->rdrInputFile != NULL)
					free(smpl->rdrInputFile);
				if(smpl->rdrOutputFile != NULL)
					free(smpl->rdrOutputFile);
				delCommand(&smpl);
				delLex(cur_l);
				clearLexList(pSt.list);
				return NULL;
			}

			if (type == LEX_REDIRECT_INPUT)
			{
				if(smpl->rdrInputFile != NULL)
					free(smpl->rdrInputFile);
				smpl->rdrInputFile = cur_l->str;
			}
			else if (type == LEX_REDIRECT_OUTPUT ||
				type == LEX_REDIRECT_OUTPUT_APPEND)
			{
				if (type == LEX_REDIRECT_OUTPUT_APPEND)
					smpl->rdrOutputAppend = 1;
				else
					smpl->rdrOutputAppend = 0;

				if(smpl->rdrOutputFile != NULL)
					free(smpl->rdrOutputFile);
				smpl->rdrOutputFile = cur_l->str;
			}
		}
		else
			break;
	}

	genArgVector(smpl, pSt.list);
	smpl->file = smpl->argv[0];

	cmd = genTCmd(smpl);
	cmd->cmdType = TCMD_SIMPLE;

	return cmd;
}
