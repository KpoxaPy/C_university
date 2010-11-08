#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"
#include "lexlist.h"

#define PT_STDIN 0
#define PT_STRING 1
/*#define PT_FILE 2*/


/*
 *struct gcInfo {
 *  int type;
 *  int quiet;
 *  int c;
 *
 *  const char * srcStr;
 *  struct bufferlist * buf;
 *} gcInfo = {GC_TYPE_STDIN, 1, -2, NULL, NULL};
 */


struct {
	int type;
	Lex * l;

	const char * srcStr;
	/*const char * fileStr;*/
	lexList * list;
} pSt = {PT_STDIN, NULL, NULL/*, NULL*/, NULL};

Lex * cur_l = NULL;

int parserErrorNo = PE_NONE;
Lex * errorLex = NULL;

void setParserError(int);
void resetParserError();

void waitLex();
int isWaitLex();
void gl();
void glhard();

tCmd * pT();
tCmd * pS();
tCmd * pL();
tCmd * puL();
tCmd * pP();
tCmd * pC();

void initParser()
{
	pSt.type = PT_STDIN;

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
	if (pSt.l != NULL)
	{
		delLex(pSt.l);
		pSt.l = NULL;
		cur_l = NULL;
	}
}

int isWaitLex()
{
	return pSt.l == NULL;
}

void gl()
{
	pSt.l = getlex();
	cur_l = pSt.l;
}

void glhard()
{
	if (pSt.l != NULL)
		delLex(pSt.l);
	pSt.l = getlex();
	cur_l = pSt.l;
}

void setParserError(int errType)
{
	char * str;
	parserErrorNo = errType;

	if (errorLex != NULL)
		delLex(errorLex);
	if (cur_l != NULL)
	{
		if(cur_l->str != NULL)
		{
			str = (char *)malloc(sizeof(char)*(strlen(cur_l->str + 1)));
			strcpy(str, cur_l->str);
		}
		consLex(cur_l->type, str);
	}
}

void resetParserError()
{
	parserErrorNo = PE_NONE;

	if (errorLex != NULL)
		delLex(errorLex);
}

int parse(tCmd ** cmd)
{
	int parseStatus;

	resetParserError();

	if (cmd == NULL)
	{
		parserErrorNo = PE_NULL_POINTER;
		return PS_ERROR;
	}

	if (isWaitLex())
		gl();

	*cmd = pT();

	if (*cmd != NULL)
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
			glhard();
		while (cur_l->type != LEX_EOL &&
			cur_l->type != LEX_EOF);
	}

	return parseStatus;
}

tCmd * pT()
{
	tCmd * cmd;

	if (cur_l->type == LEX_EOL)
	{
		glhard();
		return NULL;
	}
	else if (cur_l->type == LEX_EOF)
		return NULL;

	cmd = pS();

	if (cmd == NULL)
		return NULL;

	if (cur_l->type == LEX_BG)
	{
		cmd->modeBG = 1;
		glhard();
	}

	if (cur_l->type != LEX_EOL &&
		cur_l->type != LEX_EOF)
	{
		setParserError(PE_UNEXPECTED_END_OF_COMMAND);
		delTCmd(&cmd);
		return NULL;
	}

	waitLex();

	return cmd;
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
		glhard();

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
		glhard();

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
		glhard();

		cmd = pS();

		if (cmd == NULL)
			return NULL;

		if (cur_l->type != LEX_RPAREN)
		{
			setParserError(PE_EXPECTED_RPAREN);
			delTCmd(&cmd);
			return NULL;
		}
		glhard();
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
		glhard();

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
		gl();

		if (cur_l->type == LEX_WORD)
			addLex(pSt.list, pSt.l);
		else if (cur_l->type == LEX_REDIRECT_INPUT ||
			cur_l->type == LEX_REDIRECT_OUTPUT ||
			cur_l->type == LEX_REDIRECT_OUTPUT_APPEND)
		{
			int type = cur_l->type;
			glhard();

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

	smpl->argv = genArgVector(pSt.list);
	smpl->file = smpl->argv[0];

	cmd = genTCmd(smpl);
	cmd->cmdType = TCMD_SIMPLE;

	return cmd;
}

/*
 *  for (;;)
 *  {
 *    clearLexList(list);
 *
 *    for (;;)
 *    {
 *      lex = getlex();
 *
 *      if (lex != NULL)
 *        addLex(list, lex);
 *      else
 *        break;
 *
 *      if (lex->type == LEX_EOF || lex->type == LEX_EOL)
 *        break;
 *    }
 *
 *    if (lex != NULL)
 *      echoLexList(list);
 *    else
 *      printf("Inner error!\n");
 *
 *    if(lex == NULL || lex->type == LEX_EOF)
 *      break;
 *  }
 *  clearLexList(list);
 */
